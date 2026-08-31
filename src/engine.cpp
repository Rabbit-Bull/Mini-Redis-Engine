#include "mini_redis/engine.hpp"
#include "mini_redis/fen_compressor.hpp"
#include "mini_redis/parser.hpp"
#include <iostream>
#include <sstream>

namespace mini_redis {

DatabaseEngine::DatabaseEngine(const std::string& wal_filename)
    : wal_filename_(wal_filename) {
    replay_wal();
    wal_stream_.open(wal_filename_, std::ios::app);
}

DatabaseEngine::~DatabaseEngine() {
    if (wal_stream_.is_open()) {
        wal_stream_.flush();
        wal_stream_.close();
    }
}

std::string DatabaseEngine::escape_string(const std::string& str) {
    std::string result = "\"";
    for (char c : str) {
        if (c == '"') {
            result += "\\\"";
        } else if (c == '\\') {
            result += "\\\\";
        } else if (c == '\n') {
            result += "\\n";
        } else if (c == '\t') {
            result += "\\t";
        } else if (c == '\r') {
            result += "\\r";
        } else {
            result += c;
        }
    }
    result += "\"";
    return result;
}

void DatabaseEngine::replay_wal() {
    std::ifstream in(wal_filename_);
    if (!in.is_open()) {
        return;
    }

    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty()) {
            continue;
        }

        Command cmd = CommandParser::parse(line);
        if (!cmd.is_valid) {
            continue;
        }

        if (cmd.type == CommandType::SET) {
            db_[cmd.args[0]] = cmd.args[1];
        } else if (cmd.type == CommandType::DEL) {
            db_.erase(cmd.args[0]);
        } else if (cmd.type == CommandType::ZADD) {
            try {
                double score = std::stod(cmd.args[1]);
                leaderboards_[cmd.args[0]].insert(cmd.args[2], score);
            } catch (...) {
                // Ignore corrupt entry
            }
        }
    }
}

void DatabaseEngine::set(const std::string& key, const std::string& value) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    db_[key] = value;
    if (wal_stream_.is_open()) {
        wal_stream_ << "SET " << escape_string(key) << " " << escape_string(value) << "\n";
        wal_stream_.flush();
    }
}

std::optional<std::string> DatabaseEngine::get(const std::string& key) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    auto it = db_.find(key);
    if (it != db_.end()) {
        return it->second;
    }
    return std::nullopt;
}

bool DatabaseEngine::del(const std::string& key) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    size_t count = db_.erase(key);
    leaderboards_.erase(key);
    if (count > 0 && wal_stream_.is_open()) {
        wal_stream_ << "DEL " << escape_string(key) << "\n";
        wal_stream_.flush();
    }
    return count > 0;
}

bool DatabaseEngine::exists(const std::string& key) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return db_.find(key) != db_.end() || leaderboards_.find(key) != leaderboards_.end();
}

bool DatabaseEngine::zadd(const std::string& key, double score, const std::string& member) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    bool added = leaderboards_[key].insert(member, score);
    if (wal_stream_.is_open()) {
        wal_stream_ << "ZADD " << escape_string(key) << " " << score << " " << escape_string(member) << "\n";
        wal_stream_.flush();
    }
    return added;
}

std::optional<double> DatabaseEngine::zscore(const std::string& key, const std::string& member) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    auto it = leaderboards_.find(key);
    if (it != leaderboards_.end()) {
        return it->second.get_score(member);
    }
    return std::nullopt;
}

std::vector<std::pair<std::string, double>> DatabaseEngine::zrange(const std::string& key, double min_score, double max_score) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    auto it = leaderboards_.find(key);
    if (it != leaderboards_.end()) {
        return it->second.range(min_score, max_score);
    }
    return {};
}

std::string DatabaseEngine::execute(const Command& cmd) {
    if (!cmd.is_valid) {
        if (!cmd.error_message.empty()) {
            return "(error) " + cmd.error_message;
        }
        return "(error) ERR invalid command";
    }

    switch (cmd.type) {
        case CommandType::PING:
            if (cmd.args.empty()) {
                return "PONG";
            }
            return "\"" + cmd.args[0] + "\"";

        case CommandType::SET:
            set(cmd.args[0], cmd.args[1]);
            return "OK";

        case CommandType::GET: {
            auto val = get(cmd.args[0]);
            if (val.has_value()) {
                return "\"" + *val + "\"";
            }
            return "(nil)";
        }

        case CommandType::DEL: {
            bool removed = del(cmd.args[0]);
            return "(integer) " + std::to_string(removed ? 1 : 0);
        }

        case CommandType::EXISTS: {
            bool found = exists(cmd.args[0]);
            return "(integer) " + std::to_string(found ? 1 : 0);
        }

        case CommandType::SET_FEN: {
            std::string compressed = FenCompressor::compress(cmd.args[1]);
            if (compressed.empty()) {
                return "(error) ERR invalid FEN string";
            }
            set(cmd.args[0], compressed);
            return "OK";
        }

        case CommandType::GET_FEN: {
            auto val = get(cmd.args[0]);
            if (!val.has_value()) {
                return "(nil)";
            }
            std::string decompressed = FenCompressor::decompress(*val);
            if (decompressed.empty()) {
                return "(error) ERR stored data is not a valid compressed FEN";
            }
            return "\"" + decompressed + "\"";
        }

        case CommandType::ZADD: {
            double score = std::stod(cmd.args[1]);
            bool added = zadd(cmd.args[0], score, cmd.args[2]);
            return "(integer) " + std::to_string(added ? 1 : 0);
        }

        case CommandType::ZSCORE: {
            auto score = zscore(cmd.args[0], cmd.args[1]);
            if (!score.has_value()) {
                return "(nil)";
            }
            std::ostringstream oss;
            oss << *score;
            return "\"" + oss.str() + "\"";
        }

        case CommandType::ZRANGE: {
            double min_s = std::stod(cmd.args[1]);
            double max_s = std::stod(cmd.args[2]);
            auto results = zrange(cmd.args[0], min_s, max_s);
            if (results.empty()) {
                return "(empty list or set)";
            }
            std::string output;
            for (size_t i = 0; i < results.size(); ++i) {
                std::ostringstream oss;
                oss << (i + 1) << ") \"" << results[i].first << "\" (score: " << results[i].second << ")";
                output += oss.str();
                if (i + 1 < results.size()) {
                    output += "\n";
                }
            }
            return output;
        }

        case CommandType::HELP:
        case CommandType::EXIT:
        case CommandType::UNKNOWN:
        default:
            return "";
    }
}

} // namespace mini_redis

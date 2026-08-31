#include "mini_redis/parser.hpp"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>
#include <vector>

namespace mini_redis {

std::string CommandParser::to_upper(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    return str;
}

std::vector<std::string> CommandParser::tokenize(const std::string& line, std::string& error_out) {
    error_out.clear();
    std::vector<std::string> tokens;
    std::string current_token;

    enum State {
        OUTSIDE_TOKEN,
        IN_UNQUOTED_TOKEN,
        IN_QUOTED_TOKEN,
        ESCAPE_SEQUENCE
    };

    State state = OUTSIDE_TOKEN;

    for (size_t i = 0; i < line.length(); ++i) {
        char c = line[i];

        switch (state) {
            case OUTSIDE_TOKEN:
                if (std::isspace(static_cast<unsigned char>(c))) {
                    continue;
                } else if (c == '"') {
                    state = IN_QUOTED_TOKEN;
                } else {
                    current_token.push_back(c);
                    state = IN_UNQUOTED_TOKEN;
                }
                break;

            case IN_UNQUOTED_TOKEN:
                if (std::isspace(static_cast<unsigned char>(c))) {
                    tokens.push_back(current_token);
                    current_token.clear();
                    state = OUTSIDE_TOKEN;
                } else if (c == '"') {
                    // Opening quote in middle of unquoted string: switch to quoted state
                    state = IN_QUOTED_TOKEN;
                } else {
                    current_token.push_back(c);
                }
                break;

            case IN_QUOTED_TOKEN:
                if (c == '"') {
                    // Closed quote
                    tokens.push_back(current_token);
                    current_token.clear();
                    state = OUTSIDE_TOKEN;
                } else if (c == '\\') {
                    state = ESCAPE_SEQUENCE;
                } else {
                    current_token.push_back(c);
                }
                break;

            case ESCAPE_SEQUENCE:
                if (c == '"' || c == '\\') {
                    current_token.push_back(c);
                } else if (c == 'n') {
                    current_token.push_back('\n');
                } else if (c == 't') {
                    current_token.push_back('\t');
                } else if (c == 'r') {
                    current_token.push_back('\r');
                } else {
                    // Unknown escape sequence: retain literal backslash and character
                    current_token.push_back('\\');
                    current_token.push_back(c);
                }
                state = IN_QUOTED_TOKEN;
                break;
        }
    }

    if (state == IN_QUOTED_TOKEN || state == ESCAPE_SEQUENCE) {
        error_out = "PARSE_ERROR: Unclosed quoted string";
        return {};
    }

    if (state == IN_UNQUOTED_TOKEN && !current_token.empty()) {
        tokens.push_back(current_token);
    }

    return tokens;
}

CommandType CommandParser::parse_command_type(const std::string& token) {
    std::string upper = to_upper(token);
    if (upper == "PING") return CommandType::PING;
    if (upper == "SET") return CommandType::SET;
    if (upper == "GET") return CommandType::GET;
    if (upper == "DEL") return CommandType::DEL;
    if (upper == "EXISTS") return CommandType::EXISTS;
    if (upper == "SET_FEN") return CommandType::SET_FEN;
    if (upper == "GET_FEN") return CommandType::GET_FEN;
    if (upper == "ZADD") return CommandType::ZADD;
    if (upper == "ZSCORE") return CommandType::ZSCORE;
    if (upper == "ZRANGE") return CommandType::ZRANGE;
    if (upper == "HELP") return CommandType::HELP;
    if (upper == "EXIT" || upper == "QUIT") return CommandType::EXIT;
    return CommandType::UNKNOWN;
}

bool CommandParser::validate_command(Command& cmd) {
    if (cmd.type == CommandType::UNKNOWN) {
        cmd.is_valid = false;
        cmd.error_message = "ERR unknown command '" + cmd.raw_command_name + "'";
        return false;
    }

    size_t num_args = cmd.args.size();

    switch (cmd.type) {
        case CommandType::PING:
            if (num_args > 1) {
                cmd.is_valid = false;
                cmd.error_message = "ERR wrong number of arguments for 'PING' command";
                return false;
            }
            break;

        case CommandType::SET:
            if (num_args != 2) {
                cmd.is_valid = false;
                cmd.error_message = "ERR wrong number of arguments for 'SET' command (usage: SET <key> <value>)";
                return false;
            }
            break;

        case CommandType::GET:
            if (num_args != 1) {
                cmd.is_valid = false;
                cmd.error_message = "ERR wrong number of arguments for 'GET' command (usage: GET <key>)";
                return false;
            }
            break;

        case CommandType::DEL:
            if (num_args != 1) {
                cmd.is_valid = false;
                cmd.error_message = "ERR wrong number of arguments for 'DEL' command (usage: DEL <key>)";
                return false;
            }
            break;

        case CommandType::EXISTS:
            if (num_args != 1) {
                cmd.is_valid = false;
                cmd.error_message = "ERR wrong number of arguments for 'EXISTS' command (usage: EXISTS <key>)";
                return false;
            }
            break;

        case CommandType::SET_FEN:
            if (num_args < 2) {
                cmd.is_valid = false;
                cmd.error_message = "ERR wrong number of arguments for 'SET_FEN' command (usage: SET_FEN <key> <fen_string>)";
                return false;
            }
            if (num_args > 2) {
                std::string fen;
                for (size_t i = 1; i < num_args; ++i) {
                    fen += cmd.args[i];
                    if (i + 1 < num_args) {
                        fen += " ";
                    }
                }
                cmd.args.resize(2);
                cmd.args[1] = fen;
            }
            break;

        case CommandType::GET_FEN:
            if (num_args != 1) {
                cmd.is_valid = false;
                cmd.error_message = "ERR wrong number of arguments for 'GET_FEN' command (usage: GET_FEN <key>)";
                return false;
            }
            break;

        case CommandType::ZADD:
            if (num_args != 3) {
                cmd.is_valid = false;
                cmd.error_message = "ERR wrong number of arguments for 'ZADD' command (usage: ZADD <key> <score> <member>)";
                return false;
            }
            try {
                size_t idx = 0;
                std::stod(cmd.args[1], &idx);
                if (idx != cmd.args[1].length()) {
                    cmd.is_valid = false;
                    cmd.error_message = "ERR value is not a valid float";
                    return false;
                }
            } catch (...) {
                cmd.is_valid = false;
                cmd.error_message = "ERR value is not a valid float";
                return false;
            }
            break;

        case CommandType::ZSCORE:
            if (num_args != 2) {
                cmd.is_valid = false;
                cmd.error_message = "ERR wrong number of arguments for 'ZSCORE' command (usage: ZSCORE <key> <member>)";
                return false;
            }
            break;

        case CommandType::ZRANGE:
            if (num_args != 3) {
                cmd.is_valid = false;
                cmd.error_message = "ERR wrong number of arguments for 'ZRANGE' command (usage: ZRANGE <key> <min_score> <max_score>)";
                return false;
            }
            try {
                size_t idx1 = 0;
                std::stod(cmd.args[1], &idx1);
                if (idx1 != cmd.args[1].length()) {
                    cmd.is_valid = false;
                    cmd.error_message = "ERR value is not a valid float";
                    return false;
                }
                size_t idx2 = 0;
                std::stod(cmd.args[2], &idx2);
                if (idx2 != cmd.args[2].length()) {
                    cmd.is_valid = false;
                    cmd.error_message = "ERR value is not a valid float";
                    return false;
                }
            } catch (...) {
                cmd.is_valid = false;
                cmd.error_message = "ERR value is not a valid float";
                return false;
            }
            break;

        case CommandType::HELP:
            if (num_args != 0) {
                cmd.is_valid = false;
                cmd.error_message = "ERR wrong number of arguments for 'HELP' command";
                return false;
            }
            break;

        case CommandType::EXIT:
            if (num_args != 0) {
                cmd.is_valid = false;
                cmd.error_message = "ERR wrong number of arguments for 'EXIT' command";
                return false;
            }
            break;

        case CommandType::UNKNOWN:
        default:
            break;
    }

    cmd.is_valid = true;
    cmd.error_message.clear();
    return true;
}

Command CommandParser::parse(const std::string& line) {
    Command cmd;
    std::string tokenize_error;

    std::vector<std::string> tokens = tokenize(line, tokenize_error);

    if (!tokenize_error.empty()) {
        cmd.type = CommandType::UNKNOWN;
        cmd.is_valid = false;
        cmd.error_message = tokenize_error;
        return cmd;
    }

    if (tokens.empty()) {
        // Empty command
        cmd.is_valid = false;
        cmd.error_message = ""; // Empty input is ignored by REPL
        return cmd;
    }

    cmd.raw_command_name = tokens[0];
    cmd.type = parse_command_type(tokens[0]);
    cmd.args.assign(tokens.begin() + 1, tokens.end());

    validate_command(cmd);

    return cmd;
}

} // namespace mini_redis

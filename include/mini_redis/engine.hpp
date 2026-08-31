#ifndef MINI_REDIS_ENGINE_HPP
#define MINI_REDIS_ENGINE_HPP

#include "mini_redis/command.hpp"
#include "mini_redis/skiplist.hpp"
#include <fstream>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>

namespace mini_redis {

class DatabaseEngine {
public:
    explicit DatabaseEngine(const std::string& wal_filename = "mini_redis.wal");
    ~DatabaseEngine();

    // Non-copyable to prevent race conditions or unsafe copying of synchronization primitives
    DatabaseEngine(const DatabaseEngine&) = delete;
    DatabaseEngine& operator=(const DatabaseEngine&) = delete;

    // Write operations (std::unique_lock)
    void set(const std::string& key, const std::string& value);
    bool del(const std::string& key);

    // ZSET Write operations (std::unique_lock)
    bool zadd(const std::string& key, double score, const std::string& member);

    // Read operations (std::shared_lock)
    std::optional<std::string> get(const std::string& key) const;
    bool exists(const std::string& key) const;

    // ZSET Read operations (std::shared_lock)
    std::optional<double> zscore(const std::string& key, const std::string& member) const;
    std::vector<std::pair<std::string, double>> zrange(const std::string& key, double min_score, double max_score) const;

    // Command dispatcher: executes parsed Command struct and returns Redis-formatted string response
    std::string execute(const Command& cmd);

private:
    std::string wal_filename_;
    std::ofstream wal_stream_;
    std::unordered_map<std::string, std::string> db_;
    std::unordered_map<std::string, SkipList> leaderboards_;
    mutable std::shared_mutex mutex_;

    void replay_wal();
    static std::string escape_string(const std::string& str);
};

} // namespace mini_redis

#endif // MINI_REDIS_ENGINE_HPP

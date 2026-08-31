#ifndef MINI_REDIS_SKIPLIST_HPP
#define MINI_REDIS_SKIPLIST_HPP

#include <iostream>
#include <optional>
#include <random>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace mini_redis {

// Maximum height level allowed for a Skip List node (Redis standard is 32)
constexpr int SKIPLIST_MAX_LEVEL = 32;

// Probability factor for promoting a node to higher levels (p = 0.25)
constexpr double SKIPLIST_P = 0.25;

/**
 * @brief Represents a single node in the Skip List.
 * 
 * Each node stores a member key, a double score, and an array of forward pointers.
 * The size of the forward pointers vector determines the node's height in the Skip List.
 */
struct SkipListNode {
    std::string member;
    double score;
    std::vector<SkipListNode*> forward;

    SkipListNode(std::string m, double s, int level)
        : member(std::move(m)), score(s), forward(level, nullptr) {}
};

/**
 * @brief Probabilistic Skip List data structure for Sorted Sets (Leaderboards).
 * 
 * Maintains elements ordered by (score ASC, member ASC). Provides O(log N) average time
 * complexity for insertion, lookup, deletion, and range queries.
 */
class SkipList {
public:
    SkipList();
    ~SkipList();

    // Non-copyable to prevent pointer aliasing bugs
    SkipList(const SkipList&) = delete;
    SkipList& operator=(const SkipList&) = delete;

    // Move semantics
    SkipList(SkipList&& other) noexcept;
    SkipList& operator=(SkipList&& other) noexcept;

    /**
     * @brief Inserts or updates a member with a given score.
     * @param member The unique string key for the element.
     * @param score The numerical score for sorting.
     * @return true if a new member was added, false if an existing member's score was updated.
     */
    bool insert(const std::string& member, double score);

    /**
     * @brief Removes a member from the Skip List.
     * @param member The key of the member to remove.
     * @return true if the member existed and was removed, false otherwise.
     */
    bool remove(const std::string& member);

    /**
     * @brief Retrieves the score of a member.
     * @param member The key of the member.
     * @return std::optional<double> containing the score if present, std::nullopt otherwise.
     */
    std::optional<double> get_score(const std::string& member) const;

    /**
     * @brief Fetches all (member, score) pairs with scores within [min_score, max_score].
     * @param min_score Lower score boundary (inclusive).
     * @param max_score Upper score boundary (inclusive).
     * @return Vector of pairs sorted by score then member.
     */
    std::vector<std::pair<std::string, double>> range(double min_score, double max_score) const;

    /**
     * @brief Returns the total number of members in the Skip List.
     */
    size_t size() const { return size_; }

private:
    SkipListNode* head_;
    int level_{1};
    size_t size_{0};
    std::unordered_map<std::string, double> dict_; // O(1) dictionary lookup for member scores

    mutable std::mt19937 rng_;
    mutable std::uniform_real_distribution<double> dist_{0.0, 1.0};

    int random_level() const;
    void free_list();
    bool remove_node(const std::string& member, double score);
};

} // namespace mini_redis

#endif // MINI_REDIS_SKIPLIST_HPP

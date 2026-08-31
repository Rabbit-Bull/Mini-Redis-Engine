#include "mini_redis/skiplist.hpp"
#include <algorithm>

namespace mini_redis {

/**
 * @brief Constructs a new Skip List.
 * 
 * Initializes the sentinel head node with MAX_LEVEL forward pointers set to nullptr.
 */
SkipList::SkipList()
    : head_(new SkipListNode("", 0.0, SKIPLIST_MAX_LEVEL)),
      rng_(std::random_device{}()) {}

/**
 * @brief Destructor frees all dynamically allocated nodes in the Skip List.
 */
SkipList::~SkipList() {
    free_list();
}

/**
 * @brief Cleans up memory by deleting nodes sequentially via level 0 pointers.
 */
void SkipList::free_list() {
    if (!head_) {
        return;
    }
    SkipListNode* curr = head_->forward[0];
    while (curr) {
        SkipListNode* next = curr->forward[0];
        delete curr;
        curr = next;
    }
    delete head_;
    head_ = nullptr;
    size_ = 0;
    level_ = 1;
    dict_.clear();
}

/**
 * @brief Move constructor for transferring ownership of a Skip List.
 */
SkipList::SkipList(SkipList&& other) noexcept
    : head_(other.head_),
      level_(other.level_),
      size_(other.size_),
      dict_(std::move(other.dict_)),
      rng_(std::move(other.rng_)),
      dist_(std::move(other.dist_)) {
    other.head_ = nullptr;
    other.size_ = 0;
    other.level_ = 1;
}

/**
 * @brief Move assignment operator for transferring ownership of a Skip List.
 */
SkipList& SkipList::operator=(SkipList&& other) noexcept {
    if (this != &other) {
        free_list();
        head_ = other.head_;
        level_ = other.level_;
        size_ = other.size_;
        dict_ = std::move(other.dict_);
        rng_ = std::move(other.rng_);
        dist_ = std::move(other.dist_);

        other.head_ = nullptr;
        other.size_ = 0;
        other.level_ = 1;
    }
    return *this;
}

/**
 * @brief Generates a random height level for a new node.
 * 
 * Uses geometric distribution where each level has a probability p = 0.25 of promotion.
 */
int SkipList::random_level() const {
    int lvl = 1;
    while (dist_(rng_) < SKIPLIST_P && lvl < SKIPLIST_MAX_LEVEL) {
        lvl++;
    }
    return lvl;
}

/**
 * @brief Helper method to remove a node given its member key and score.
 */
bool SkipList::remove_node(const std::string& member, double score) {
    std::vector<SkipListNode*> update(SKIPLIST_MAX_LEVEL, nullptr);
    SkipListNode* curr = head_;

    // Traverse levels top-down to find predecessors at each level
    for (int i = level_ - 1; i >= 0; --i) {
        while (curr->forward[i] &&
               (curr->forward[i]->score < score ||
                (curr->forward[i]->score == score && curr->forward[i]->member < member))) {
            curr = curr->forward[i];
        }
        update[i] = curr;
    }

    // Target candidate is at level 0
    curr = curr->forward[0];

    // Check if candidate matches exact member and score
    if (curr && curr->score == score && curr->member == member) {
        // Unlink node from all levels where it participates
        for (int i = 0; i < level_; ++i) {
            if (update[i]->forward[i] != curr) {
                break;
            }
            update[i]->forward[i] = curr->forward[i];
        }

        // Adjust overall list level if top levels are now empty
        while (level_ > 1 && head_->forward[level_ - 1] == nullptr) {
            level_--;
        }

        delete curr;
        dict_.erase(member);
        size_--;
        return true;
    }

    return false;
}

/**
 * @brief Inserts or updates a member key with a score in O(log N) average time.
 */
bool SkipList::insert(const std::string& member, double score) {
    bool is_update = false;

    // Check if member already exists in dictionary
    auto it = dict_.find(member);
    if (it != dict_.end()) {
        double old_score = it->second;
        if (old_score == score) {
            return false; // No change in score
        }
        // Remove existing node from skip list structure before re-inserting
        remove_node(member, old_score);
        is_update = true;
    }

    // Generate random level for the new node
    int new_level = random_level();

    // If new level exceeds current list height, extend update pointers to head_
    std::vector<SkipListNode*> update(SKIPLIST_MAX_LEVEL, nullptr);
    if (new_level > level_) {
        for (int i = level_; i < new_level; ++i) {
            update[i] = head_;
        }
        level_ = new_level;
    }

    // Traverse list from top level down to locate insertion point
    SkipListNode* curr = head_;
    for (int i = level_ - 1; i >= 0; --i) {
        while (curr->forward[i] &&
               (curr->forward[i]->score < score ||
                (curr->forward[i]->score == score && curr->forward[i]->member < member))) {
            curr = curr->forward[i];
        }
        update[i] = curr;
    }

    // Create new node and link forward pointers at each level up to new_level
    SkipListNode* new_node = new SkipListNode(member, score, new_level);
    for (int i = 0; i < new_level; ++i) {
        new_node->forward[i] = update[i]->forward[i];
        update[i]->forward[i] = new_node;
    }

    // Update dictionary lookup map
    dict_[member] = score;

    if (!is_update) {
        size_++;
    }

    return !is_update;
}

/**
 * @brief Removes a member key from the Skip List.
 */
bool SkipList::remove(const std::string& member) {
    auto it = dict_.find(member);
    if (it == dict_.end()) {
        return false;
    }
    return remove_node(member, it->second);
}

/**
 * @brief O(1) score lookup using internal dictionary map.
 */
std::optional<double> SkipList::get_score(const std::string& member) const {
    auto it = dict_.find(member);
    if (it != dict_.end()) {
        return it->second;
    }
    return std::nullopt;
}

/**
 * @brief Returns all (member, score) pairs with min_score <= score <= max_score.
 */
std::vector<std::pair<std::string, double>> SkipList::range(double min_score, double max_score) const {
    std::vector<std::pair<std::string, double>> result;

    if (min_score > max_score || size_ == 0) {
        return result;
    }

    // Locate the first node with score >= min_score
    SkipListNode* curr = head_;
    for (int i = level_ - 1; i >= 0; --i) {
        while (curr->forward[i] && curr->forward[i]->score < min_score) {
            curr = curr->forward[i];
        }
    }

    // Advance to level 0 candidate
    curr = curr->forward[0];

    // Traverse level 0 forward pointers while score <= max_score
    while (curr && curr->score <= max_score) {
        if (curr->score >= min_score) {
            result.push_back({curr->member, curr->score});
        }
        curr = curr->forward[0];
    }

    return result;
}

} // namespace mini_redis

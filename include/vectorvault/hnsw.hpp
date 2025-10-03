#pragma once

#include <memory>
#include <random>
#include <shared_mutex>
#include <span>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "distance.hpp"

namespace vectorvault {

struct HNSWParams {
    int M = 16;                 // Number of bi-directional links per node
    int ef_construction = 200;  // Size of dynamic candidate list during construction
    int max_M = 16;             // Maximum M value
    int max_M0 = 32;            // Maximum M value for layer 0
    uint64_t seed = 42;         // Random seed for level generation
    DistanceMetric metric = DistanceMetric::L2;
};

class HNSWIndex {
   public:
    HNSWIndex(int dim, const HNSWParams& params = HNSWParams{});
    ~HNSWIndex();

    // Add a vector with given ID
    void add(int id, std::span<const float> vec);

    // Search for k nearest neighbors
    struct SearchResult {
        int id;
        float distance;

        bool operator<(const SearchResult& other) const { return distance < other.distance; }
    };

    std::vector<SearchResult> search(std::span<const float> query, int k, int ef_search = 50) const;

    // Reserve space for n vectors
    void reserve(size_t n);

    // Persistence
    bool save(const std::string& path) const;
    bool load(const std::string& path);

    // Stats
    int dimension() const { return dim_; }
    size_t size() const { return nodes_.size(); }
    int max_level() const { return max_level_; }
    const HNSWParams& params() const { return params_; }

   private:
    struct Node {
        int id;
        int level;
        std::vector<float> vector;
        std::vector<std::vector<int>> neighbors;  // neighbors[layer] = list of neighbor IDs
    };

    // Generate random level for new node
    int random_level();

    // Search for ef nearest neighbors in a single layer
    std::vector<SearchResult> search_layer(std::span<const float> query, int entry_point, int ef,
                                           int layer) const;

    // Select M neighbors using heuristic
    std::vector<int> select_neighbors_heuristic(std::span<const float> base_vec,
                                                const std::vector<SearchResult>& candidates,
                                                int M) const;

    // Connect new node to the graph
    void connect_node(int node_id, int level);

    // Prune connections to maintain max_M constraint
    void prune_connections(int node_id, int level);

    float distance(std::span<const float> a, std::span<const float> b) const {
        return dist_calc_(a.data(), b.data(), dim_);
    }

    // Index parameters
    int dim_;
    HNSWParams params_;
    DistanceCalculator dist_calc_;

    // Graph structure
    std::vector<std::unique_ptr<Node>> nodes_;     // All nodes
    std::unordered_map<int, size_t> id_to_index_;  // ID -> index in nodes_
    int entry_point_;                              // Entry point node index (-1 if empty)
    int max_level_;                                // Current maximum level

    // Random number generation
    mutable std::mt19937_64 rng_;
    mutable std::uniform_real_distribution<double> level_dist_;

    // Thread safety
    mutable std::shared_mutex mutex_;  // Protects graph structure

    // Thread-local visited sets for search
    struct VisitedSet {
        std::unordered_set<int> set;
        uint64_t generation = 0;
    };
    mutable std::vector<VisitedSet> visited_sets_;
};

}  // namespace vectorvault

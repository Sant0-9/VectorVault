#include <algorithm>
#include <queue>
#include <unordered_set>
#include <vector>

#include "vectorvault/distance.hpp"

namespace vectorvault::bench {

struct BruteForceResult {
    int id;
    float distance;

    bool operator<(const BruteForceResult& other) const { return distance < other.distance; }
};

std::vector<BruteForceResult> brute_force_search(const std::vector<float>& query,
                                                 const std::vector<std::vector<float>>& database,
                                                 int k,
                                                 DistanceMetric metric = DistanceMetric::L2) {
    if (k <= 0) {
        return {};
    }

    DistanceCalculator dist_calc(metric);

    std::vector<BruteForceResult> all_results;
    all_results.reserve(database.size());

    for (size_t i = 0; i < database.size(); ++i) {
        float dist = dist_calc(query, database[i]);
        all_results.push_back({static_cast<int>(i), dist});
    }

    // Sort by distance
    std::sort(all_results.begin(), all_results.end());

    // Return top k
    if (all_results.size() > static_cast<size_t>(k)) {
        all_results.resize(static_cast<size_t>(k));
    }

    return all_results;
}

float compute_recall(const std::vector<int>& ground_truth, const std::vector<int>& results, int k) {
    if (k <= 0) {
        return 0.0f;
    }

    size_t ground_truth_limit = std::min(static_cast<size_t>(k), ground_truth.size());
    std::unordered_set<int> gt_set;
    gt_set.reserve(ground_truth_limit);
    for (size_t i = 0; i < ground_truth_limit; ++i) {
        gt_set.insert(ground_truth[i]);
    }

    size_t matches = 0;
    size_t results_limit = std::min(static_cast<size_t>(k), results.size());
    for (size_t i = 0; i < results_limit; ++i) {
        if (gt_set.find(results[i]) != gt_set.end()) {
            ++matches;
        }
    }

    return static_cast<float>(matches) / static_cast<float>(k);
}

}  // namespace vectorvault::bench

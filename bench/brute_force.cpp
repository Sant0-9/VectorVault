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
    if (static_cast<int>(all_results.size()) > k) {
        all_results.resize(k);
    }

    return all_results;
}

float compute_recall(const std::vector<int>& ground_truth, const std::vector<int>& results, int k) {
    std::unordered_set<int> gt_set(
        ground_truth.begin(),
        ground_truth.begin() + std::min(k, static_cast<int>(ground_truth.size())));

    int matches = 0;
    for (int i = 0; i < std::min(k, static_cast<int>(results.size())); ++i) {
        if (gt_set.find(results[i]) != gt_set.end()) {
            ++matches;
        }
    }

    return static_cast<float>(matches) / static_cast<float>(k);
}

}  // namespace vectorvault::bench

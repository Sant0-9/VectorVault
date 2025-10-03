#include <gtest/gtest.h>

#include <algorithm>
#include <random>

#include "vectorvault/hnsw.hpp"

using namespace vectorvault;

class HNSWSmallTest : public ::testing::Test {
   protected:
    void SetUp() override {
        // Generate small synthetic dataset
        std::mt19937 rng(42);
        std::normal_distribution<float> dist(0.0f, 1.0f);

        N = 200;
        dim = 32;

        vectors.resize(N);
        for (int i = 0; i < N; ++i) {
            vectors[i].resize(dim);
            for (int j = 0; j < dim; ++j) {
                vectors[i][j] = dist(rng);
            }
        }
    }

    std::vector<std::vector<float>> brute_force_knn(const std::vector<float>& query, int k) {
        DistanceCalculator calc(DistanceMetric::L2);

        std::vector<std::pair<float, int>> distances;
        for (size_t i = 0; i < vectors.size(); ++i) {
            float dist = calc(query, vectors[i]);
            distances.push_back({dist, static_cast<int>(i)});
        }

        std::sort(distances.begin(), distances.end());

        std::vector<std::vector<float>> results;
        for (int i = 0; i < k && i < static_cast<int>(distances.size()); ++i) {
            results.push_back(vectors[distances[i].second]);
        }

        return results;
    }

    int N;
    int dim;
    std::vector<std::vector<float>> vectors;
};

TEST_F(HNSWSmallTest, BasicAddAndSearch) {
    HNSWParams params;
    params.M = 8;
    params.ef_construction = 100;

    HNSWIndex index(dim, params);

    // Add vectors
    for (int i = 0; i < N; ++i) {
        index.add(i, vectors[i]);
    }

    EXPECT_EQ(index.size(), static_cast<size_t>(N));
    EXPECT_EQ(index.dimension(), dim);
}

TEST_F(HNSWSmallTest, SearchAccuracy) {
    HNSWParams params;
    params.M = 16;
    params.ef_construction = 200;

    HNSWIndex index(dim, params);

    // Build index
    for (int i = 0; i < N; ++i) {
        index.add(i, vectors[i]);
    }

    // Test queries
    std::mt19937 rng(1337);
    std::uniform_int_distribution<int> dist(0, N - 1);

    int num_queries = 20;
    int k = 5;
    float total_recall = 0.0f;

    for (int q = 0; q < num_queries; ++q) {
        int query_idx = dist(rng);
        const auto& query = vectors[query_idx];

        // HNSW search
        auto results = index.search(query, k, 50);

        // Brute force ground truth
        DistanceCalculator calc(DistanceMetric::L2);
        std::vector<std::pair<float, int>> ground_truth;
        for (int i = 0; i < N; ++i) {
            float d = calc(query, vectors[i]);
            ground_truth.push_back({d, i});
        }
        std::sort(ground_truth.begin(), ground_truth.end());

        // Compute recall
        std::unordered_set<int> gt_set;
        for (int i = 0; i < k; ++i) {
            gt_set.insert(ground_truth[i].second);
        }

        int matches = 0;
        for (const auto& result : results) {
            if (gt_set.find(result.id) != gt_set.end()) {
                ++matches;
            }
        }

        float recall = static_cast<float>(matches) / static_cast<float>(k);
        total_recall += recall;
    }

    float avg_recall = total_recall / num_queries;
    EXPECT_GE(avg_recall, 0.95f) << "Average recall: " << avg_recall;
}

TEST_F(HNSWSmallTest, ExactMatchQuery) {
    HNSWParams params;
    params.ef_construction = 200;
    HNSWIndex index(dim, params);

    // Add vectors
    for (int i = 0; i < N; ++i) {
        index.add(i, vectors[i]);
    }

    // Query with exact vector from index - should be in top results
    int test_id = 42;
    auto results = index.search(vectors[test_id], 10, 100);  // Increased ef for better recall

    ASSERT_GE(results.size(), 1u);

    // Check if test_id is in top 10 results (HNSW is approximate)
    bool found = false;
    for (const auto& r : results) {
        if (r.id == test_id) {
            found = true;
            EXPECT_NEAR(r.distance, 0.0f, 1e-4f);  // Self-distance should be ~0
            break;
        }
    }
    EXPECT_TRUE(found) << "Exact match not found in top 10 results";
}

TEST_F(HNSWSmallTest, DuplicateIDThrows) {
    HNSWParams params;
    HNSWIndex index(dim, params);

    index.add(0, vectors[0]);

    EXPECT_THROW(index.add(0, vectors[1]), std::invalid_argument);
}

TEST_F(HNSWSmallTest, DimensionMismatchThrows) {
    HNSWParams params;
    HNSWIndex index(dim, params);

    std::vector<float> wrong_dim(dim + 10, 1.0f);
    EXPECT_THROW(index.add(0, wrong_dim), std::invalid_argument);
}

TEST_F(HNSWSmallTest, EmptyIndexSearch) {
    HNSWParams params;
    HNSWIndex index(dim, params);

    auto results = index.search(vectors[0], 10, 50);
    EXPECT_TRUE(results.empty());
}

TEST_F(HNSWSmallTest, CosineMetric) {
    HNSWParams params;
    params.metric = DistanceMetric::COSINE;

    HNSWIndex index(dim, params);

    for (int i = 0; i < N; ++i) {
        index.add(i, vectors[i]);
    }

    auto results = index.search(vectors[0], 5, 50);
    EXPECT_GE(results.size(), 1u);
    EXPECT_EQ(results[0].id, 0);
}

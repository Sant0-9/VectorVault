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

        vectors.resize(static_cast<size_t>(N));
        for (size_t i = 0; i < static_cast<size_t>(N); ++i) {
            vectors[i].resize(static_cast<size_t>(dim));
            for (size_t j = 0; j < static_cast<size_t>(dim); ++j) {
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
        for (size_t i = 0; i < static_cast<size_t>(k) && i < distances.size(); ++i) {
            results.push_back(vectors[static_cast<size_t>(distances[i].second)]);
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
        index.add(i, vectors[static_cast<size_t>(i)]);
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
        index.add(i, vectors[static_cast<size_t>(i)]);
    }

    // Test queries
    std::mt19937 rng(1337);
    std::uniform_int_distribution<int> dist(0, N - 1);

    int num_queries = 20;
    int k = 5;
    float total_recall = 0.0f;

    for (int q = 0; q < num_queries; ++q) {
        int query_idx = dist(rng);
        const auto& query = vectors[static_cast<size_t>(query_idx)];

        // HNSW search
        auto results = index.search(query, k, 50);

        // Brute force ground truth
        DistanceCalculator calc(DistanceMetric::L2);
        std::vector<std::pair<float, int>> ground_truth;
        for (int i = 0; i < N; ++i) {
            float d = calc(query, vectors[static_cast<size_t>(i)]);
            ground_truth.push_back({d, i});
        }
        std::sort(ground_truth.begin(), ground_truth.end());

        // Compute recall
        std::unordered_set<int> gt_set;
        for (int i = 0; i < k; ++i) {
            gt_set.insert(ground_truth[static_cast<size_t>(i)].second);
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

    float avg_recall = total_recall / static_cast<float>(num_queries);
    EXPECT_GE(avg_recall, 0.95f) << "Average recall: " << avg_recall;
}

TEST_F(HNSWSmallTest, SelfQueryAccuracy) {
    // Test that querying with vectors from the index finds them in top-k results
    // HNSW is approximate, so we test across multiple queries for robustness
    HNSWParams params;
    params.M = 16;
    params.ef_construction = 200;
    HNSWIndex index(dim, params);

    // Add vectors
    for (int i = 0; i < N; ++i) {
        index.add(i, vectors[static_cast<size_t>(i)]);
    }

    // Test multiple self-queries
    int found_count = 0;
    std::vector<int> test_ids = {0, 10, 42, 100, 150};

    for (int test_id : test_ids) {
        auto results = index.search(vectors[static_cast<size_t>(test_id)], 10, 200);
        ASSERT_GE(results.size(), 1u);

        // Check if test_id is in top 10 results
        for (const auto& r : results) {
            if (r.id == test_id && r.distance < 1e-3f) {
                found_count++;
                break;
            }
        }
    }

    // HNSW should find most self-queries in top-k (at least 80%)
    EXPECT_GE(found_count, 4) << "Found " << found_count << " out of " << test_ids.size()
                              << " self-queries";
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

    std::vector<float> wrong_dim(static_cast<size_t>(dim + 10), 1.0f);
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
        index.add(i, vectors[static_cast<size_t>(i)]);
    }

    auto results = index.search(vectors[0], 5, 50);
    EXPECT_GE(results.size(), 1u);
    EXPECT_EQ(results[0].id, 0);
}

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <random>

#include "vectorvault/hnsw.hpp"

using namespace vectorvault;

class PersistenceTest : public ::testing::Test {
   protected:
    void SetUp() override {
        std::mt19937 rng(42);
        std::normal_distribution<float> dist(0.0f, 1.0f);

        N = 100;
        dim = 64;

        vectors.resize(N);
        for (int i = 0; i < N; ++i) {
            vectors[i].resize(dim);
            for (int j = 0; j < dim; ++j) {
                vectors[i][j] = dist(rng);
            }
        }

        test_index_path = "test_index.vv";
    }

    void TearDown() override {
        // Clean up test file
        if (std::filesystem::exists(test_index_path)) {
            std::filesystem::remove(test_index_path);
        }
    }

    int N;
    int dim;
    std::vector<std::vector<float>> vectors;
    std::string test_index_path;
};

TEST_F(PersistenceTest, SaveAndLoad) {
    HNSWParams params;
    params.M = 8;
    params.ef_construction = 100;

    // Build and save index
    HNSWIndex index1(dim, params);
    for (int i = 0; i < N; ++i) {
        index1.add(i, vectors[i]);
    }

    ASSERT_TRUE(index1.save(test_index_path));

    // Load index
    HNSWIndex index2(dim, params);
    ASSERT_TRUE(index2.load(test_index_path));

    // Verify metadata
    EXPECT_EQ(index2.size(), index1.size());
    EXPECT_EQ(index2.dimension(), index1.dimension());
    EXPECT_EQ(index2.max_level(), index1.max_level());
}

TEST_F(PersistenceTest, IdenticalSearchResults) {
    HNSWParams params;
    params.M = 16;
    params.ef_construction = 200;
    params.seed = 12345;  // Fixed seed for determinism

    // Build original index
    HNSWIndex index1(dim, params);
    for (int i = 0; i < N; ++i) {
        index1.add(i, vectors[i]);
    }

    // Save and load
    ASSERT_TRUE(index1.save(test_index_path));

    HNSWIndex index2(dim, params);
    ASSERT_TRUE(index2.load(test_index_path));

    // Compare search results for multiple queries
    std::mt19937 rng(1337);
    std::normal_distribution<float> dist(0.0f, 1.0f);

    for (int q = 0; q < 10; ++q) {
        std::vector<float> query(dim);
        for (int i = 0; i < dim; ++i) {
            query[i] = dist(rng);
        }

        auto results1 = index1.search(query, 10, 50);
        auto results2 = index2.search(query, 10, 50);

        ASSERT_EQ(results1.size(), results2.size()) << "Query " << q;

        for (size_t i = 0; i < results1.size(); ++i) {
            EXPECT_EQ(results1[i].id, results2[i].id) << "Query " << q << ", result " << i;
            EXPECT_NEAR(results1[i].distance, results2[i].distance, 1e-5f)
                << "Query " << q << ", result " << i;
        }
    }
}

TEST_F(PersistenceTest, DeterministicTopKResults) {
    // This test ensures that save/load maintains exact query result ordering
    HNSWParams params;
    params.M = 16;
    params.ef_construction = 200;
    params.seed = 42;

    // Build index with fixed data
    HNSWIndex index(dim, params);
    for (int i = 0; i < N; ++i) {
        index.add(i, vectors[i]);
    }

    // Create fixed query set
    std::mt19937 query_rng(9999);
    std::normal_distribution<float> query_dist(0.0f, 1.0f);

    std::vector<std::vector<float>> queries(5);
    for (auto& q : queries) {
        q.resize(dim);
        for (int j = 0; j < dim; ++j) {
            q[j] = query_dist(query_rng);
        }
    }

    // Get results before save
    std::vector<std::vector<std::pair<int, float>>> results_before;
    for (const auto& q : queries) {
        auto res = index.search(q, 10, 100);
        std::vector<std::pair<int, float>> query_results;
        for (const auto& r : res) {
            query_results.emplace_back(r.id, r.distance);
        }
        results_before.push_back(query_results);
    }

    // Save and load
    ASSERT_TRUE(index.save(test_index_path));

    HNSWIndex loaded_index(dim, params);
    ASSERT_TRUE(loaded_index.load(test_index_path));

    // Get results after load
    std::vector<std::vector<std::pair<int, float>>> results_after;
    for (const auto& q : queries) {
        auto res = loaded_index.search(q, 10, 100);
        std::vector<std::pair<int, float>> query_results;
        for (const auto& r : res) {
            query_results.emplace_back(r.id, r.distance);
        }
        results_after.push_back(query_results);
    }

    // Verify identical results
    ASSERT_EQ(results_before.size(), results_after.size());

    for (size_t q = 0; q < queries.size(); ++q) {
        ASSERT_EQ(results_before[q].size(), results_after[q].size())
            << "Query " << q << " returned different number of results";

        for (size_t i = 0; i < results_before[q].size(); ++i) {
            EXPECT_EQ(results_before[q][i].first, results_after[q][i].first)
                << "Query " << q << ", position " << i << ": ID mismatch";
            EXPECT_FLOAT_EQ(results_before[q][i].second, results_after[q][i].second)
                << "Query " << q << ", position " << i << ": Distance mismatch";
        }
    }
}

TEST_F(PersistenceTest, LoadNonexistentFile) {
    HNSWParams params;
    HNSWIndex index(dim, params);

    EXPECT_FALSE(index.load("nonexistent_file.vv"));
}

TEST_F(PersistenceTest, LoadCorruptedFile) {
    // Create a corrupted file
    std::ofstream file(test_index_path, std::ios::binary);
    file.write("CORRUPT DATA", 12);
    file.close();

    HNSWParams params;
    HNSWIndex index(dim, params);

    EXPECT_FALSE(index.load(test_index_path));
}

TEST_F(PersistenceTest, SaveEmptyIndex) {
    HNSWParams params;
    HNSWIndex index(dim, params);

    ASSERT_TRUE(index.save(test_index_path));

    HNSWIndex index2(dim, params);
    ASSERT_TRUE(index2.load(test_index_path));

    EXPECT_EQ(index2.size(), 0u);
}

TEST_F(PersistenceTest, MultipleCosineSaveLoad) {
    HNSWParams params;
    params.metric = DistanceMetric::COSINE;

    HNSWIndex index1(dim, params);
    for (int i = 0; i < N; ++i) {
        index1.add(i, vectors[i]);
    }

    ASSERT_TRUE(index1.save(test_index_path));

    HNSWIndex index2(dim, params);
    ASSERT_TRUE(index2.load(test_index_path));

    // Verify metric was preserved
    EXPECT_EQ(index2.params().metric, DistanceMetric::COSINE);

    // Verify search works
    auto results = index2.search(vectors[0], 5, 50);
    EXPECT_GE(results.size(), 1u);
}

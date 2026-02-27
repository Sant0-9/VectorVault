#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <iterator>
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

        vectors.resize(static_cast<size_t>(N));
        for (size_t i = 0; i < static_cast<size_t>(N); ++i) {
            vectors[i].resize(static_cast<size_t>(dim));
            for (size_t j = 0; j < static_cast<size_t>(dim); ++j) {
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
        index1.add(i, vectors[static_cast<size_t>(i)]);
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
        index1.add(i, vectors[static_cast<size_t>(i)]);
    }

    // Save and load
    ASSERT_TRUE(index1.save(test_index_path));

    HNSWIndex index2(dim, params);
    ASSERT_TRUE(index2.load(test_index_path));

    // Compare search results for multiple queries
    std::mt19937 rng(1337);
    std::normal_distribution<float> dist(0.0f, 1.0f);

    for (int q = 0; q < 10; ++q) {
        std::vector<float> query(static_cast<size_t>(dim));
        for (int i = 0; i < dim; ++i) {
            query[static_cast<size_t>(i)] = dist(rng);
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
        index.add(i, vectors[static_cast<size_t>(i)]);
    }

    // Create fixed query set
    std::mt19937 query_rng(9999);
    std::normal_distribution<float> query_dist(0.0f, 1.0f);

    std::vector<std::vector<float>> queries(5);
    for (auto& q : queries) {
        q.resize(static_cast<size_t>(dim));
        for (int j = 0; j < dim; ++j) {
            q[static_cast<size_t>(j)] = query_dist(query_rng);
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
        index1.add(i, vectors[static_cast<size_t>(i)]);
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

TEST_F(PersistenceTest, LoadPreservesExistingIndexOnFailure) {
    HNSWParams params;
    params.M = 8;
    params.ef_construction = 100;

    HNSWIndex baseline_index(dim, params);
    baseline_index.add(999, vectors[0]);

    auto baseline_results = baseline_index.search(vectors[0], 1, 50);
    ASSERT_EQ(baseline_results.size(), 1u);
    ASSERT_EQ(baseline_results[0].id, 999);

    HNSWIndex source_index(dim, params);
    source_index.add(1, vectors[1]);
    source_index.add(2, vectors[2]);
    ASSERT_TRUE(source_index.save(test_index_path));

    std::ifstream input(test_index_path, std::ios::binary);
    ASSERT_TRUE(input.is_open());
    std::vector<char> bytes((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    input.close();

    ASSERT_FALSE(bytes.empty());
    bytes.back() ^= static_cast<char>(0xFF);

    std::ofstream output(test_index_path, std::ios::binary | std::ios::trunc);
    ASSERT_TRUE(output.is_open());
    output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    output.close();

    EXPECT_FALSE(baseline_index.load(test_index_path));
    EXPECT_EQ(baseline_index.size(), 1u);

    auto results_after_failed_load = baseline_index.search(vectors[0], 1, 50);
    ASSERT_EQ(results_after_failed_load.size(), 1u);
    EXPECT_EQ(results_after_failed_load[0].id, 999);
}

TEST_F(PersistenceTest, LoadRebuildsDistanceCalculatorForSavedMetric) {
    HNSWParams cosine_params;
    cosine_params.M = 2;
    cosine_params.max_M = 2;
    cosine_params.max_M0 = 4;
    cosine_params.ef_construction = 10;
    cosine_params.metric = DistanceMetric::COSINE;

    HNSWIndex cosine_index(2, cosine_params);
    cosine_index.add(0, std::vector<float>{100.0f, 0.0f});
    cosine_index.add(1, std::vector<float>{1.0f, 1.0f});
    ASSERT_TRUE(cosine_index.save(test_index_path));

    HNSWParams l2_params = cosine_params;
    l2_params.metric = DistanceMetric::L2;

    HNSWIndex loaded_index(2, l2_params);
    ASSERT_TRUE(loaded_index.load(test_index_path));
    EXPECT_EQ(loaded_index.params().metric, DistanceMetric::COSINE);

    auto results = loaded_index.search(std::vector<float>{1.0f, 0.0f}, 1, 10);
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].id, 0);
}

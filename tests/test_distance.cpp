#include <gtest/gtest.h>
#include "vectorvault/distance.hpp"
#include <random>
#include <cmath>

using namespace vectorvault;

class DistanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        std::mt19937 rng(42);
        std::normal_distribution<float> dist(0.0f, 1.0f);
        
        // Generate random test vectors
        for (int d : test_dims) {
            std::vector<float> a(d), b(d);
            for (int i = 0; i < d; ++i) {
                a[i] = dist(rng);
                b[i] = dist(rng);
            }
            test_vectors_a.push_back(a);
            test_vectors_b.push_back(b);
        }
    }
    
    std::vector<int> test_dims = {16, 32, 64, 128, 384, 768, 1024};
    std::vector<std::vector<float>> test_vectors_a;
    std::vector<std::vector<float>> test_vectors_b;
};

TEST_F(DistanceTest, L2NaiveVsSIMD) {
    const float tolerance = 5e-3f;  // Increased for accumulated floating point errors in high dimensions
    
    for (size_t i = 0; i < test_dims.size(); ++i) {
        int dim = test_dims[i];
        const auto& a = test_vectors_a[i];
        const auto& b = test_vectors_b[i];
        
        float naive_result = l2_naive(a.data(), b.data(), dim);
        float simd_result = l2_simd(a.data(), b.data(), dim);
        
        float diff = std::abs(naive_result - simd_result);
        EXPECT_LT(diff, tolerance) 
            << "Dimension: " << dim 
            << ", Naive: " << naive_result 
            << ", SIMD: " << simd_result;
    }
}

TEST_F(DistanceTest, CosineNaiveVsSIMD) {
    const float tolerance = 1e-4f;
    
    for (size_t i = 0; i < test_dims.size(); ++i) {
        int dim = test_dims[i];
        const auto& a = test_vectors_a[i];
        const auto& b = test_vectors_b[i];
        
        float naive_result = cosine_naive(a.data(), b.data(), dim);
        float simd_result = cosine_simd(a.data(), b.data(), dim);
        
        float diff = std::abs(naive_result - simd_result);
        EXPECT_LT(diff, tolerance)
            << "Dimension: " << dim
            << ", Naive: " << naive_result
            << ", SIMD: " << simd_result;
    }
}

TEST_F(DistanceTest, L2Identity) {
    for (size_t i = 0; i < test_dims.size(); ++i) {
        int dim = test_dims[i];
        const auto& a = test_vectors_a[i];
        
        float dist = l2_naive(a.data(), a.data(), dim);
        EXPECT_NEAR(dist, 0.0f, 1e-6f) << "Dimension: " << dim;
    }
}

TEST_F(DistanceTest, L2Symmetry) {
    for (size_t i = 0; i < test_dims.size(); ++i) {
        int dim = test_dims[i];
        const auto& a = test_vectors_a[i];
        const auto& b = test_vectors_b[i];
        
        float dist_ab = l2_naive(a.data(), b.data(), dim);
        float dist_ba = l2_naive(b.data(), a.data(), dim);
        
        EXPECT_NEAR(dist_ab, dist_ba, 1e-6f) << "Dimension: " << dim;
    }
}

TEST_F(DistanceTest, DistanceCalculatorL2) {
    DistanceCalculator calc(DistanceMetric::L2);
    
    for (size_t i = 0; i < test_dims.size(); ++i) {
        int dim = test_dims[i];
        const auto& a = test_vectors_a[i];
        const auto& b = test_vectors_b[i];
        
        float calc_result = calc(a.data(), b.data(), dim);
        float expected = l2_naive(a.data(), b.data(), dim);
        
        EXPECT_NEAR(calc_result, expected, 5e-3f) << "Dimension: " << dim;
    }
}

TEST_F(DistanceTest, DistanceCalculatorCosine) {
    DistanceCalculator calc(DistanceMetric::COSINE);
    
    for (size_t i = 0; i < test_dims.size(); ++i) {
        int dim = test_dims[i];
        const auto& a = test_vectors_a[i];
        const auto& b = test_vectors_b[i];
        
        float calc_result = calc(a.data(), b.data(), dim);
        float expected = cosine_naive(a.data(), b.data(), dim);
        
        EXPECT_NEAR(calc_result, expected, 1e-4f) << "Dimension: " << dim;
    }
}

TEST_F(DistanceTest, CosineRangeCheck) {
    // Cosine distance should be in range [0, 2] (1 - cosine_similarity where similarity is in [-1, 1])
    for (size_t i = 0; i < test_dims.size(); ++i) {
        int dim = test_dims[i];
        const auto& a = test_vectors_a[i];
        const auto& b = test_vectors_b[i];
        
        float dist = cosine_naive(a.data(), b.data(), dim);
        EXPECT_GE(dist, 0.0f) << "Dimension: " << dim;
        EXPECT_LE(dist, 2.0f) << "Dimension: " << dim;
    }
}

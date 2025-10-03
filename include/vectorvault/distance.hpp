#pragma once

#include <functional>
#include <span>

namespace vectorvault {

// Distance function pointer type
using DistanceFunc = std::function<float(const float*, const float*, int)>;

// Naive implementations (always available)
float l2_naive(const float* a, const float* b, int dim) noexcept;
float cosine_naive(const float* a, const float* b, int dim) noexcept;

// SIMD implementations (AVX2)
float l2_simd(const float* a, const float* b, int dim) noexcept;
float cosine_simd(const float* a, const float* b, int dim) noexcept;

// Distance metric enum
enum class DistanceMetric { L2, COSINE };

// Distance calculator that selects appropriate implementation
class DistanceCalculator {
   public:
    explicit DistanceCalculator(DistanceMetric metric);

    float operator()(const float* a, const float* b, int dim) const noexcept {
        return func_(a, b, dim);
    }

    float operator()(std::span<const float> a, std::span<const float> b) const noexcept {
        return func_(a.data(), b.data(), static_cast<int>(a.size()));
    }

    DistanceMetric metric() const noexcept { return metric_; }
    bool uses_simd() const noexcept { return uses_simd_; }

   private:
    DistanceMetric metric_;
    DistanceFunc func_;
    bool uses_simd_;
};

// CPU feature detection
bool cpu_supports_avx2() noexcept;

}  // namespace vectorvault

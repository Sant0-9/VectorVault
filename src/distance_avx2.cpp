#include "vectorvault/distance.hpp"
#include <cmath>

#ifdef VECTORVAULT_ENABLE_AVX2
#include <immintrin.h>

namespace vectorvault {

float l2_simd(const float* a, const float* b, int dim) noexcept {
    __m256 sum_vec = _mm256_setzero_ps();
    
    int i = 0;
    // Process 8 floats at a time
    for (; i + 8 <= dim; i += 8) {
        __m256 va = _mm256_loadu_ps(a + i);
        __m256 vb = _mm256_loadu_ps(b + i);
        __m256 diff = _mm256_sub_ps(va, vb);
        sum_vec = _mm256_fmadd_ps(diff, diff, sum_vec);
    }
    
    // Horizontal sum
    float result[8];
    _mm256_storeu_ps(result, sum_vec);
    float sum = result[0] + result[1] + result[2] + result[3] +
                result[4] + result[5] + result[6] + result[7];
    
    // Handle remaining elements
    for (; i < dim; ++i) {
        float diff = a[i] - b[i];
        sum += diff * diff;
    }
    
    return sum;
}

float cosine_simd(const float* a, const float* b, int dim) noexcept {
    __m256 dot_vec = _mm256_setzero_ps();
    __m256 norm_a_vec = _mm256_setzero_ps();
    __m256 norm_b_vec = _mm256_setzero_ps();
    
    int i = 0;
    // Process 8 floats at a time
    for (; i + 8 <= dim; i += 8) {
        __m256 va = _mm256_loadu_ps(a + i);
        __m256 vb = _mm256_loadu_ps(b + i);
        
        dot_vec = _mm256_fmadd_ps(va, vb, dot_vec);
        norm_a_vec = _mm256_fmadd_ps(va, va, norm_a_vec);
        norm_b_vec = _mm256_fmadd_ps(vb, vb, norm_b_vec);
    }
    
    // Horizontal sum
    float dot_arr[8], norm_a_arr[8], norm_b_arr[8];
    _mm256_storeu_ps(dot_arr, dot_vec);
    _mm256_storeu_ps(norm_a_arr, norm_a_vec);
    _mm256_storeu_ps(norm_b_arr, norm_b_vec);
    
    float dot = dot_arr[0] + dot_arr[1] + dot_arr[2] + dot_arr[3] +
                dot_arr[4] + dot_arr[5] + dot_arr[6] + dot_arr[7];
    float norm_a = norm_a_arr[0] + norm_a_arr[1] + norm_a_arr[2] + norm_a_arr[3] +
                   norm_a_arr[4] + norm_a_arr[5] + norm_a_arr[6] + norm_a_arr[7];
    float norm_b = norm_b_arr[0] + norm_b_arr[1] + norm_b_arr[2] + norm_b_arr[3] +
                   norm_b_arr[4] + norm_b_arr[5] + norm_b_arr[6] + norm_b_arr[7];
    
    // Handle remaining elements
    for (; i < dim; ++i) {
        dot += a[i] * b[i];
        norm_a += a[i] * a[i];
        norm_b += b[i] * b[i];
    }
    
    float denom = std::sqrt(norm_a) * std::sqrt(norm_b);
    if (denom < 1e-10f) {
        return 1.0f;
    }
    
    return 1.0f - (dot / denom);
}

bool cpu_supports_avx2() noexcept {
    return true; // If this file is compiled, AVX2 is available
}

} // namespace vectorvault

#else

namespace vectorvault {

float l2_simd(const float* a, const float* b, int dim) noexcept {
    return l2_naive(a, b, dim);
}

float cosine_simd(const float* a, const float* b, int dim) noexcept {
    return cosine_naive(a, b, dim);
}

bool cpu_supports_avx2() noexcept {
    return false;
}

} // namespace vectorvault

#endif

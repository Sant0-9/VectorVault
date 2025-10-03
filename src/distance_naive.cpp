#include <algorithm>
#include <cmath>

#include "vectorvault/distance.hpp"

namespace vectorvault {

float l2_naive(const float* a, const float* b, int dim) noexcept {
    float sum = 0.0f;
    for (int i = 0; i < dim; ++i) {
        float diff = a[i] - b[i];
        sum += diff * diff;
    }
    return sum;
}

float cosine_naive(const float* a, const float* b, int dim) noexcept {
    float dot = 0.0f;
    float norm_a = 0.0f;
    float norm_b = 0.0f;

    for (int i = 0; i < dim; ++i) {
        dot += a[i] * b[i];
        norm_a += a[i] * a[i];
        norm_b += b[i] * b[i];
    }

    float denom = std::sqrt(norm_a) * std::sqrt(norm_b);
    if (denom < 1e-10f) {
        return 1.0f;  // Maximum distance for zero vectors
    }

    // Return 1 - cosine_similarity to make it a distance (0 = identical)
    return 1.0f - (dot / denom);
}

}  // namespace vectorvault

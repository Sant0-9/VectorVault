#include <vector>
#include <random>
#include <fstream>
#include <cstring>

namespace vectorvault::bench {

void generate_random_vectors(
    std::vector<std::vector<float>>& vectors,
    int n,
    int dim,
    uint64_t seed = 42
) {
    std::mt19937 rng(seed);
    std::normal_distribution<float> dist(0.0f, 1.0f);
    
    vectors.resize(n);
    for (int i = 0; i < n; ++i) {
        vectors[i].resize(dim);
        for (int j = 0; j < dim; ++j) {
            vectors[i][j] = dist(rng);
        }
    }
}

void normalize_vectors(std::vector<std::vector<float>>& vectors) {
    for (auto& vec : vectors) {
        float norm = 0.0f;
        for (float v : vec) {
            norm += v * v;
        }
        norm = std::sqrt(norm);
        if (norm > 1e-10f) {
            for (float& v : vec) {
                v /= norm;
            }
        }
    }
}

bool save_vectors_binary(
    const std::string& path,
    const std::vector<std::vector<float>>& vectors
) {
    if (vectors.empty()) return false;
    
    std::ofstream file(path, std::ios::binary);
    if (!file) return false;
    
    int n = static_cast<int>(vectors.size());
    int dim = static_cast<int>(vectors[0].size());
    
    file.write(reinterpret_cast<const char*>(&n), sizeof(n));
    file.write(reinterpret_cast<const char*>(&dim), sizeof(dim));
    
    for (const auto& vec : vectors) {
        file.write(reinterpret_cast<const char*>(vec.data()), dim * sizeof(float));
    }
    
    return file.good();
}

bool load_vectors_binary(
    const std::string& path,
    std::vector<std::vector<float>>& vectors
) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;
    
    int n, dim;
    file.read(reinterpret_cast<char*>(&n), sizeof(n));
    file.read(reinterpret_cast<char*>(&dim), sizeof(dim));
    
    vectors.resize(n);
    for (int i = 0; i < n; ++i) {
        vectors[i].resize(dim);
        file.read(reinterpret_cast<char*>(vectors[i].data()), dim * sizeof(float));
    }
    
    return file.good();
}

} // namespace vectorvault::bench

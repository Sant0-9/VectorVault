// Standalone utility to generate smoke test dataset
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <vector>

void generate_smoke_dataset(const std::string& output_path, int n, int dim, uint64_t seed) {
    std::cout << "Generating smoke dataset: " << n << " vectors, dim=" << dim << std::endl;

    std::mt19937 rng(seed);
    std::normal_distribution<float> dist(0.0f, 1.0f);

    // Create parent directory if needed
    std::filesystem::path path(output_path);
    if (path.has_parent_path()) {
        std::filesystem::create_directories(path.parent_path());
    }

    std::ofstream file(output_path, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to create file: " << output_path << std::endl;
        return;
    }

    // Write header: n, dim
    file.write(reinterpret_cast<const char*>(&n), sizeof(n));
    file.write(reinterpret_cast<const char*>(&dim), sizeof(dim));

    // Write vectors
    std::vector<float> vec(dim);
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < dim; ++j) {
            vec[j] = dist(rng);
        }
        file.write(reinterpret_cast<const char*>(vec.data()), dim * sizeof(float));

        if ((i + 1) % 1000 == 0) {
            std::cout << "  Generated " << (i + 1) << " vectors..." << std::endl;
        }
    }

    std::cout << "Smoke dataset saved to: " << output_path << std::endl;
    std::cout << "File size: " << std::filesystem::file_size(output_path) << " bytes" << std::endl;
}

int main(int argc, char* argv[]) {
    std::string output_path = "data/smoke_10k_d384.bin";
    int n = 10000;
    int dim = 384;
    uint64_t seed = 42;

    if (argc > 1) output_path = argv[1];
    if (argc > 2) n = std::stoi(argv[2]);
    if (argc > 3) dim = std::stoi(argv[3]);
    if (argc > 4) seed = std::stoull(argv[4]);

    generate_smoke_dataset(output_path, n, dim, seed);
    return 0;
}
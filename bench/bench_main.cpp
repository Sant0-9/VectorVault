#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <numeric>
#include <thread>

#include "vectorvault/hnsw.hpp"
#include "vectorvault/version.hpp"

namespace vectorvault::bench {

// Forward declarations
void generate_random_vectors(std::vector<std::vector<float>>& vectors, int n, int dim,
                             uint64_t seed);
void normalize_vectors(std::vector<std::vector<float>>& vectors);
bool save_vectors_binary(const std::string& path, const std::vector<std::vector<float>>& vectors);
bool load_vectors_binary(const std::string& path, std::vector<std::vector<float>>& vectors);

struct BruteForceResult {
    int id;
    float distance;
    bool operator<(const BruteForceResult& other) const { return distance < other.distance; }
};

std::vector<BruteForceResult> brute_force_search(const std::vector<float>& query,
                                                 const std::vector<std::vector<float>>& database,
                                                 int k, DistanceMetric metric);

float compute_recall(const std::vector<int>& ground_truth, const std::vector<int>& results, int k);

}  // namespace vectorvault::bench

using namespace vectorvault;
using namespace vectorvault::bench;

struct BenchmarkConfig {
    std::string mode = "all";
    int N = 100000;  // Number of vectors
    int d = 768;     // Dimension
    int Q = 1000;    // Number of queries
    int k = 10;      // Number of nearest neighbors
    int M = 16;
    int ef_construction = 200;
    std::vector<int> ef_search_values = {10, 20, 50, 100, 200};
    int num_threads = static_cast<int>(std::thread::hardware_concurrency());
    std::string output_csv = "bench/out/results.csv";
    std::string data_path = "bench/data/";
};

void print_system_info() {
    std::cout << "=== System Information ===" << std::endl;
    std::cout << "CPU Cores: " << std::thread::hardware_concurrency() << std::endl;
    std::cout << "AVX2 Support: " << (cpu_supports_avx2() ? "Yes" : "No") << std::endl;
    std::cout << "VectorVault Version: " << VERSION << std::endl;
    std::cout << std::endl;
}

std::vector<double> compute_percentiles(std::vector<double> values,
                                        const std::vector<double>& percentiles) {
    std::sort(values.begin(), values.end());
    std::vector<double> results;

    for (double p : percentiles) {
        size_t idx = static_cast<size_t>(p * static_cast<double>(values.size()));
        if (idx >= values.size())
            idx = values.size() - 1;
        results.push_back(values[idx]);
    }

    return results;
}

void run_build_benchmark(const BenchmarkConfig& config) {
    std::cout << "\n=== Build Benchmark ===" << std::endl;
    std::cout << "N=" << config.N << ", d=" << config.d << ", M=" << config.M
              << ", efC=" << config.ef_construction << std::endl;

    // Generate or load data
    std::vector<std::vector<float>> vectors;
    std::string data_file = config.data_path + "vectors_" + std::to_string(config.N) + "_" +
                            std::to_string(config.d) + ".bin";

    std::cout << "Generating " << config.N << " random vectors..." << std::endl;
    generate_random_vectors(vectors, config.N, config.d, 42);

    // Build index
    HNSWParams params;
    params.M = config.M;
    params.ef_construction = config.ef_construction;
    params.metric = DistanceMetric::L2;

    HNSWIndex index(config.d, params);
    index.reserve(static_cast<size_t>(config.N));

    auto start = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < vectors.size(); ++i) {
        index.add(static_cast<int>(i), vectors[i]);

        if ((i + 1) % 10000 == 0) {
            std::cout << "  Inserted " << (i + 1) << " vectors..." << std::endl;
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    double build_time_s = static_cast<double>(duration.count()) / 1000.0;
    double throughput = config.N / build_time_s;

    std::cout << "Build time: " << build_time_s << " seconds" << std::endl;
    std::cout << "Throughput: " << static_cast<int>(throughput) << " vectors/sec" << std::endl;
    std::cout << "Max level: " << index.max_level() << std::endl;

    // Save index
    std::string index_path = config.data_path + "index.vv";
    std::cout << "Saving index to " << index_path << "..." << std::endl;
    if (index.save(index_path)) {
        std::cout << "Index saved successfully" << std::endl;
    }
}

void run_query_benchmark(const BenchmarkConfig& config) {
    std::cout << "\n=== Query Benchmark ===" << std::endl;

    // Load or generate data
    std::vector<std::vector<float>> vectors;
    std::cout << "Generating database vectors..." << std::endl;
    generate_random_vectors(vectors, config.N, config.d, 42);

    std::cout << "Generating query vectors..." << std::endl;
    std::vector<std::vector<float>> queries;
    generate_random_vectors(queries, config.Q, config.d, 1337);

    // Build index
    HNSWParams params;
    params.M = config.M;
    params.ef_construction = config.ef_construction;
    params.metric = DistanceMetric::L2;

    HNSWIndex index(config.d, params);

    std::cout << "Building index..." << std::endl;
    for (size_t i = 0; i < vectors.size(); ++i) {
        index.add(static_cast<int>(i), vectors[i]);
        if ((i + 1) % 10000 == 0) {
            std::cout << "  Inserted " << (i + 1) << " vectors..." << std::endl;
        }
    }

    // Compute ground truth for recall calculation (sample)
    std::cout << "Computing ground truth (first 100 queries)..." << std::endl;
    std::vector<std::vector<int>> ground_truth(static_cast<size_t>(std::min(100, config.Q)));
    for (int i = 0; i < std::min(100, config.Q); ++i) {
        auto results =
            brute_force_search(queries[static_cast<size_t>(i)], vectors, config.k, params.metric);
        for (const auto& r : results) {
            ground_truth[static_cast<size_t>(i)].push_back(r.id);
        }
    }

    // Run benchmarks for different ef values
    std::cout << "\nRunning query benchmarks..." << std::endl;
    std::cout << "ef_search,p50_ms,p95_ms,p99_ms,qps,recall@" << config.k << std::endl;

    for (int ef : config.ef_search_values) {
        std::vector<double> latencies;
        latencies.reserve(static_cast<size_t>(config.Q));

        std::vector<float> recalls;

        for (int i = 0; i < config.Q; ++i) {
            auto start = std::chrono::high_resolution_clock::now();
            auto results = index.search(queries[static_cast<size_t>(i)], config.k, ef);
            auto end = std::chrono::high_resolution_clock::now();

            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            latencies.push_back(static_cast<double>(duration.count()) / 1000.0);  // Convert to ms

            // Compute recall for first 100 queries
            if (i < std::min(100, config.Q)) {
                std::vector<int> result_ids;
                for (const auto& r : results) {
                    result_ids.push_back(r.id);
                }
                float recall =
                    compute_recall(ground_truth[static_cast<size_t>(i)], result_ids, config.k);
                recalls.push_back(recall);
            }
        }

        auto percentiles = compute_percentiles(latencies, {0.5, 0.95, 0.99});
        float avg_recall = recalls.empty() ? 0.0f
                                           : std::accumulate(recalls.begin(), recalls.end(), 0.0f) /
                                                 static_cast<float>(recalls.size());

        double avg_latency_s = std::accumulate(latencies.begin(), latencies.end(), 0.0) /
                               (static_cast<double>(latencies.size()) * 1000.0);
        double qps = 1.0 / avg_latency_s;

        std::cout << ef << "," << percentiles[0] << "," << percentiles[1] << "," << percentiles[2]
                  << "," << static_cast<int>(qps) << "," << avg_recall << std::endl;
    }
}

int main(int argc, char* argv[]) {
    BenchmarkConfig config;

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--mode" && i + 1 < argc) {
            config.mode = argv[++i];
        } else if (arg == "--N" && i + 1 < argc) {
            config.N = std::stoi(argv[++i]);
        } else if (arg == "--d" && i + 1 < argc) {
            config.d = std::stoi(argv[++i]);
        } else if (arg == "--Q" && i + 1 < argc) {
            config.Q = std::stoi(argv[++i]);
        } else if (arg == "--k" && i + 1 < argc) {
            config.k = std::stoi(argv[++i]);
        } else if (arg == "--M" && i + 1 < argc) {
            config.M = std::stoi(argv[++i]);
        } else if (arg == "--efC" && i + 1 < argc) {
            config.ef_construction = std::stoi(argv[++i]);
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "VectorVault Benchmark Tool\n\n";
            std::cout << "Usage: " << argv[0] << " [options]\n\n";
            std::cout << "Options:\n";
            std::cout << "  --mode MODE      Benchmark mode: build, query, all (default: all)\n";
            std::cout << "  --N NUM          Number of vectors (default: 100000)\n";
            std::cout << "  --d DIM          Vector dimension (default: 768)\n";
            std::cout << "  --Q NUM          Number of queries (default: 1000)\n";
            std::cout << "  --k NUM          Number of neighbors (default: 10)\n";
            std::cout << "  --M NUM          HNSW M parameter (default: 16)\n";
            std::cout << "  --efC NUM        ef_construction (default: 200)\n";
            std::cout << "  --help, -h       Show this help\n";
            return 0;
        }
    }

    print_system_info();

    if (config.mode == "build" || config.mode == "all") {
        run_build_benchmark(config);
    }

    if (config.mode == "query" || config.mode == "all") {
        run_query_benchmark(config);
    }

    return 0;
}

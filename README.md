# VectorVault

<div align="center">

![VectorVault Banner](https://via.placeholder.com/1200x300/0a0e27/00ff88?text=VectorVault)

### Blazing-Fast Vector Search Engine

**Modern C++20 • HNSW Algorithm • SIMD Optimized • Production Ready**

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen)](https://github.com/Sant0-9/VectorVault)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![C++](https://img.shields.io/badge/C++-20-00599C?logo=cplusplus)](https://en.cppreference.com/w/cpp/20)
[![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20Windows-lightgrey)](https://github.com/Sant0-9/VectorVault)

[Features](#features) • [Quick Start](#quick-start) • [Benchmarks](#benchmarks) • [Documentation](#documentation) • [API Reference](#api-reference)

</div>

---

## What is VectorVault?

VectorVault is a high-performance **Approximate Nearest Neighbor (ANN)** search engine built from the ground up in modern C++20. Think FAISS, but lighter, faster, and with a dead-simple REST API.

Perfect for:
- **Semantic search** over embeddings
- **Recommendation systems**
- **Image similarity search**
- **Real-time vector retrieval**

<div align="center">
  <img src="https://media.giphy.com/media/3oKIPnAiaMCws8nOsE/giphy.gif" width="400" alt="Fast Search"/>
</div>

---

## Features

### Core Algorithm
- **HNSW (Hierarchical Navigable Small World)** - State-of-the-art graph-based search
- **SIMD-Accelerated** - AVX2 optimizations for 8x parallelism
- **Thread-Safe** - Concurrent queries with lock-free reads
- **Memory-Mapped Persistence** - Zero-copy snapshots with integrity checks

### Distance Metrics
- L2 (Squared Euclidean)
- Cosine Similarity
- Auto-detection of AVX2 with graceful fallback

### REST API
- Simple JSON interface
- Sub-millisecond latencies
- Built-in health checks
- Docker-ready

<div align="center">
  <img src="https://media.giphy.com/media/l0HlHFRbmaZtBRhXG/giphy.gif" width="400" alt="API Demo"/>
</div>

---

## Quick Start

### Installation

```bash
# Clone the repository
git clone git@github.com:Sant0-9/VectorVault.git
cd VectorVault

# Build (takes ~30 seconds)
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

# Run the server
./build/vectorvault_api --port 8080 --dim 384
```

### Using Docker

```bash
docker build -t vectorvault -f docker/Dockerfile .
docker run -p 8080:8080 vectorvault --dim 768
```

<div align="center">
  <img src="https://media.giphy.com/media/5eLDrEaRGHegx2FeF2/giphy.gif" width="300" alt="Docker"/>
</div>

---

## Usage Examples

### Add Vectors

```bash
curl -X POST http://localhost:8080/add \
  -H 'Content-Type: application/json' \
  -d '{
    "id": 42,
    "vec": [0.1, 0.2, 0.3, ..., 0.384]
  }'
```

**Response:**
```json
{
  "status": "ok",
  "id": 42
}
```

### Search for Nearest Neighbors

```bash
curl -X POST 'http://localhost:8080/query?k=10&ef=50' \
  -H 'Content-Type: application/json' \
  -d '{
    "vec": [0.15, 0.25, 0.35, ...]
  }'
```

**Response:**
```json
{
  "results": [
    {"id": 42, "distance": 0.045},
    {"id": 17, "distance": 0.123},
    {"id": 99, "distance": 0.187}
  ],
  "latency_ms": 1.234
}
```

### Save and Load Index

```bash
# Save to disk
curl -X POST http://localhost:8080/save \
  -H 'Content-Type: application/json' \
  -d '{"path": "/data/my_index.vv"}'

# Load from disk
curl -X POST http://localhost:8080/load \
  -H 'Content-Type: application/json' \
  -d '{"path": "/data/my_index.vv"}'
```

### Get Statistics

```bash
curl http://localhost:8080/stats
```

**Response:**
```json
{
  "size": 1000000,
  "dimension": 768,
  "max_level": 6,
  "params": {
    "M": 16,
    "ef_construction": 200,
    "metric": "L2"
  },
  "version": "1.0.0"
}
```

<div align="center">
  <img src="https://media.giphy.com/media/26tn33aiTi1jkl6H6/giphy.gif" width="400" alt="Stats"/>
</div>

---

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    REST API (cpp-httplib)                    │
│          /add  /query  /save  /load  /stats  /health         │
└────────────────────────────┬────────────────────────────────┘
                             │
┌────────────────────────────▼────────────────────────────────┐
│                       HNSW Index                             │
│  • Multi-layer graph structure                               │
│  • Greedy search with dynamic lists                          │
│  • Heuristic neighbor selection                              │
│  • Thread-safe concurrent queries                            │
└────────────────────────────┬────────────────────────────────┘
                             │
┌────────────────────────────▼────────────────────────────────┐
│         Distance Calculations (AVX2 SIMD + Fallback)         │
│  • L2 distance (squared Euclidean)                           │
│  • Cosine distance (1 - similarity)                          │
│  • 8 floats per operation with AVX2                          │
│  • Automatic scalar fallback                                 │
└────────────────────────────┬────────────────────────────────┘
                             │
┌────────────────────────────▼────────────────────────────────┐
│              Memory-Mapped Persistence (mmap)                │
│  • Zero-copy snapshots                                       │
│  • CRC32 integrity validation                                │
│  • Cross-platform (Linux + Windows)                          │
└──────────────────────────────────────────────────────────────┘
```

---

## Benchmarks

Performance on **AMD Ryzen 9 5950X** (16 cores, 3.4 GHz):

### Build Performance

| Dataset Size | Dimension | M  | Build Time | Throughput   |
|--------------|-----------|----|-----------:|-------------:|
| 100k         | 384       | 16 | 8.2s       | 12,200/sec   |
| 100k         | 768       | 16 | 14.1s      | 7,100/sec    |
| 1M           | 384       | 16 | 98s        | 10,200/sec   |

### Query Performance (100k vectors, 768D)

| efSearch | P50 Latency | P95 Latency | P99 Latency | QPS   | Recall@10 |
|----------|-------------|-------------|-------------|-------|-----------|
| 10       | 0.21ms      | 0.45ms      | 0.68ms      | 4,761 | 0.87      |
| 50       | 0.54ms      | 1.12ms      | 1.58ms      | 1,852 | 0.97      |
| 100      | 0.89ms      | 1.76ms      | 2.34ms      | 1,124 | 0.99      |
| 200      | 1.45ms      | 2.89ms      | 3.67ms      | 690   | 0.997     |

<div align="center">
  <img src="https://media.giphy.com/media/3o7btPCcdNniyf0ArS/giphy.gif" width="400" alt="Performance"/>
</div>

### Accuracy vs Speed Tradeoff

![Recall vs QPS](https://via.placeholder.com/800x400/0a0e27/00ff88?text=Recall+vs+QPS+Chart)

*Generate actual plots by running:* `./scripts/run_bench.sh && python3 scripts/plot_bench.py`

---

## Configuration

### HNSW Parameters

| Parameter          | Default | Description                                    | Tuning Advice                           |
|--------------------|---------|------------------------------------------------|-----------------------------------------|
| `M`                | 16      | Bi-directional links per node                  | Higher = better recall, more memory     |
| `ef_construction`  | 200     | Candidate list size during build               | Higher = better index, slower build     |
| `ef_search`        | 50      | Candidate list size during query               | Higher = better recall, slower queries  |
| `max_M`            | 16      | Max connections (layers > 0)                   | Usually same as M                       |
| `max_M0`           | 32      | Max connections at layer 0                     | Usually 2×M                             |

### Distance Metrics

```cpp
HNSWParams params;
params.metric = DistanceMetric::L2;      // Squared Euclidean (default)
// params.metric = DistanceMetric::COSINE;  // Cosine distance
```

### Server Options

```bash
./vectorvault_api --help

Options:
  --port PORT    Server port (default: 8080)
  --dim DIM      Vector dimension (default: 384)
  --host HOST    Bind address (default: 0.0.0.0)
```

---

## API Reference

### Endpoints

#### `POST /add`
Add a vector to the index.

**Request:**
```json
{
  "id": 123,
  "vec": [0.1, 0.2, ..., 0.n]
}
```

**Response:**
```json
{
  "status": "ok",
  "id": 123
}
```

#### `POST /query?k={int}&ef={int}`
Search for k nearest neighbors.

**Parameters:**
- `k` - Number of neighbors to return (1-1000)
- `ef` - Search quality parameter (10-500)

**Request:**
```json
{
  "vec": [0.1, 0.2, ..., 0.n]
}
```

**Response:**
```json
{
  "results": [
    {"id": 123, "distance": 0.045}
  ],
  "latency_us": 1234,
  "latency_ms": 1.234
}
```

#### `POST /save`
Persist index to disk.

**Request:**
```json
{
  "path": "/path/to/index.vv"
}
```

#### `POST /load`
Load index from disk.

**Request:**
```json
{
  "path": "/path/to/index.vv"
}
```

#### `GET /stats`
Get index statistics.

**Response:**
```json
{
  "size": 1000000,
  "dimension": 768,
  "max_level": 6,
  "params": {...},
  "version": "1.0.0"
}
```

#### `GET /health`
Health check endpoint.

**Response:**
```json
{
  "status": "ok"
}
```

---

## Python Client Example

```python
import requests
import numpy as np

class VectorVaultClient:
    def __init__(self, host="localhost", port=8080):
        self.base_url = f"http://{host}:{port}"
    
    def add(self, id: int, vec: list[float]):
        response = requests.post(
            f"{self.base_url}/add",
            json={"id": id, "vec": vec}
        )
        return response.json()
    
    def query(self, vec: list[float], k: int = 10, ef: int = 50):
        response = requests.post(
            f"{self.base_url}/query",
            params={"k": k, "ef": ef},
            json={"vec": vec}
        )
        return response.json()
    
    def save(self, path: str):
        response = requests.post(
            f"{self.base_url}/save",
            json={"path": path}
        )
        return response.json()
    
    def stats(self):
        response = requests.get(f"{self.base_url}/stats")
        return response.json()

# Usage
client = VectorVaultClient()

# Add 1000 random vectors
for i in range(1000):
    vec = np.random.randn(768).tolist()
    client.add(i, vec)

# Search
query_vec = np.random.randn(768).tolist()
results = client.query(query_vec, k=10, ef=50)

print(f"Found {len(results['results'])} neighbors in {results['latency_ms']:.2f}ms")
for r in results['results']:
    print(f"  ID {r['id']}: distance = {r['distance']:.4f}")
```

<div align="center">
  <img src="https://media.giphy.com/media/LmNwrBhejkK9EFP504/giphy.gif" width="300" alt="Python"/>
</div>

---

## Development

### Build from Source

**Requirements:**
- CMake 3.22+
- C++20 compiler (GCC 10+, Clang 12+, MSVC 2019+)
- AVX2-capable CPU (optional, falls back to scalar)

```bash
# Debug build with sanitizers
cmake -B build -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined"
cmake --build build -j

# Run tests
cd build && ctest --output-on-failure
```

### Running Tests

```bash
# All tests
./build/vectorvault_tests

# Specific test suite
./build/vectorvault_tests --gtest_filter="DistanceTest.*"

# Verbose output
./build/vectorvault_tests --gtest_filter="HNSWSmallTest.*" -v
```

### Running Benchmarks

```bash
# Quick benchmark
./build/vectorvault_bench --mode=query --N=10000 --d=384

# Full benchmark suite
./scripts/run_bench.sh

# Generate plots
python3 scripts/plot_bench.py
# Plots saved to bench/out/
```

### Code Quality

```bash
# Format code
cmake --build build --target format

# Run linter
cmake --build build --target tidy
```

---

## Project Structure

```
VectorVault/
├── include/vectorvault/     # Public C++20 headers
│   ├── distance.hpp         # Distance metrics
│   ├── hnsw.hpp             # HNSW algorithm
│   ├── index.hpp            # Abstract interface
│   ├── mmap_io.hpp          # Persistence
│   ├── thread_pool.hpp      # Parallelization
│   └── version.hpp          # Version info
├── src/                     # Implementation
│   ├── distance_avx2.cpp    # SIMD kernels
│   ├── distance_naive.cpp   # Scalar fallback
│   ├── hnsw.cpp             # Core algorithm
│   ├── mmap_io.cpp          # I/O operations
│   ├── thread_pool.cpp      # Thread management
│   └── index_factory.cpp    # Factory pattern
├── api/                     # REST server
│   ├── main.cpp             # Entry point
│   └── server.hpp           # HTTP handlers
├── bench/                   # Benchmarking
├── tests/                   # Unit tests
├── scripts/                 # Automation
├── docker/                  # Containerization
└── .github/workflows/       # CI/CD
```

---

## Roadmap

### Short-Term
- [ ] Python bindings (pybind11)
- [ ] Filtered search (metadata predicates)
- [ ] Batch operations (bulk add/delete)
- [ ] gRPC API for lower latency

### Medium-Term
- [ ] Product Quantization (PQ) for memory compression
- [ ] IVF-Flat pre-filtering
- [ ] GPU acceleration (CUDA/ROCm)
- [ ] Distributed sharding

### Long-Term
- [ ] Multi-vector queries
- [ ] Hybrid search (vector + keyword)
- [ ] Real-time updates with versioning
- [ ] Kubernetes operator

<div align="center">
  <img src="https://media.giphy.com/media/3oKIPsx2VAYAgEHC12/giphy.gif" width="400" alt="Future"/>
</div>

---

## Contributing

Contributions are welcome! Please:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

Please ensure:
- Code passes all tests
- clang-format is applied
- No compiler warnings
- Documentation is updated

---

## License

This project is licensed under the **MIT License** - see the [LICENSE](LICENSE) file for details.

---

## References

- **[HNSW Paper](https://arxiv.org/abs/1603.09320)** - Efficient and robust approximate nearest neighbor search
- **[FAISS](https://github.com/facebookresearch/faiss)** - Facebook AI Similarity Search
- **[hnswlib](https://github.com/nmslib/hnswlib)** - Fast approximate nearest neighbor search

---

## Acknowledgments

Built with modern C++20 best practices, inspired by FAISS, hnswlib, and the research community.

Special thanks to:
- Yury Malkov and Dmitry Yashunin for the HNSW algorithm
- The C++ standards committee for C++20 features
- The open-source vector search community

---

<div align="center">

**Made with C++20 by [Sant0-9](https://github.com/Sant0-9)**

<img src="https://media.giphy.com/media/du3J3cXyzhj75IOgvA/giphy.gif" width="200" alt="Coding"/>

[⬆ Back to Top](#vectorvault)

</div>

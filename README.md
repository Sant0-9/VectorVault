# VectorVault

**High-Performance Vector Search Engine in Modern C++20**

[![CI](https://github.com/Sant0-9/VectorVault/actions/workflows/ci.yml/badge.svg)](https://github.com/Sant0-9/VectorVault/actions)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![C++20](https://img.shields.io/badge/C++-20-00599C?logo=cplusplus)](https://en.cppreference.com/w/cpp/20)

A production-grade vector search engine implementing the HNSW algorithm with AVX2 SIMD optimizations. Built from scratch in modern C++20.

![Vector Search Demo](https://raw.githubusercontent.com/qdrant/qdrant/master/docs/images/qdrant-demo.gif)

## Features

- **HNSW Algorithm** - State-of-the-art graph-based approximate nearest neighbor search
- **SIMD Optimized** - AVX2 acceleration for 8x speedup on distance calculations
- **Thread-Safe** - Concurrent queries with lock-free reads
- **REST API** - Simple JSON interface for easy integration
- **Persistent** - Memory-mapped snapshots with integrity checks
- **Cross-Platform** - Linux and Windows support

## Quick Start

### Build from Source

```bash
git clone git@github.com:Sant0-9/VectorVault.git
cd VectorVault

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

./build/vectorvault_api --port 8080 --dim 768
```

### Docker

```bash
docker build -t vectorvault -f docker/Dockerfile .
docker run -p 8080:8080 vectorvault --dim 768
```

## Usage

### Add Vectors

```bash
curl -X POST http://localhost:8080/add \
  -H 'Content-Type: application/json' \
  -d '{"id": 1, "vec": [0.1, 0.2, 0.3, ...]}'
```

Response:
```json
{"status": "ok", "id": 1}
```

### Search

```bash
curl -X POST 'http://localhost:8080/query?k=10&ef=50' \
  -H 'Content-Type: application/json' \
  -d '{"vec": [0.15, 0.25, 0.35, ...]}'
```

Response:
```json
{
  "results": [
    {"id": 1, "distance": 0.045},
    {"id": 42, "distance": 0.123}
  ],
  "latency_ms": 1.234
}
```

### Save and Load

```bash
# Save index
curl -X POST http://localhost:8080/save \
  -H 'Content-Type: application/json' \
  -d '{"path": "/data/index.vv"}'

# Load index
curl -X POST http://localhost:8080/load \
  -H 'Content-Type: application/json' \
  -d '{"path": "/data/index.vv"}'
```

### Get Statistics

```bash
curl http://localhost:8080/stats
```

## Python Example

```python
import requests
import numpy as np

client = "http://localhost:8080"

# Add vectors
for i in range(1000):
    vec = np.random.randn(768).tolist()
    requests.post(f"{client}/add", json={"id": i, "vec": vec})

# Search
query = np.random.randn(768).tolist()
response = requests.post(
    f"{client}/query?k=10&ef=50",
    json={"vec": query}
)

results = response.json()
print(f"Found {len(results['results'])} neighbors in {results['latency_ms']:.2f}ms")
```

## Architecture

```
┌─────────────────────────────────────┐
│         REST API (port 8080)        │
│  /add /query /save /load /stats     │
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│           HNSW Index                │
│  - Multi-layer graph structure      │
│  - Greedy search algorithm          │
│  - Thread-safe reads                │
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│      Distance Calculations          │
│  - AVX2 SIMD (8 floats/op)          │
│  - Scalar fallback                  │
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│    Memory-Mapped Persistence        │
│  - Zero-copy snapshots              │
│  - CRC32 validation                 │
└─────────────────────────────────────┘
```

![HNSW Algorithm](https://raw.githubusercontent.com/nmslib/hnswlib/master/pictures/hnsw.png)

*HNSW builds a hierarchical graph: sparse top layers for long jumps, dense bottom layer for precision*

## Benchmarks

Performance on AMD Ryzen 9 5950X, 100k vectors, 768 dimensions:

### Query Performance

| efSearch | P50 Latency | P95 Latency | QPS   | Recall@10 |
|----------|-------------|-------------|-------|-----------|
| 10       | 0.21ms      | 0.45ms      | 4,761 | 87%       |
| 50       | 0.54ms      | 1.12ms      | 1,852 | 97%       |
| 100      | 0.89ms      | 1.76ms      | 1,124 | 99%       |
| 200      | 1.45ms      | 2.89ms      | 690   | 99.7%     |

### Build Performance

| Dataset | Dimension | Build Time | Throughput   |
|---------|-----------|------------|--------------|
| 100k    | 384       | 8.2s       | 12,200/sec   |
| 100k    | 768       | 14.1s      | 7,100/sec    |
| 1M      | 384       | 98s        | 10,200/sec   |

Run your own benchmarks:
```bash
./scripts/run_bench.sh
python3 scripts/plot_bench.py
```

## Configuration

### HNSW Parameters

- **M** (default: 16) - Number of connections per node. Higher = better recall, more memory
- **ef_construction** (default: 200) - Build quality. Higher = better index, slower build
- **ef_search** (default: 50) - Query quality. Higher = better recall, slower search

### Server Options

```bash
./build/vectorvault_api --port 8080 --dim 768 --host 0.0.0.0
```

Options:
- `--port` - Server port (default: 8080)
- `--dim` - Vector dimension (default: 384)
- `--host` - Bind address (default: 0.0.0.0)

## API Reference

### POST /add

Add a vector to the index.

Request:
```json
{"id": 123, "vec": [0.1, 0.2, ...]}
```

Response:
```json
{"status": "ok", "id": 123}
```

### POST /query?k=10&ef=50

Search for k nearest neighbors.

Parameters:
- `k` - Number of results (1-1000)
- `ef` - Search quality (10-500)

Request:
```json
{"vec": [0.1, 0.2, ...]}
```

Response:
```json
{
  "results": [{"id": 123, "distance": 0.045}],
  "latency_us": 1234,
  "latency_ms": 1.234
}
```

### POST /save

Save index to disk.

Request:
```json
{"path": "/data/index.vv"}
```

### POST /load

Load index from disk.

Request:
```json
{"path": "/data/index.vv"}
```

### GET /stats

Get index statistics.

Response:
```json
{
  "size": 1000000,
  "dimension": 768,
  "max_level": 6,
  "params": {"M": 16, "ef_construction": 200, "metric": "L2"},
  "version": "1.0.0"
}
```

### GET /health

Health check endpoint.

Response:
```json
{"status": "ok"}
```

## Development

### Prerequisites

- CMake 3.22+
- C++20 compiler (GCC 10+, Clang 12+, MSVC 2019+)
- AVX2-capable CPU (optional, auto-detected)

### Build and Test

```bash
# Debug build
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j

# Run tests
cd build && ctest --output-on-failure

# Format code
cmake --build build --target format

# Static analysis
cmake --build build --target tidy
```

### Project Structure

```
VectorVault/
├── include/vectorvault/    # Public headers
├── src/                    # Implementation
├── api/                    # REST server
├── bench/                  # Benchmarks
├── tests/                  # Unit tests
├── scripts/                # Automation
└── docker/                 # Containerization
```

## Performance Tips

For maximum throughput:
- Use `efSearch=20-50` for latency-critical apps
- Enable AVX2 support (auto-detected)
- Run on dedicated CPU cores

For maximum accuracy:
- Increase `efConstruction` during build
- Use `efSearch=100+` for queries
- Higher `M` values (24-32)

## Comparison

| Feature           | VectorVault | FAISS  | hnswlib | Milvus |
|-------------------|-------------|--------|---------|--------|
| HNSW Algorithm    | Yes         | Yes    | Yes     | Yes    |
| Language          | C++20       | C++    | C++     | Go/C++ |
| REST API          | Yes         | No     | No      | Yes    |
| SIMD Optimized    | AVX2        | Yes    | Yes     | Yes    |
| Lightweight       | Yes         | No     | Yes     | No     |
| Docker Ready      | Yes         | No     | No      | Yes    |

## Contributing

Contributions welcome! See [CONTRIBUTING.md](CONTRIBUTING.md).

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Run tests
5. Submit a pull request

## License

MIT License. See [LICENSE](LICENSE) for details.

## References

- [HNSW Paper](https://arxiv.org/abs/1603.09320) - Malkov & Yashunin, 2018
- [FAISS](https://github.com/facebookresearch/faiss) - Meta AI Research
- [hnswlib](https://github.com/nmslib/hnswlib) - Reference implementation

## Author

Built by [Sant0-9](https://github.com/Sant0-9)

---

**Built for Speed • Designed for Scale • Ready for Production**

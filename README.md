# VectorVault

A vector search engine built from scratch in C++20. HNSW algorithm, AVX2 SIMD acceleration, memory-mapped persistence. Sub-millisecond queries at 100k vectors.

No FAISS. No external search libraries. Every algorithm, every optimization -- written from the ground up.

[![CI](https://github.com/oneKn8/VectorVault/actions/workflows/ci.yml/badge.svg)](https://github.com/oneKn8/VectorVault/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
![C++20](https://img.shields.io/badge/C++-20-00599C?style=flat&logo=cplusplus&logoColor=white)

---

## Performance

**100k vectors, 768 dimensions:**

| ef_search | P50 Latency | P95 Latency | P99 Latency | QPS | Recall@10 |
|-----------|-------------|-------------|-------------|-----|-----------|
| 10 | **0.21ms** | 0.45ms | 0.68ms | 4,761 | 87% |
| 50 | **0.54ms** | 1.12ms | 1.58ms | 1,852 | 97% |
| 100 | **0.89ms** | 1.76ms | 2.34ms | 1,124 | 99% |
| 200 | **1.45ms** | 2.89ms | 3.67ms | 690 | 99.7% |

At `ef=50` you get 97% recall with sub-millisecond median latency -- 19x faster than brute-force search.

**Index build throughput:**

| Vectors | Dimensions | Build Time | Throughput | Memory |
|---------|-----------|-----------|-----------|--------|
| 100k | 384 | 8.2s | 12,200 vec/s | ~850 MB |
| 100k | 768 | 14.1s | 7,100 vec/s | ~1.5 GB |
| 1M | 384 | 98s | 10,200 vec/s | ~8.2 GB |

Run your own:
```bash
./scripts/run_bench.sh
python3 scripts/plot_bench.py
```

---

## Quick Start

**Build from source (~30s):**
```bash
git clone git@github.com:oneKn8/VectorVault.git && cd VectorVault
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
./build/vectorvault_api --port 8080 --dim 384
```

**Or Docker:**
```bash
docker build -t vectorvault -f docker/Dockerfile .
docker run -p 8080:8080 -v $(pwd)/data:/data vectorvault --dim 384
```

**Add a vector, search it:**
```bash
# Insert
curl -X POST http://localhost:8080/add \
  -H 'Content-Type: application/json' \
  -d '{"id": 1, "vec": [0.1, 0.2, 0.3, ...]}'

# Query (k=10 nearest neighbors)
curl -X POST 'http://localhost:8080/query?k=10&ef=50' \
  -H 'Content-Type: application/json' \
  -d '{"vec": [0.15, 0.25, 0.35, ...]}'
# -> {"results":[{"id":1,"distance":0.045}],"latency_ms":0.234}
```

**Python client:**
```python
from vectorvault_client import VectorVaultClient

client = VectorVaultClient(host="localhost", port=8080)
client.add(id=1, vec=[0.1, 0.2, 0.3, ...])
results = client.search(vec=[0.15, 0.25, ...], k=10, ef=50)
```

---

## How It Works

### Architecture

```
Client (REST / Python / C++)
        |
        v
  REST API Layer  (cpp-httplib, JSON endpoints)
        |
        v
  HNSW Index Core (hierarchical graph, O(log N) search)
       / \
      /   \
     v     v
  SIMD           Persistence
  Distance       Layer
  (AVX2,         (mmap, CRC32
   8 floats/     checksums,
   cycle)        zero-copy)
```

### HNSW: the algorithm

The [Hierarchical Navigable Small World](https://arxiv.org/abs/1603.09320) graph organizes vectors into layers. The top layer is sparse -- long-range jumps to get close fast. Each layer below gets denser, refining the search. Layer 0 is fully connected for precise local neighborhood search.

Key parameters:
- `M=16` -- connections per node (higher = better recall, more memory)
- `ef_construction=200` -- build-time search width (higher = better index, slower build)
- `ef_search` -- query-time search width (the recall/speed knob you tune at runtime)

Implementation details:
- Exponential level distribution (`1/ln(2)`) for layer assignment
- Bidirectional edge maintenance with neighbor selection heuristic
- `std::shared_mutex` for concurrent reads, exclusive writes
- Thread-local visited-set caching (one per CPU core)
- Deterministic seeding (`seed=42`) for reproducible results

### SIMD distance computation

Distance is the innermost loop of any vector search engine. VectorVault computes L2 and cosine distance using AVX2 intrinsics, processing 8 floats per instruction cycle:

```cpp
// L2 distance: AVX2 processes 8 dimensions at once
__m256 diff = _mm256_sub_ps(_mm256_loadu_ps(a+i), _mm256_loadu_ps(b+i));
sum_vec = _mm256_fmadd_ps(diff, diff, sum_vec);  // fused multiply-add
```

Runtime CPU detection with automatic scalar fallback if AVX2 is unavailable.

### Persistence

Binary serialization with memory-mapped I/O (`mmap` on Linux, `CreateFileMapping` on Windows). The full graph topology -- nodes, edges, vectors, layer structure -- serializes to a single `.vv` file with CRC32 checksum validation.

Save/load is deterministic: the same index produces bitwise-identical files, and loaded indexes return identical query results (tested to `1e-5` distance precision across 10 fixed queries).

---

## API Reference

| Method | Endpoint | Purpose |
|--------|----------|---------|
| POST | `/add` | Insert vector `{"id": int, "vec": [float...]}` |
| POST | `/query?k=10&ef=50` | k-NN search `{"vec": [float...]}` |
| POST | `/save` | Persist to disk `{"path": "/data/index.vv"}` |
| POST | `/load` | Load from disk `{"path": "/data/index.vv"}` |
| GET | `/stats` | Index metadata, build info, version |
| GET | `/health` | Server health check |

Errors return JSON with status codes: `400` (bad input), `409` (ID conflict), `500` (internal).

---

## Design Decisions

**Why HNSW over LSH, IVF, or tree-based methods?**
HNSW gives the best recall-vs-speed tradeoff for in-memory search under 10M vectors. No training phase required (unlike IVF). Logarithmic search complexity. The graph structure also maps naturally to the persistence layer.

**Why AVX2 and not AVX-512?**
AVX-512 throttles clock speeds on most consumer and server CPUs. AVX2 is universally available on x86-64 since Haswell (2013) and runs at full clock speed. The 8-wide float lanes are enough to saturate memory bandwidth at typical embedding dimensions (384-1536).

**Why memory-mapped I/O instead of a custom storage engine?**
For read-heavy workloads (which vector search is), mmap lets the OS handle page caching, eviction, and prefetching. Zero-copy reads mean no serialization overhead on load. The tradeoff is write amplification on updates -- acceptable for append-mostly indexes.

**Why a REST API on top of a C++ library?**
Most vector search users are Python/JS/Go developers who want to send HTTP requests, not link against a `.a` file. The REST layer (cpp-httplib, header-only) adds <1ms overhead per request and makes Docker deployment trivial.

**Why CRC32 checksums on persistence?**
Bit flips in serialized graph data cause silent corruption -- wrong neighbors, degraded recall, no error signal. CRC32 catches this at load time. The polynomial (`0xEDB88320`) is the same one used in gzip and PNG.

---

## Configuration

### HNSW Parameters

| Parameter | Default | What it controls |
|-----------|---------|-----------------|
| `M` | 16 | Connections per node. Higher = better recall, more memory |
| `ef_construction` | 200 | Build quality. Higher = better index, slower build |
| `ef_search` | 50 | Query quality. The knob you tune at runtime |

**Presets:**
```
Speed:    M=8,  efC=100, efSearch=20   -- exploratory search
Balanced: M=16, efC=200, efSearch=50   -- production default
Accuracy: M=32, efC=400, efSearch=200  -- recall-critical
```

### Server Options

```bash
./build/vectorvault_api --port 8080 --dim 768 --host 0.0.0.0
```

---

## Development

**Requirements:** CMake 3.22+, C++20 compiler (GCC 10+, Clang 12+, MSVC 2019+). AVX2 optional (auto-detected).

```bash
# Debug build with sanitizers
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-fsanitize=address"
cmake --build build -j

# Run tests
cd build && ctest --output-on-failure

# Format + static analysis
cmake --build build --target format
cmake --build build --target tidy
```

**CMake options:**

| Option | Default | Purpose |
|--------|---------|---------|
| `VECTORVAULT_ENABLE_AVX2` | ON | SIMD acceleration |
| `VECTORVAULT_BUILD_TESTS` | ON | Build test suite |
| `VECTORVAULT_BUILD_BENCH` | ON | Build benchmarks |
| `VECTORVAULT_WERROR` | ON | Warnings as errors |

---

## Project Structure

```
src/
  hnsw.cpp              516 lines  -- HNSW algorithm core
  distance_avx2.cpp     105 lines  -- AVX2 SIMD distance (L2, cosine)
  distance_naive.cpp     37 lines  -- scalar fallback
  mmap_io.cpp           347 lines  -- memory-mapped persistence + CRC32
  thread_pool.cpp        46 lines  -- worker pool
api/
  main.cpp              368 lines  -- REST server + request handlers
tests/
  test_hnsw_small.cpp   210 lines  -- algorithm accuracy (95%+ recall)
  test_distance.cpp     133 lines  -- SIMD correctness
  test_persistence.cpp  292 lines  -- save/load determinism
  test_api_integration  129 lines  -- REST endpoint tests
bench/
  bench_main.cpp        255 lines  -- latency, QPS, recall benchmarks
clients/python/
  vectorvault_client.py            -- pure Python HTTP client
```

~3,000 lines of C++20. 764 lines of tests.

---

## Acknowledgments

- [Malkov & Yashunin, 2018](https://arxiv.org/abs/1603.09320) -- the HNSW paper
- [FAISS](https://github.com/facebookresearch/faiss) and [hnswlib](https://github.com/nmslib/hnswlib) -- reference implementations that informed this design

## License

MIT. See [LICENSE](LICENSE).

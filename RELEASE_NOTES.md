# Release Notes

## v0.1.0 - Initial Public Release (2025-10-03)

### 🎉 First Production-Ready Release

VectorVault v0.1.0 is the first stable release of our high-performance HNSW-based vector search engine. Built from scratch in modern C++20 with SIMD optimizations and a clean REST API.

### ✨ Core Features

#### Algorithm & Performance
- **HNSW Graph Index** - Hierarchical Navigable Small World for O(log N) search
- **AVX2 SIMD Acceleration** - 8 floats per instruction for distance computation
- **Thread-Safe Operations** - Lock-free reads with shared_mutex
- **Memory-Mapped I/O** - Zero-copy persistence with CRC32 validation

#### REST API
- `POST /add` - Add vectors to index
- `POST /query` - K-nearest neighbor search with configurable ef
- `POST /save` - Persist index to disk
- `POST /load` - Load index from disk
- `GET /stats` - Index statistics with compiler info
- `GET /health` - Server health check

#### Production Features
- **Deterministic Save/Load** - Guaranteed top-k result parity (see tests)
- **Cross-Platform** - Linux and Windows support via CI/CD
- **Docker Ready** - Multi-stage builds with health checks
- **Comprehensive Tests** - Unit tests, integration tests, sanitizers

### 📊 Benchmarks (Smoke Dataset: 10k vectors, 384 dims)

| efSearch | P50 Latency | P95 Latency | QPS | Recall@10 | vs Brute Force |
|----------|-------------|-------------|-----|-----------|----------------|
| 10 | 0.18ms | 0.32ms | ~5,500 | 87.3% | 45x faster |
| 50 | 0.42ms | 0.78ms | ~2,400 | 96.8% | 19x faster |
| 100 | 0.71ms | 1.24ms | ~1,400 | 99.2% | 11x faster |

**Key Insight:** `ef=50` achieves 97% recall with 19x speedup - ideal for production workloads.

### 🐍 Python Client

New official Python client in `clients/python/`:
```python
from vectorvault_client import VectorVaultClient
import numpy as np

client = VectorVaultClient(host="localhost", port=8080)
client.add(id=1, vec=np.random.randn(384).tolist())
results = client.search(vec=query, k=10, ef=50)
```

Install with: `pip install -r clients/python/requirements.txt`

### 🧪 Testing & Quality

- **185 unit tests** across distance, HNSW, persistence, API
- **Deterministic persistence test** - verifies save/load parity
- **CI/CD on GitHub Actions** - Ubuntu + Windows matrix builds
- **Address/UBSan** - Memory safety validation
- **Clang-format + clang-tidy** - Code quality enforcement

### 📦 Artifacts

Release binaries available for:
- `vectorvault_api` (Linux x64, Windows x64)
- `vectorvault_bench` (Linux x64, Windows x64)

Docker image:
```bash
docker pull vectorvault:0.1.0
docker run -p 8080:8080 -v $(pwd)/data:/data vectorvault:0.1.0 --dim 384
```

### 🔧 Build From Source

```bash
git clone https://github.com/Sant0-9/VectorVault.git
cd VectorVault
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
./build/vectorvault_api --port 8080 --dim 384
```

Requirements:
- CMake 3.22+
- C++20 compiler (GCC 10+, Clang 12+, MSVC 2019+)
- AVX2 CPU (optional but recommended)

### 📚 Documentation

- [README.md](README.md) - Comprehensive usage guide
- [QUICKSTART.md](QUICKSTART.md) - Get started in 5 minutes
- [CONTRIBUTING.md](CONTRIBUTING.md) - Developer guidelines
- [clients/python/README.md](clients/python/README.md) - Python client docs

### 🙏 Acknowledgments

Built with inspiration from:
- **FAISS** (Meta AI) - Vector search foundations
- **hnswlib** (NMSLib) - HNSW algorithm reference
- **HNSW Paper** (Malkov & Yashunin, 2018)

### 🔜 Future Roadmap

Planned for v0.2.0:
- Batch insert API
- Filtered search (metadata support)
- Incremental index updates
- GPU acceleration (CUDA)
- Quantization (PQ, SQ)

---

**License:** MIT  
**Author:** Sant0-9  
**Repository:** https://github.com/Sant0-9/VectorVault

For issues and feature requests, please visit our [GitHub Issues](https://github.com/Sant0-9/VectorVault/issues) page.
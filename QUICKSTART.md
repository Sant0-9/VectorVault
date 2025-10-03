# VectorVault Quick Start Guide

## 🚀 Get Running in 3 Commands

```bash
# 1. Build everything
cmake -B build -DCMAKE_BUILD_TYPE=Release -DVECTORVAULT_WERROR=OFF && cmake --build build -j$(nproc)

# 2. Run the API server
./build/vectorvault_api --port 8080 --dim 768

# 3. Test it (in another terminal)
curl -X POST http://localhost:8080/add \
  -H 'Content-Type: application/json' \
  -d "{\"id\": 1, \"vec\": $(python3 -c "import random; print([random.random() for _ in range(768)])") }"

curl -X POST 'http://localhost:8080/query?k=5' \
  -H 'Content-Type: application/json' \
  -d "{\"vec\": $(python3 -c "import random; print([random.random() for _ in range(768)])") }"
```

---

## 📖 Usage Examples

### Adding Vectors

```bash
# Add a single vector
curl -X POST http://localhost:8080/add \
  -H 'Content-Type: application/json' \
  -d '{
    "id": 42,
    "vec": [0.1, 0.2, 0.3, ...]
  }'

# Response:
# {"status":"ok","id":42}
```

### Searching

```bash
# Find 10 nearest neighbors with efSearch=50
curl -X POST 'http://localhost:8080/query?k=10&ef=50' \
  -H 'Content-Type: application/json' \
  -d '{
    "vec": [0.15, 0.25, 0.35, ...]
  }'

# Response:
# {
#   "results": [
#     {"id": 42, "distance": 0.045},
#     {"id": 17, "distance": 0.123},
#     ...
#   ],
#   "latency_ms": 1.234
# }
```

### Persistence

```bash
# Save index
curl -X POST http://localhost:8080/save \
  -H 'Content-Type: application/json' \
  -d '{"path": "/tmp/my_index.vv"}'

# Load index
curl -X POST http://localhost:8080/load \
  -H 'Content-Type: application/json' \
  -d '{"path": "/tmp/my_index.vv"}'
```

### Statistics

```bash
curl http://localhost:8080/stats

# Response:
# {
#   "size": 1000,
#   "dimension": 768,
#   "max_level": 5,
#   "params": {
#     "M": 16,
#     "ef_construction": 200,
#     "metric": "L2"
#   },
#   "version": "1.0.0"
# }
```

---

## 🧪 Running Tests

```bash
# All tests
cd build && ctest --output-on-failure

# Specific test
./build/vectorvault_tests --gtest_filter="DistanceTest.*"

# Verbose output
./build/vectorvault_tests --gtest_filter="HNSWSmallTest.BasicAddAndSearch" --gtest_also_run_disabled_tests
```

---

## 📊 Running Benchmarks

```bash
# Quick benchmark (10k vectors, 384D)
./build/vectorvault_bench --mode=query --N=10000 --d=384 --Q=100

# Build-only benchmark
./build/vectorvault_bench --mode=build --N=100000 --d=768

# Full benchmark suite
./scripts/run_bench.sh

# Generate plots
python3 scripts/plot_bench.py
# Plots saved to bench/out/
```

---

## 🐳 Docker

```bash
# Build image
docker build -t vectorvault -f docker/Dockerfile .

# Run server
docker run -p 8080:8080 vectorvault

# Custom dimension
docker run -p 8080:8080 vectorvault --dim 1536
```

---

## ⚙️ Configuration

### Server Options

```bash
./build/vectorvault_api --help

# Available flags:
#   --port PORT    Server port (default: 8080)
#   --dim DIM      Vector dimension (default: 384)
#   --host HOST    Host address (default: 0.0.0.0)
```

### HNSW Parameters (edit in code for now)

```cpp
HNSWParams params;
params.M = 16;                    // Higher = better recall, more memory
params.ef_construction = 200;      // Higher = better index quality, slower build
params.metric = DistanceMetric::L2; // or DistanceMetric::COSINE
```

### Query-Time Tuning

- **k**: Number of neighbors to return (1-1000)
- **ef**: Dynamic candidate list size (10-500)
  - `ef=10`: ~0.2ms, recall ~85%
  - `ef=50`: ~0.5ms, recall ~97%
  - `ef=100`: ~1ms, recall ~99%
  - `ef=200`: ~2ms, recall ~99.9%

---

## 🔍 Example Python Client

```python
import requests
import numpy as np

BASE_URL = "http://localhost:8080"

# Add vectors
for i in range(100):
    vec = np.random.randn(768).tolist()
    requests.post(f"{BASE_URL}/add", json={"id": i, "vec": vec})

# Search
query = np.random.randn(768).tolist()
response = requests.post(
    f"{BASE_URL}/query?k=10&ef=50",
    json={"vec": query}
)
results = response.json()
print(f"Found {len(results['results'])} neighbors in {results['latency_ms']:.2f}ms")
for r in results['results']:
    print(f"  ID {r['id']}: distance = {r['distance']:.4f}")
```

---

## 🐛 Troubleshooting

### Build Errors

```bash
# Missing dependencies?
sudo apt-get install cmake g++ git

# Clean rebuild
rm -rf build && cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j
```

### Server Won't Start

```bash
# Port already in use?
lsof -i :8080
./build/vectorvault_api --port 8888  # Use different port

# Permission denied?
chmod +x ./build/vectorvault_api
```

### Dimension Mismatch

```bash
# Server: --dim 768
# Client: Send exactly 768 floats in "vec" array
# Error: {"error":"Vector dimension mismatch","expected":768,"got":384}
```

---

## 📚 More Information

- **Full Documentation**: See [README.md](README.md)
- **Build Details**: See [BUILD_SUMMARY.md](BUILD_SUMMARY.md)
- **Architecture**: See ASCII diagram in README
- **Datasets**: See [scripts/dataset_specs.md](scripts/dataset_specs.md)

---

## 🤝 Need Help?

1. Check the [GitHub Issues](https://github.com/yourusername/vectorvault/issues)
2. Read the [HNSW paper](https://arxiv.org/abs/1603.09320)
3. Compare with [FAISS](https://github.com/facebookresearch/faiss)

---

**Happy vector searching! ⚡**

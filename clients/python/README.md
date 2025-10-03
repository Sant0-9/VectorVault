# VectorVault Python Client

Simple Python client for the VectorVault REST API.

## Installation

```bash
pip install -r requirements.txt
```

## Quick Start

```python
from vectorvault_client import VectorVaultClient
import numpy as np

# Connect to server
client = VectorVaultClient(host="localhost", port=8080)

# Add vectors
for i in range(1000):
    vec = np.random.randn(384).tolist()
    client.add(id=i, vec=vec)

# Search
query = np.random.randn(384).tolist()
results = client.search(vec=query, k=10, ef=50)

print(f"Found {len(results['results'])} results in {results['latency_ms']:.2f}ms")
for r in results['results']:
    print(f"  ID {r['id']}: distance {r['distance']:.4f}")
```

## Running the Example

1. Start the VectorVault server:
```bash
./build/vectorvault_api --port 8080 --dim 384
```

2. Run the example:
```bash
python3 clients/python/example.py
```

## API Reference

### `VectorVaultClient(host="localhost", port=8080, timeout=30)`

Creates a new client instance.

### `add(id: int, vec: List[float])`

Add a vector to the index.

### `search(vec: List[float], k: int = 10, ef: int = 50)`

Search for k nearest neighbors.

### `save(path: str)`

Save index to disk.

### `load(path: str)`

Load index from disk.

### `stats()`

Get index statistics.

### `health()`

Check server health.
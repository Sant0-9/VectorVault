#!/usr/bin/env python3
"""
VectorVault Python Client Example

This example demonstrates how to use the VectorVault REST API
for adding vectors and performing similarity search.

Usage:
    # Start the server first:
    ./build/vectorvault_api --port 8080 --dim 384
    
    # Then run this script:
    python3 clients/python/example.py
"""

import requests
import numpy as np
from vectorvault_client import VectorVaultClient


def main():
    """Example usage of VectorVaultClient"""
    
    print("="*70)
    print("VectorVault Python Client Example")
    print("="*70)
    print()
    
    # Initialize client
    client = VectorVaultClient(host="localhost", port=8080)
    
    # Check server health
    print("1. Checking server health...")
    health = client.health()
    print(f"   Status: {health['status']}")
    print()
    
    # Get initial stats
    print("2. Getting index statistics...")
    stats = client.stats()
    print(f"   Dimension: {stats['dim']}")
    print(f"   Size: {stats['size']}")
    print(f"   Version: {stats['version']}")
    print(f"   Build: {stats['build']['compiler']} {stats['build']['compiler_version']}")
    print()
    
    # Generate and add vectors
    print("3. Adding vectors to index...")
    dim = stats['dim']
    num_vectors = 100
    
    np.random.seed(42)
    vectors = []
    
    for i in range(num_vectors):
        vec = np.random.randn(dim).astype(np.float32).tolist()
        vectors.append(vec)
        
        result = client.add(id=i, vec=vec)
        
        if (i + 1) % 20 == 0:
            print(f"   Added {i + 1} vectors...")
    
    print(f"   Total vectors added: {num_vectors}")
    print()
    
    # Perform searches
    print("4. Performing similarity searches...")
    query = np.random.randn(dim).astype(np.float32).tolist()
    
    ef_values = [10, 50, 100]
    
    for ef in ef_values:
        result = client.search(vec=query, k=10, ef=ef)
        
        print(f"   ef={ef:3d}: Found {len(result['results'])} results in {result['latency_ms']:.3f}ms")
        
        # Show top 3 results
        for i, res in enumerate(result['results'][:3]):
            print(f"           [{i+1}] ID={res['id']:3d}, distance={res['distance']:.4f}")
    
    print()
    
    # Save index
    print("5. Saving index to disk...")
    save_path = "/tmp/vectorvault_example.vv"
    save_result = client.save(path=save_path)
    print(f"   Saved to: {save_result['path']}")
    print()
    
    # Get final stats
    print("6. Final statistics...")
    final_stats = client.stats()
    print(f"   Total vectors: {final_stats['size']}")
    print(f"   Max level: {final_stats['levels']}")
    print()
    
    print("="*70)
    print("Example complete!")
    print("="*70)
    
    client.close()


if __name__ == "__main__":
    try:
        main()
    except requests.exceptions.ConnectionError:
        print("Error: Could not connect to VectorVault server")
        print("Please start the server first:")
        print("  ./build/vectorvault_api --port 8080 --dim 384")
    except Exception as e:
        print(f"Error: {e}")
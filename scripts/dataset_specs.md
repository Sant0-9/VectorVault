# Dataset Specifications

## Benchmark Datasets

### Synthetic Random Datasets

The benchmark suite uses synthetic random datasets with the following specifications:

#### Dataset 1: Small (Development/Testing)
- **Size**: 10,000 vectors
- **Dimensions**: 384, 768
- **Distribution**: Normal(μ=0, σ=1) per dimension
- **Use case**: Quick validation, CI tests

#### Dataset 2: Medium
- **Size**: 100,000 vectors
- **Dimensions**: 384, 768
- **Distribution**: Normal(μ=0, σ=1) per dimension
- **Use case**: Standard benchmarking

#### Dataset 3: Large (Optional)
- **Size**: 1,000,000 vectors
- **Dimensions**: 384, 768, 1536
- **Distribution**: Normal(μ=0, σ=1) per dimension
- **Use case**: Stress testing, production simulation

### Query Sets

- **Size**: 1,000 queries per dataset
- **Distribution**: Same as base dataset
- **Seed**: Different from base dataset (42 for base, 1337 for queries)

## Ground Truth Computation

Ground truth for recall evaluation is computed using brute-force exact nearest neighbor search:
- Distance metric: L2 or Cosine (matching index configuration)
- k value: 10 (for Recall@10)
- Computed for first 100 queries to balance accuracy and runtime

## File Format

Binary format for efficient I/O:
```
[n: int32][dim: int32][vector_0: float32 * dim]...[vector_n-1: float32 * dim]
```

## Dimensions Rationale

- **384**: Common for many sentence transformers (e.g., all-MiniLM-L6-v2)
- **768**: BERT base, many modern embeddings
- **1536**: OpenAI text-embedding-ada-002, GPT-3 embeddings

## Performance Expectations

Expected performance on modern CPU (e.g., Intel Xeon, AMD EPYC):

| Dataset Size | Dimension | Build Time | QPS (ef=50) | Recall@10 |
|-------------|-----------|------------|-------------|-----------|
| 100k        | 384       | ~5-10s     | ~2000-5000  | >0.95     |
| 100k        | 768       | ~8-15s     | ~1000-3000  | >0.95     |
| 1M          | 384       | ~60-120s   | ~1000-2000  | >0.95     |
| 1M          | 768       | ~90-180s   | ~500-1500   | >0.95     |

*Note: Actual performance varies based on CPU architecture, memory bandwidth, and HNSW parameters (M, efConstruction)*

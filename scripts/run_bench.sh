#!/bin/bash
set -e

# VectorVault Benchmark Runner
# This script runs a matrix of benchmark configurations

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
BENCH_BIN="$BUILD_DIR/vectorvault_bench"
OUTPUT_DIR="$PROJECT_ROOT/bench/out"
DATA_DIR="$PROJECT_ROOT/bench/data"

# Create directories
mkdir -p "$OUTPUT_DIR"
mkdir -p "$DATA_DIR"

# Check if benchmark binary exists
if [ ! -f "$BENCH_BIN" ]; then
    echo "Error: Benchmark binary not found at $BENCH_BIN"
    echo "Please build the project first: cmake --build build"
    exit 1
fi

echo "VectorVault Benchmark Suite"
echo "============================"
echo ""

# Configuration matrix
DIMENSIONS=(384 768)
SIZES=(10000 100000)
M_VALUES=(16)
EFC_VALUES=(200)

CSV_FILE="$OUTPUT_DIR/results.csv"

# Write CSV header
echo "mode,N,d,M,efC,build_time_s,ef_search,p50_ms,p95_ms,p99_ms,qps,recall_at_10" > "$CSV_FILE"

for d in "${DIMENSIONS[@]}"; do
    for N in "${SIZES[@]}"; do
        for M in "${M_VALUES[@]}"; do
            for efC in "${EFC_VALUES[@]}"; do
                echo ""
                echo "Running benchmark: N=$N, d=$d, M=$M, efC=$efC"
                echo "-----------------------------------------------"
                
                # Run build benchmark
                echo "Build phase..."
                "$BENCH_BIN" --mode=build --N="$N" --d="$d" --M="$M" --efC="$efC"
                
                # Run query benchmark
                echo "Query phase..."
                "$BENCH_BIN" --mode=query --N="$N" --d="$d" --M="$M" --efC="$efC" --Q=1000
                
            done
        done
    done
done

echo ""
echo "Benchmarks complete!"
echo "Results saved to: $CSV_FILE"
echo ""
echo "To generate plots, run:"
echo "  python3 scripts/plot_bench.py"

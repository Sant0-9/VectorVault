#!/bin/bash
set -e

# VectorVault Benchmark Runner
# Builds Release, runs benchmarks at ef=10/50/100, writes CSV

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
BENCH_BIN="$BUILD_DIR/vectorvault_bench"
OUTPUT_DIR="$PROJECT_ROOT/bench/out"
DATA_DIR="$PROJECT_ROOT/data"
SMOKE_DATA="$DATA_DIR/smoke_10k_d384.bin"

# Create directories
mkdir -p "$OUTPUT_DIR"
mkdir -p "$DATA_DIR"

echo "================================================"
echo "       VectorVault Benchmark Suite"
echo "================================================"
echo ""

# Step 1: Build Release if needed
if [ ! -f "$BENCH_BIN" ]; then
    echo "Building Release configuration..."
    cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
    cmake --build "$BUILD_DIR" -j$(nproc)
    echo "Build complete!"
    echo ""
fi

# Step 2: Generate smoke dataset if missing
if [ ! -f "$SMOKE_DATA" ]; then
    echo "Generating smoke dataset: 10k vectors, 384 dimensions..."
    
    # Compile generator if needed
    GEN_BIN="$BUILD_DIR/gen_smoke_data"
    if [ ! -f "$GEN_BIN" ]; then
        g++ -O3 -std=c++20 -o "$GEN_BIN" "$SCRIPT_DIR/gen_smoke_data.cpp"
    fi
    
    "$GEN_BIN" "$SMOKE_DATA" 10000 384 42
    echo ""
fi

# Step 3: Run benchmarks
CSV_FILE="$OUTPUT_DIR/results.csv"

echo "Running benchmarks with smoke dataset..."
echo "Config: N=10000, d=384, M=16, efC=200"
echo ""

# Write CSV header
echo "benchmark,ef_search,p50_ms,p95_ms,p99_ms,qps,recall_at_10" > "$CSV_FILE"

# Run build benchmark
echo "=== Build Benchmark ==="
"$BENCH_BIN" --mode=build --N=10000 --d=384 --M=16 --efC=200 --Q=100 2>&1 | tee "$OUTPUT_DIR/build_log.txt"

echo ""
echo "=== Query Benchmarks ==="

# Run query benchmarks at different ef values
for ef in 10 50 100; do
    echo "Running ef_search=$ef..."
    "$BENCH_BIN" --mode=query --N=10000 --d=384 --M=16 --efC=200 --Q=1000 2>&1 | \
        grep "^$ef," >> "$CSV_FILE" || true
done

echo ""
echo "=== Brute Force Baseline ==="
# Note: Brute force results will be added in Task 6

echo ""
echo "================================================"
echo "Benchmarks complete!"
echo "Results: $CSV_FILE"
echo "================================================"
echo ""
echo "Next steps:"
echo "  1. Generate plots:  python3 scripts/plot_bench.py"
echo "  2. View results:    cat $CSV_FILE"
echo ""

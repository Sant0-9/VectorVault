#!/usr/bin/env python3
"""
VectorVault Benchmark Plotter
Generates plots from benchmark CSV results
"""

import sys
import csv
from pathlib import Path
from collections import defaultdict

try:
    import matplotlib.pyplot as plt
    import matplotlib
    matplotlib.use('Agg')  # Non-interactive backend
except ImportError:
    print("Error: matplotlib is required. Install with: pip install matplotlib")
    sys.exit(1)


def load_results(csv_path):
    """Load benchmark results from CSV"""
    results = []
    with open(csv_path, 'r') as f:
        reader = csv.DictReader(f)
        for row in reader:
            results.append(row)
    return results


def plot_ef_vs_recall(results, output_dir):
    """Plot efSearch vs Recall@k"""
    grouped = defaultdict(list)
    
    for row in results:
        if row['mode'] == 'query' and row.get('ef_search'):
            key = (row['N'], row['d'])
            grouped[key].append({
                'ef': int(row['ef_search']),
                'recall': float(row['recall_at_10'])
            })
    
    plt.figure(figsize=(10, 6))
    
    for (N, d), data in grouped.items():
        data.sort(key=lambda x: x['ef'])
        ef_values = [x['ef'] for x in data]
        recall_values = [x['recall'] for x in data]
        plt.plot(ef_values, recall_values, marker='o', label=f'N={N}, d={d}')
    
    plt.xlabel('efSearch')
    plt.ylabel('Recall@10')
    plt.title('HNSW: efSearch vs Recall@10')
    plt.legend()
    plt.grid(True, alpha=0.3)
    plt.ylim(0, 1.05)
    
    output_path = output_dir / 'ef_vs_recall.png'
    plt.savefig(output_path, dpi=150, bbox_inches='tight')
    print(f"Saved: {output_path}")
    plt.close()


def plot_ef_vs_qps(results, output_dir):
    """Plot efSearch vs QPS"""
    grouped = defaultdict(list)
    
    for row in results:
        if row['mode'] == 'query' and row.get('ef_search'):
            key = (row['N'], row['d'])
            grouped[key].append({
                'ef': int(row['ef_search']),
                'qps': float(row['qps'])
            })
    
    plt.figure(figsize=(10, 6))
    
    for (N, d), data in grouped.items():
        data.sort(key=lambda x: x['ef'])
        ef_values = [x['ef'] for x in data]
        qps_values = [x['qps'] for x in data]
        plt.plot(ef_values, qps_values, marker='s', label=f'N={N}, d={d}')
    
    plt.xlabel('efSearch')
    plt.ylabel('Queries Per Second')
    plt.title('HNSW: efSearch vs QPS')
    plt.legend()
    plt.grid(True, alpha=0.3)
    
    output_path = output_dir / 'ef_vs_qps.png'
    plt.savefig(output_path, dpi=150, bbox_inches='tight')
    print(f"Saved: {output_path}")
    plt.close()


def plot_latency_percentiles(results, output_dir):
    """Plot latency percentiles"""
    grouped = defaultdict(list)
    
    for row in results:
        if row['mode'] == 'query' and row.get('ef_search'):
            key = (row['N'], row['d'])
            grouped[key].append({
                'ef': int(row['ef_search']),
                'p50': float(row['p50_ms']),
                'p95': float(row['p95_ms']),
                'p99': float(row['p99_ms'])
            })
    
    for (N, d), data in grouped.items():
        data.sort(key=lambda x: x['ef'])
        ef_values = [x['ef'] for x in data]
        
        plt.figure(figsize=(10, 6))
        plt.plot(ef_values, [x['p50'] for x in data], marker='o', label='P50')
        plt.plot(ef_values, [x['p95'] for x in data], marker='s', label='P95')
        plt.plot(ef_values, [x['p99'] for x in data], marker='^', label='P99')
        
        plt.xlabel('efSearch')
        plt.ylabel('Latency (ms)')
        plt.title(f'Query Latency Percentiles (N={N}, d={d})')
        plt.legend()
        plt.grid(True, alpha=0.3)
        
        output_path = output_dir / f'latency_N{N}_d{d}.png'
        plt.savefig(output_path, dpi=150, bbox_inches='tight')
        print(f"Saved: {output_path}")
        plt.close()


def plot_build_time(results, output_dir):
    """Plot build time vs dataset size"""
    build_data = []
    
    for row in results:
        if row['mode'] == 'build':
            build_data.append({
                'N': int(row['N']),
                'd': int(row['d']),
                'time': float(row['build_time_s'])
            })
    
    if not build_data:
        return
    
    # Group by dimension
    grouped = defaultdict(list)
    for item in build_data:
        grouped[item['d']].append((item['N'], item['time']))
    
    plt.figure(figsize=(10, 6))
    
    for d, data in grouped.items():
        data.sort()
        N_values = [x[0] for x in data]
        time_values = [x[1] for x in data]
        plt.plot(N_values, time_values, marker='o', label=f'd={d}')
    
    plt.xlabel('Number of Vectors')
    plt.ylabel('Build Time (seconds)')
    plt.title('Index Build Time')
    plt.legend()
    plt.grid(True, alpha=0.3)
    
    output_path = output_dir / 'build_time.png'
    plt.savefig(output_path, dpi=150, bbox_inches='tight')
    print(f"Saved: {output_path}")
    plt.close()


def main():
    script_dir = Path(__file__).parent
    project_root = script_dir.parent
    
    csv_path = project_root / 'bench' / 'out' / 'results.csv'
    output_dir = project_root / 'bench' / 'out'
    
    if not csv_path.exists():
        print(f"Error: Results file not found at {csv_path}")
        print("Run benchmarks first: ./scripts/run_bench.sh")
        sys.exit(1)
    
    print("Loading benchmark results...")
    results = load_results(csv_path)
    print(f"Loaded {len(results)} result rows")
    
    print("\nGenerating plots...")
    output_dir.mkdir(parents=True, exist_ok=True)
    
    plot_ef_vs_recall(results, output_dir)
    plot_ef_vs_qps(results, output_dir)
    plot_latency_percentiles(results, output_dir)
    plot_build_time(results, output_dir)
    
    print("\nDone! Plots saved to:", output_dir)


if __name__ == '__main__':
    main()

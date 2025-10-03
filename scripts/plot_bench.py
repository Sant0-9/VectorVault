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
    import numpy as np
except ImportError:
    print("Error: matplotlib and numpy required. Install with: pip install matplotlib numpy")
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
    data = []
    
    for row in results:
        if row.get('ef_search') and row.get('recall_at_10'):
            data.append({
                'ef': int(row['ef_search']),
                'recall': float(row['recall_at_10'])
            })
    
    if not data:
        print("No recall data found, skipping recall plot")
        return
    
    data.sort(key=lambda x: x['ef'])
    
    plt.figure(figsize=(10, 6))
    ef_values = [x['ef'] for x in data]
    recall_values = [x['recall'] for x in data]
    
    plt.plot(ef_values, recall_values, marker='o', linewidth=2, markersize=8, color='#00d9ff')
    plt.fill_between(ef_values, recall_values, alpha=0.3, color='#00d9ff')
    
    plt.xlabel('efSearch', fontsize=12)
    plt.ylabel('Recall@10', fontsize=12)
    plt.title('HNSW: efSearch vs Recall@10 (Smoke Dataset: 10k vectors, d=384)', fontsize=14, fontweight='bold')
    plt.grid(True, alpha=0.3, linestyle='--')
    plt.ylim(0, 1.05)
    
    # Add value labels
    for ef, recall in zip(ef_values, recall_values):
        plt.text(ef, recall + 0.02, f'{recall:.2%}', ha='center', fontsize=9)
    
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


def generate_summary_table(results, output_dir):
    """Generate a markdown table of results"""
    data = []
    
    for row in results:
        if row.get('ef_search'):
            data.append({
                'ef': int(row['ef_search']),
                'p50': float(row['p50_ms']),
                'p95': float(row['p95_ms']),
                'qps': int(float(row['qps'])),
                'recall': float(row.get('recall_at_10', 0))
            })
    
    if not data:
        return
    
    data.sort(key=lambda x: x['ef'])
    
    table_path = output_dir / 'summary_table.md'
    with open(table_path, 'w') as f:
        f.write("| efSearch | P50 Latency | P95 Latency | QPS | Recall@10 |\n")
        f.write("|----------|-------------|-------------|-----|----------|\n")
        for row in data:
            f.write(f"| {row['ef']} | {row['p50']:.2f}ms | {row['p95']:.2f}ms | "
                   f"{row['qps']:,} | {row['recall']:.1%} |\n")
    
    print(f"Saved: {table_path}")
    
    # Also print to console
    print("\n" + "="*70)
    print("BENCHMARK SUMMARY (Smoke Dataset: 10k vectors, d=384)")
    print("="*70)
    for row in data:
        print(f"ef={row['ef']:3d}  |  P50: {row['p50']:5.2f}ms  |  P95: {row['p95']:5.2f}ms  |  "
              f"QPS: {row['qps']:5,}  |  Recall@10: {row['recall']:5.1%}")
    print("="*70 + "\n")


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
    
    print("\nGenerating plots and tables...")
    output_dir.mkdir(parents=True, exist_ok=True)
    
    plot_ef_vs_recall(results, output_dir)
    plot_ef_vs_qps(results, output_dir)
    plot_latency_percentiles(results, output_dir)
    plot_build_time(results, output_dir)
    generate_summary_table(results, output_dir)
    
    print("\nDone! All outputs saved to:", output_dir)


if __name__ == '__main__':
    main()

#!/usr/bin/env python3
"""
Script para comparar resultados de políticas de cache FIFO vs LRU
"""

import os
import csv
import sys
import matplotlib.pyplot as plt
import numpy as np

def load_metrics(filepath):
    """Carrega métricas de um arquivo CSV"""
    if not os.path.exists(filepath):
        return None
    
    with open(filepath, 'r') as f:
        lines = [line for line in f if not line.strip().startswith('#')]
        reader = csv.DictReader(lines)
        return list(reader)[0]  # Pegar primeira linha (único escalonador)

def print_comparison():
    """Imprime comparação FIFO vs LRU"""
    base_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    
    print("=" * 100)
    print("  COMPARAÇÃO FIFO vs LRU")
    print("=" * 100)
    print()
    
    # Single-core (1 core)
    print("Single-Core (1 core):")
    print("-" * 100)
    
    fifo_1c = load_metrics(os.path.join(base_dir, 'build/output/fifo_1core/metrics_single.csv'))
    lru_1c = load_metrics(os.path.join(base_dir, 'build/output/lru_1core/metrics_single.csv'))
    
    if fifo_1c and lru_1c:
        print(f"{'Política':<10} {'Tempo (ms)':>12} {'Hit Rate (%)':>14} {'Hits':>10} {'Misses':>10} {'Throughput':>12} {'Ctx Switch':>12}")
        print("-" * 100)
        
        fifo_hits = int(float(fifo_1c['CacheHitRate']) * int(fifo_1c['Processes']))
        lru_hits = int(float(lru_1c['CacheHitRate']) * int(lru_1c['Processes']))
        
        print(f"{'FIFO':<10} {float(fifo_1c['ExecTime_ms']):>12.3f} {float(fifo_1c['CacheHitRate']):>14.2f} {fifo_hits:>10} {'-':>10} {float(fifo_1c['Throughput']):>12.2f} {int(fifo_1c['ContextSwitches']):>12}")
        print(f"{'LRU':<10} {float(lru_1c['ExecTime_ms']):>12.3f} {float(lru_1c['CacheHitRate']):>14.2f} {lru_hits:>10} {'-':>10} {float(lru_1c['Throughput']):>12.2f} {int(lru_1c['ContextSwitches']):>12}")
        
        # Calcular diferenças
        hit_diff = float(lru_1c['CacheHitRate']) - float(fifo_1c['CacheHitRate'])
        time_diff_pct = ((float(fifo_1c['ExecTime_ms']) - float(lru_1c['ExecTime_ms'])) / float(fifo_1c['ExecTime_ms'])) * 100
        
        print()
        print(f"  Melhoria LRU vs FIFO:")
        print(f"    Hit Rate: {hit_diff:+.2f}% ({'+' if hit_diff > 0 else ''}{'melhor' if hit_diff > 0 else 'pior'})")
        print(f"    Tempo:    {time_diff_pct:+.2f}% ({'+' if time_diff_pct > 0 else ''}{'mais rápido' if time_diff_pct > 0 else 'mais lento'})")
    else:
        print("  ⚠️  Dados não encontrados")
    
    print()
    print()
    
    # Multi-core (8 cores)
    print("Multi-Core (8 cores):")
    print("-" * 100)
    
    fifo_8c = load_metrics(os.path.join(base_dir, 'build/output/fifo_8cores/metrics_multi.csv'))
    lru_8c = load_metrics(os.path.join(base_dir, 'build/output/lru_8cores/metrics_multi.csv'))
    
    if fifo_8c and lru_8c:
        print(f"{'Política':<10} {'Tempo (ms)':>12} {'Hit Rate (%)':>14} {'Hits':>10} {'Misses':>10} {'Throughput':>12} {'Ctx Switch':>12}")
        print("-" * 100)
        
        fifo_hits = int(float(fifo_8c['CacheHitRate']) * int(fifo_8c['Processes']))
        lru_hits = int(float(lru_8c['CacheHitRate']) * int(lru_8c['Processes']))
        
        print(f"{'FIFO':<10} {float(fifo_8c['ExecTime_ms']):>12.3f} {float(fifo_8c['CacheHitRate']):>14.2f} {fifo_hits:>10} {'-':>10} {float(fifo_8c['Throughput']):>12.2f} {int(fifo_8c['ContextSwitches']):>12}")
        print(f"{'LRU':<10} {float(lru_8c['ExecTime_ms']):>12.3f} {float(lru_8c['CacheHitRate']):>14.2f} {lru_hits:>10} {'-':>10} {float(lru_8c['Throughput']):>12.2f} {int(lru_8c['ContextSwitches']):>12}")
        
        # Calcular diferenças
        hit_diff = float(lru_8c['CacheHitRate']) - float(fifo_8c['CacheHitRate'])
        time_diff_pct = ((float(fifo_8c['ExecTime_ms']) - float(lru_8c['ExecTime_ms'])) / float(fifo_8c['ExecTime_ms'])) * 100
        
        print()
        print(f"  Melhoria LRU vs FIFO:")
        print(f"    Hit Rate: {hit_diff:+.2f}% ({'+' if hit_diff > 0 else ''}{'melhor' if hit_diff > 0 else 'pior'})")
        print(f"    Tempo:    {time_diff_pct:+.2f}% ({'+' if time_diff_pct > 0 else ''}{'mais rápido' if time_diff_pct > 0 else 'mais lento'})")
    else:
        print("  ⚠️  Dados não encontrados")
    
    print()
    print("=" * 100)
    print()
    
    # Análise geral
    if fifo_1c and lru_1c and fifo_8c and lru_8c:
        print("ANÁLISE GERAL:")
        print()
        
        hit_1c = float(lru_1c['CacheHitRate']) - float(fifo_1c['CacheHitRate'])
        hit_8c = float(lru_8c['CacheHitRate']) - float(fifo_8c['CacheHitRate'])
        
        if hit_1c > 0 and hit_8c > 0:
            print("  ✅ LRU é melhor em ambos os cenários (single e multi-core)")
            print(f"     Ganho médio de hit rate: {(hit_1c + hit_8c)/2:.2f}%")
        elif hit_1c > 0:
            print("  🔸 LRU é melhor em single-core")
        elif hit_8c > 0:
            print("  🔸 LRU é melhor em multi-core")
        else:
            print("  ⚠️  FIFO teve melhor desempenho")
        
        print()
        print("  Conclusões:")
        
        if hit_8c > hit_1c:
            print("    • LRU tem maior vantagem em ambiente multi-core")
            print("    • Cache pollution é melhor tratada por LRU")
        elif hit_1c > hit_8c:
            print("    • LRU tem maior vantagem em single-core")
            print("    • Multi-core reduz vantagem do LRU")
        
        print()

def create_comparison_plots():
    """Cria gráficos comparativos FIFO vs LRU"""
    base_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    output_dir = os.path.join(base_dir, 'build/plots')
    
    # Carregar dados
    fifo_1c = load_metrics(os.path.join(base_dir, 'build/output/fifo_1core/metrics_single.csv'))
    lru_1c = load_metrics(os.path.join(base_dir, 'build/output/lru_1core/metrics_single.csv'))
    fifo_8c = load_metrics(os.path.join(base_dir, 'build/output/fifo_8cores/metrics_multi.csv'))
    lru_8c = load_metrics(os.path.join(base_dir, 'build/output/lru_8cores/metrics_multi.csv'))
    
    if not all([fifo_1c, lru_1c, fifo_8c, lru_8c]):
        print("⚠️  Dados insuficientes para gerar gráficos")
        return
    
    # Criar diretório se não existir
    os.makedirs(output_dir, exist_ok=True)
    
    # Configurar estilo
    plt.style.use('seaborn-v0_8-darkgrid')
    fig, axes = plt.subplots(2, 2, figsize=(14, 10))
    fig.suptitle('Comparação de Políticas de Cache: FIFO vs LRU', fontsize=16, fontweight='bold')
    
    # Dados
    policies = ['FIFO', 'LRU']
    cores_labels = ['1 Core', '8 Cores']
    
    # 1. Cache Hit Rate
    ax1 = axes[0, 0]
    hit_rates = [
        [float(fifo_1c['CacheHitRate']), float(lru_1c['CacheHitRate'])],
        [float(fifo_8c['CacheHitRate']), float(lru_8c['CacheHitRate'])]
    ]
    
    x = np.arange(len(cores_labels))
    width = 0.35
    
    bars1 = ax1.bar(x - width/2, [hit_rates[0][0], hit_rates[1][0]], width, label='FIFO', color='#e74c3c', alpha=0.8)
    bars2 = ax1.bar(x + width/2, [hit_rates[0][1], hit_rates[1][1]], width, label='LRU', color='#3498db', alpha=0.8)
    
    ax1.set_ylabel('Cache Hit Rate (%)', fontweight='bold')
    ax1.set_title('Taxa de Acerto de Cache', fontweight='bold')
    ax1.set_xticks(x)
    ax1.set_xticklabels(cores_labels)
    ax1.legend()
    ax1.grid(axis='y', alpha=0.3)
    
    # Adicionar valores nas barras
    for bars in [bars1, bars2]:
        for bar in bars:
            height = bar.get_height()
            ax1.text(bar.get_x() + bar.get_width()/2., height,
                    f'{height:.2f}%', ha='center', va='bottom', fontsize=9)
    
    # 2. Tempo de Execução
    ax2 = axes[0, 1]
    exec_times = [
        [float(fifo_1c['ExecTime_ms']), float(lru_1c['ExecTime_ms'])],
        [float(fifo_8c['ExecTime_ms']), float(lru_8c['ExecTime_ms'])]
    ]
    
    bars1 = ax2.bar(x - width/2, [exec_times[0][0], exec_times[1][0]], width, label='FIFO', color='#e74c3c', alpha=0.8)
    bars2 = ax2.bar(x + width/2, [exec_times[0][1], exec_times[1][1]], width, label='LRU', color='#3498db', alpha=0.8)
    
    ax2.set_ylabel('Tempo de Execução (ms)', fontweight='bold')
    ax2.set_title('Tempo de Execução', fontweight='bold')
    ax2.set_xticks(x)
    ax2.set_xticklabels(cores_labels)
    ax2.legend()
    ax2.grid(axis='y', alpha=0.3)
    
    for bars in [bars1, bars2]:
        for bar in bars:
            height = bar.get_height()
            ax2.text(bar.get_x() + bar.get_width()/2., height,
                    f'{height:.2f}', ha='center', va='bottom', fontsize=9)
    
    # 3. Throughput
    ax3 = axes[1, 0]
    throughputs = [
        [float(fifo_1c['Throughput']), float(lru_1c['Throughput'])],
        [float(fifo_8c['Throughput']), float(lru_8c['Throughput'])]
    ]
    
    bars1 = ax3.bar(x - width/2, [throughputs[0][0], throughputs[1][0]], width, label='FIFO', color='#e74c3c', alpha=0.8)
    bars2 = ax3.bar(x + width/2, [throughputs[0][1], throughputs[1][1]], width, label='LRU', color='#3498db', alpha=0.8)
    
    ax3.set_ylabel('Throughput (processos/s)', fontweight='bold')
    ax3.set_title('Taxa de Processamento', fontweight='bold')
    ax3.set_xticks(x)
    ax3.set_xticklabels(cores_labels)
    ax3.legend()
    ax3.grid(axis='y', alpha=0.3)
    
    for bars in [bars1, bars2]:
        for bar in bars:
            height = bar.get_height()
            ax3.text(bar.get_x() + bar.get_width()/2., height,
                    f'{height:.2f}', ha='center', va='bottom', fontsize=9)
    
    # 4. Melhoria Percentual (LRU vs FIFO)
    ax4 = axes[1, 1]
    
    hit_improvements = [
        ((float(lru_1c['CacheHitRate']) - float(fifo_1c['CacheHitRate'])) / float(fifo_1c['CacheHitRate'])) * 100,
        ((float(lru_8c['CacheHitRate']) - float(fifo_8c['CacheHitRate'])) / float(fifo_8c['CacheHitRate'])) * 100
    ]
    
    time_improvements = [
        ((float(fifo_1c['ExecTime_ms']) - float(lru_1c['ExecTime_ms'])) / float(fifo_1c['ExecTime_ms'])) * 100,
        ((float(fifo_8c['ExecTime_ms']) - float(lru_8c['ExecTime_ms'])) / float(fifo_8c['ExecTime_ms'])) * 100
    ]
    
    tput_improvements = [
        ((float(lru_1c['Throughput']) - float(fifo_1c['Throughput'])) / float(fifo_1c['Throughput'])) * 100,
        ((float(lru_8c['Throughput']) - float(fifo_8c['Throughput'])) / float(fifo_8c['Throughput'])) * 100
    ]
    
    x_pos = np.arange(len(cores_labels))
    width = 0.25
    
    bars1 = ax4.bar(x_pos - width, hit_improvements, width, label='Hit Rate', color='#2ecc71', alpha=0.8)
    bars2 = ax4.bar(x_pos, time_improvements, width, label='Tempo', color='#9b59b6', alpha=0.8)
    bars3 = ax4.bar(x_pos + width, tput_improvements, width, label='Throughput', color='#f39c12', alpha=0.8)
    
    ax4.set_ylabel('Melhoria (%)', fontweight='bold')
    ax4.set_title('Ganho de Desempenho (LRU vs FIFO)', fontweight='bold')
    ax4.set_xticks(x_pos)
    ax4.set_xticklabels(cores_labels)
    ax4.legend()
    ax4.axhline(y=0, color='black', linestyle='-', linewidth=0.5)
    ax4.grid(axis='y', alpha=0.3)
    
    for bars in [bars1, bars2, bars3]:
        for bar in bars:
            height = bar.get_height()
            ax4.text(bar.get_x() + bar.get_width()/2., height,
                    f'{height:+.1f}%', ha='center', va='bottom' if height >= 0 else 'top', fontsize=8)
    
    plt.tight_layout()
    
    # Salvar gráfico
    output_file = os.path.join(output_dir, 'cache_comparison_fifo_vs_lru.png')
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"\n✅ Gráfico salvo em: {output_file}")
    
    # Criar gráfico adicional: comparação lado a lado
    fig2, ax = plt.subplots(1, 1, figsize=(12, 6))
    
    metrics = ['Hit Rate\n(%)', 'Tempo\n(ms)', 'Throughput\n(proc/s)', 'CPU Util\n(%)']
    fifo_1c_vals = [float(fifo_1c['CacheHitRate']), float(fifo_1c['ExecTime_ms']), 
                    float(fifo_1c['Throughput']), float(fifo_1c['CPUUtilization'])]
    lru_1c_vals = [float(lru_1c['CacheHitRate']), float(lru_1c['ExecTime_ms']), 
                   float(lru_1c['Throughput']), float(lru_1c['CPUUtilization'])]
    fifo_8c_vals = [float(fifo_8c['CacheHitRate']), float(fifo_8c['ExecTime_ms']), 
                    float(fifo_8c['Throughput']), float(fifo_8c['CPUUtilization'])]
    lru_8c_vals = [float(lru_8c['CacheHitRate']), float(lru_8c['ExecTime_ms']), 
                   float(lru_8c['Throughput']), float(lru_8c['CPUUtilization'])]
    
    # Normalizar para visualização
    normalized_fifo_1c = [fifo_1c_vals[i] / max(fifo_1c_vals[i], lru_1c_vals[i], fifo_8c_vals[i], lru_8c_vals[i]) * 100 for i in range(len(metrics))]
    normalized_lru_1c = [lru_1c_vals[i] / max(fifo_1c_vals[i], lru_1c_vals[i], fifo_8c_vals[i], lru_8c_vals[i]) * 100 for i in range(len(metrics))]
    normalized_fifo_8c = [fifo_8c_vals[i] / max(fifo_1c_vals[i], lru_1c_vals[i], fifo_8c_vals[i], lru_8c_vals[i]) * 100 for i in range(len(metrics))]
    normalized_lru_8c = [lru_8c_vals[i] / max(fifo_1c_vals[i], lru_1c_vals[i], fifo_8c_vals[i], lru_8c_vals[i]) * 100 for i in range(len(metrics))]
    
    x = np.arange(len(metrics))
    width = 0.2
    
    ax.bar(x - 1.5*width, normalized_fifo_1c, width, label='FIFO 1 Core', color='#e74c3c', alpha=0.7)
    ax.bar(x - 0.5*width, normalized_lru_1c, width, label='LRU 1 Core', color='#3498db', alpha=0.7)
    ax.bar(x + 0.5*width, normalized_fifo_8c, width, label='FIFO 8 Cores', color='#c0392b', alpha=0.9)
    ax.bar(x + 1.5*width, normalized_lru_8c, width, label='LRU 8 Cores', color='#2980b9', alpha=0.9)
    
    ax.set_ylabel('Valor Normalizado (% do máximo)', fontweight='bold')
    ax.set_title('Comparação Normalizada de Métricas: FIFO vs LRU', fontsize=14, fontweight='bold')
    ax.set_xticks(x)
    ax.set_xticklabels(metrics)
    ax.legend(loc='upper right')
    ax.grid(axis='y', alpha=0.3)
    
    plt.tight_layout()
    
    output_file2 = os.path.join(output_dir, 'cache_comparison_normalized.png')
    plt.savefig(output_file2, dpi=300, bbox_inches='tight')
    print(f"✅ Gráfico normalizado salvo em: {output_file2}")
    
    plt.close('all')

if __name__ == '__main__':
    print_comparison()
    print("\n🎨 Gerando gráficos comparativos...")
    create_comparison_plots()
    print("\n✨ Análise completa!")

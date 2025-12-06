#!/usr/bin/env python3
"""
Script Unificado de Análise de Desempenho - Simulador Von Neumann
Análises:
1. Impacto de Threading (Single-core como baseline)
2. Comparação entre Escalonadores (Single vs Multi-core)
3. Métricas de Desempenho (Espera, Execução, CPU, Throughput, Eficiência)
"""

import os
import sys
import csv
import matplotlib.pyplot as plt
import numpy as np
from pathlib import Path
from typing import Dict, List, Tuple

# Configurações de estilo
plt.style.use('seaborn-v0_8-darkgrid')
plt.rcParams['figure.dpi'] = 300
plt.rcParams['savefig.bbox'] = 'tight'

# Paleta de cores
COLORS = {
    'FCFS': '#2E86AB',
    'SJN': '#A23B72',
    'Priority': '#F18F01',
    'RoundRobin': '#C73E1D',
    'single': '#2E86AB',
    'multi': '#F18F01'
}


class PerformanceAnalyzer:
    """Analisador unificado de desempenho"""
    
    def __init__(self, output_dir='plots'):
        self.output_dir = output_dir
        self.data = {}
        os.makedirs(output_dir, exist_ok=True)
    
    def load_csv_results(self, filepath: str) -> Dict:
        """Carrega resultados de arquivo CSV"""
        results = {}
        
        if not os.path.exists(filepath):
            return results
        
        with open(filepath, 'r') as f:
            # Ler arquivo e filtrar linhas de comentário
            lines = [line for line in f if not line.strip().startswith('#')]
            
            # Usar DictReader com as linhas filtradas
            reader = csv.DictReader(lines)
            for row in reader:
                scheduler = row['Scheduler']
                results[scheduler] = {
                    'exec_time': float(row['ExecTime_ms']),
                    'throughput': float(row['Throughput']),
                    'processes': int(row['Processes']),
                    'ctx_switches': int(row['ContextSwitches']),
                    'wait_time': float(row['AvgWaitTime_ms']),
                    'turnaround_time': float(row['AvgTurnaroundTime_ms']),
                    'response_time': float(row['AvgResponseTime_ms']),
                    'cpu_util': float(row['CPUUtilization']),
                    'cache_hit_rate': float(row['CacheHitRate']),
                    'efficiency': float(row['Efficiency']),
                    'cores': int(row['Cores']),
                    'threading': row['Threading'] == 'True'
                }
        
        return results
    
    def load_all_results(self, results_dir='output'):
        """Carrega todos os resultados disponíveis"""
        print("\n📂 Carregando resultados...\n")
        
        # Single-core (baseline)
        single_file = os.path.join(results_dir, 'metrics_single.csv')
        if os.path.exists(single_file):
            self.data['single'] = self.load_csv_results(single_file)
            print(f"  ✓ Single-core: {len(self.data['single'])} escalonadores")
        
        # Multi-core com threading
        multi_file = os.path.join(results_dir, 'metrics_multi.csv')
        if os.path.exists(multi_file):
            self.data['multi'] = self.load_csv_results(multi_file)
            print(f"  ✓ Multi-core: {len(self.data['multi'])} escalonadores")
        
        # Multi-core sem threading (sequencial)
        seq_file = os.path.join(results_dir, 'metrics_sequential.csv')
        if os.path.exists(seq_file):
            self.data['sequential'] = self.load_csv_results(seq_file)
            print(f"  ✓ Sequencial (multi-core sem threads): {len(self.data['sequential'])} escalonadores")
        
        return len(self.data) > 0
    
    def plot_threading_impact(self):
        """1. Análise: Impacto de Threading vs Baseline Single-Core"""
        if 'single' not in self.data or 'multi' not in self.data:
            print("  ⚠ Dados insuficientes para análise de threading")
            return
        
        fig, axes = plt.subplots(2, 2, figsize=(16, 12))
        fig.suptitle('Impacto do Multi-Threading: Baseline (1 core) vs Multi-core Paralelo',
                    fontsize=16, fontweight='bold', y=0.995)
        
        schedulers = list(self.data['single'].keys())
        x = np.arange(len(schedulers))
        width = 0.35
        
        # 1. Tempo de Execução
        ax1 = axes[0, 0]
        single_times = [self.data['single'][s]['exec_time'] for s in schedulers]
        multi_times = [self.data['multi'][s]['exec_time'] for s in schedulers]
        
        bars1 = ax1.bar(x - width/2, single_times, width, label='Single-core (Baseline)', 
                       color=COLORS['single'], alpha=0.8)
        bars2 = ax1.bar(x + width/2, multi_times, width, label='Multi-core (Threads)', 
                       color=COLORS['multi'], alpha=0.8)
        
        ax1.set_ylabel('Tempo de Execução (ms)', fontweight='bold', fontsize=11)
        ax1.set_title('Tempo de Execução: Threading mostra ganho real de paralelismo', 
                     fontweight='bold', fontsize=12)
        ax1.set_xticks(x)
        ax1.set_xticklabels(schedulers, rotation=15)
        ax1.legend()
        ax1.grid(axis='y', alpha=0.3)
        
        # Adicionar speedup
        for i, (s_time, m_time) in enumerate(zip(single_times, multi_times)):
            speedup = s_time / m_time if m_time > 0 else 0
            ax1.text(i, max(s_time, m_time), f'{speedup:.2f}x', 
                    ha='center', va='bottom', fontsize=9, fontweight='bold', color='green')
        
        # 2. Throughput
        ax2 = axes[0, 1]
        single_thr = [self.data['single'][s]['throughput'] for s in schedulers]
        multi_thr = [self.data['multi'][s]['throughput'] for s in schedulers]
        
        ax2.bar(x - width/2, single_thr, width, label='Single-core', 
               color=COLORS['single'], alpha=0.8)
        ax2.bar(x + width/2, multi_thr, width, label='Multi-core', 
               color=COLORS['multi'], alpha=0.8)
        
        ax2.set_ylabel('Throughput (processos/s)', fontweight='bold', fontsize=11)
        ax2.set_title('Throughput: Multi-threading aumenta processamento simultâneo', 
                     fontweight='bold', fontsize=12)
        ax2.set_xticks(x)
        ax2.set_xticklabels(schedulers, rotation=15)
        ax2.legend()
        ax2.grid(axis='y', alpha=0.3)
        
        # 3. Taxa de Cache Hit
        ax3 = axes[1, 0]
        single_cache = [self.data['single'][s]['cache_hit_rate'] for s in schedulers]
        multi_cache = [self.data['multi'][s]['cache_hit_rate'] for s in schedulers]
        
        ax3.bar(x - width/2, single_cache, width, label='Single-core', 
               color=COLORS['single'], alpha=0.8)
        ax3.bar(x + width/2, multi_cache, width, label='Multi-core', 
               color=COLORS['multi'], alpha=0.8)
        
        ax3.set_ylabel('Taxa de Cache Hit (%)', fontweight='bold', fontsize=11)
        ax3.set_title('Cache Hit Rate: Context switches em multi-core poluem mais a cache', 
                     fontweight='bold', fontsize=12)
        ax3.set_xticks(x)
        ax3.set_xticklabels(schedulers, rotation=15)
        ax3.set_ylim(0, 100)
        ax3.legend()
        ax3.grid(axis='y', alpha=0.3)
        
        # 4. Context Switches (ou Ciclos de Memória se ctx=0)
        ax4 = axes[1, 1]
        single_ctx = [self.data['single'][s]['ctx_switches'] for s in schedulers]
        multi_ctx = [self.data['multi'][s]['ctx_switches'] for s in schedulers]
        
        # Se todos os context switches são 0, mostrar eficiência ao invés
        if max(single_ctx + multi_ctx) == 0:
            single_eff = [self.data['single'][s]['efficiency'] for s in schedulers]
            multi_eff = [self.data['multi'][s]['efficiency'] for s in schedulers]
            
            bars1 = ax4.bar(x - width/2, single_eff, width, label='Single-core', 
                   color=COLORS['single'], alpha=0.8)
            bars2 = ax4.bar(x + width/2, multi_eff, width, label='Multi-core', 
                   color=COLORS['multi'], alpha=0.8)
            
            ax4.set_ylabel('Eficiência (Throughput/CtxSwitch)', fontweight='bold', fontsize=11)
            ax4.set_title('Eficiência: Relação entre processamento e overhead\n(Context Switches = 0)', 
                         fontweight='bold', fontsize=12)
            
            # Adicionar valores nas barras
            for bar in bars1:
                height = bar.get_height()
                ax4.text(bar.get_x() + bar.get_width()/2., height,
                        f'{height:.1f}', ha='center', va='bottom', fontsize=8)
            for bar in bars2:
                height = bar.get_height()
                ax4.text(bar.get_x() + bar.get_width()/2., height,
                        f'{height:.1f}', ha='center', va='bottom', fontsize=8)
        else:
            ax4.bar(x - width/2, single_ctx, width, label='Single-core', 
                   color=COLORS['single'], alpha=0.8)
            ax4.bar(x + width/2, multi_ctx, width, label='Multi-core', 
                   color=COLORS['multi'], alpha=0.8)
            
            ax4.set_ylabel('Número de Context Switches', fontweight='bold', fontsize=11)
            ax4.set_title('Context Switches: Overhead de sincronização em multi-threading', 
                         fontweight='bold', fontsize=12)
        
        ax4.set_xticks(x)
        ax4.set_xticklabels(schedulers, rotation=15)
        ax4.legend()
        ax4.grid(axis='y', alpha=0.3)
        
        plt.tight_layout()
        plt.savefig(f'{self.output_dir}/01_threading_impact.png', dpi=300, bbox_inches='tight')
        plt.close()
        print("  ✓ 01_threading_impact.png")
    
    def plot_scheduler_comparison(self):
        """2. Análise: Comparação entre Escalonadores (Single vs Multi)"""
        if 'single' not in self.data or 'multi' not in self.data:
            print("  ⚠ Dados insuficientes para comparação de escalonadores")
            return
        
        schedulers = list(self.data['single'].keys())
        
        fig, axes = plt.subplots(2, 2, figsize=(16, 12))
        fig.suptitle('Comparação de Escalonadores: Qual algoritmo ganha desempenho com paralelismo?',
                    fontsize=16, fontweight='bold', y=0.995)
        
        # 1. Speedup por Escalonador
        ax1 = axes[0, 0]
        speedups = []
        for s in schedulers:
            single_time = self.data['single'][s]['exec_time']
            multi_time = self.data['multi'][s]['exec_time']
            speedup = single_time / multi_time if multi_time > 0 else 0
            speedups.append(speedup)
        
        colors = [COLORS.get(s, '#666666') for s in schedulers]
        bars = ax1.bar(schedulers, speedups, color=colors, alpha=0.9)
        ax1.axhline(y=1, color='red', linestyle='--', label='Baseline (1x)')
        ax1.set_ylabel('Speedup (Single/Multi)', fontweight='bold', fontsize=11)
        ax1.set_title('Speedup: Quanto cada escalonador se beneficia de paralelismo (>1 = melhor)',
                     fontweight='bold', fontsize=12)
        ax1.set_xticklabels(schedulers, rotation=15)
        ax1.legend()
        ax1.grid(axis='y', alpha=0.3)
        
        # Adicionar valores
        for bar, speedup in zip(bars, speedups):
            height = bar.get_height()
            ax1.text(bar.get_x() + bar.get_width()/2., height,
                    f'{speedup:.2f}x', ha='center', va='bottom', 
                    fontsize=10, fontweight='bold')
        
        # 2. Eficiência Relativa
        ax2 = axes[0, 1]
        single_eff = [self.data['single'][s]['efficiency'] for s in schedulers]
        multi_eff = [self.data['multi'][s]['efficiency'] for s in schedulers]
        
        x = np.arange(len(schedulers))
        width = 0.35
        ax2.bar(x - width/2, single_eff, width, label='Single-core', 
               color=COLORS['single'], alpha=0.8)
        ax2.bar(x + width/2, multi_eff, width, label='Multi-core', 
               color=COLORS['multi'], alpha=0.8)
        
        ax2.set_ylabel('Eficiência (Throughput/Ctx Switch)', fontweight='bold', fontsize=11)
        ax2.set_title('Eficiência Global: Relação entre processamento e overhead',
                     fontweight='bold', fontsize=12)
        ax2.set_xticks(x)
        ax2.set_xticklabels(schedulers, rotation=15)
        ax2.legend()
        ax2.grid(axis='y', alpha=0.3)
        
        # 3. Cache Hit Rate Comparativa
        ax3 = axes[1, 0]
        single_cache = [self.data['single'][s]['cache_hit_rate'] for s in schedulers]
        multi_cache = [self.data['multi'][s]['cache_hit_rate'] for s in schedulers]
        
        # Usar gráfico de barras se valores muito próximos
        max_diff = max(max(single_cache) - min(single_cache), 
                      max(multi_cache) - min(multi_cache))
        
        if max_diff < 1.0:  # Se diferença < 1%, usar barras com zoom
            ax3.bar(x - width/2, single_cache, width, label='Single-core', 
                   color=COLORS['single'], alpha=0.8)
            ax3.bar(x + width/2, multi_cache, width, label='Multi-core', 
                   color=COLORS['multi'], alpha=0.8)
            
            # Zoom na região de interesse
            min_val = min(min(single_cache), min(multi_cache))
            max_val = max(max(single_cache), max(multi_cache))
            margin = (max_val - min_val) * 0.5 if max_val > min_val else 1.0
            ax3.set_ylim(max(0, min_val - margin), min(100, max_val + margin))
            
            # Adicionar valores exatos nas barras
            for i, (s, m) in enumerate(zip(single_cache, multi_cache)):
                ax3.text(i - width/2, s, f'{s:.2f}%', ha='center', va='bottom', 
                        fontsize=8, fontweight='bold')
                ax3.text(i + width/2, m, f'{m:.2f}%', ha='center', va='bottom', 
                        fontsize=8, fontweight='bold')
        else:
            ax3.plot(schedulers, single_cache, marker='o', linewidth=2, markersize=8,
                    label='Single-core', color=COLORS['single'])
            ax3.plot(schedulers, multi_cache, marker='s', linewidth=2, markersize=8,
                    label='Multi-core', color=COLORS['multi'])
            ax3.set_ylim(0, 100)
        
        ax3.set_ylabel('Taxa de Cache Hit (%)', fontweight='bold', fontsize=11)
        ax3.set_title('Cache Performance: Comparação entre modos\n(Valores próximos indicam comportamento similar)',
                     fontweight='bold', fontsize=12)
        ax3.set_xticks(x)
        ax3.set_xticklabels(schedulers, rotation=15)
        ax3.legend()
        ax3.grid(alpha=0.3)
        
        # 4. Utilização de CPU
        ax4 = axes[1, 1]
        single_cpu = [self.data['single'][s]['cpu_util'] for s in schedulers]
        multi_cpu = [self.data['multi'][s]['cpu_util'] for s in schedulers]
        
        # Verificar se valores são muito próximos
        max_diff_cpu = max(max(single_cpu) - min(single_cpu), 
                          max(multi_cpu) - min(multi_cpu))
        
        if max_diff_cpu < 0.5:  # Se diferença < 0.5%, mostrar com zoom
            bars1 = ax4.bar(x - width/2, single_cpu, width, label='Single-core', 
                   color=COLORS['single'], alpha=0.8)
            bars2 = ax4.bar(x + width/2, multi_cpu, width, label='Multi-core', 
                   color=COLORS['multi'], alpha=0.8)
            
            # Zoom
            min_val = min(min(single_cpu), min(multi_cpu))
            max_val = max(max(single_cpu), max(multi_cpu))
            margin = max(0.5, (max_val - min_val) * 0.5)
            ax4.set_ylim(max(0, min_val - margin), min(100, max_val + margin))
            
            # Valores nas barras
            for i, (s, m) in enumerate(zip(single_cpu, multi_cpu)):
                ax4.text(i - width/2, s, f'{s:.2f}%', ha='center', va='bottom', 
                        fontsize=8, fontweight='bold')
                ax4.text(i + width/2, m, f'{m:.2f}%', ha='center', va='bottom', 
                        fontsize=8, fontweight='bold')
            
            ax4.set_title('CPU Utilization: Valores similares (zoom aplicado)\n(Diferença < 0.5%)',
                         fontweight='bold', fontsize=12)
        else:
            ax4.bar(x - width/2, single_cpu, width, label='Single-core', 
                   color=COLORS['single'], alpha=0.8)
            ax4.bar(x + width/2, multi_cpu, width, label='Multi-core', 
                   color=COLORS['multi'], alpha=0.8)
            ax4.set_ylim(0, 100)
            ax4.set_title('CPU Utilization: Aproveitamento dos recursos computacionais',
                         fontweight='bold', fontsize=12)
        
        ax4.set_ylabel('Utilização de CPU (%)', fontweight='bold', fontsize=11)
        ax4.set_xticks(x)
        ax4.set_xticklabels(schedulers, rotation=15)
        ax4.legend()
        ax4.grid(axis='y', alpha=0.3)
        
        plt.tight_layout()
        plt.savefig(f'{self.output_dir}/02_scheduler_comparison.png', dpi=300, bbox_inches='tight')
        plt.close()
        print("  ✓ 02_scheduler_comparison.png")
    
    def plot_scheduling_metrics(self):
        """3. Análise: Métricas de Escalonamento Detalhadas"""
        if 'single' not in self.data or 'multi' not in self.data:
            print("  ⚠ Dados insuficientes para métricas de escalonamento")
            return
        
        schedulers = list(self.data['single'].keys())
        
        fig, axes = plt.subplots(2, 3, figsize=(18, 12))
        fig.suptitle('Métricas de Escalonamento: Análise Comparativa de Desempenho',
                    fontsize=16, fontweight='bold', y=0.995)
        
        x = np.arange(len(schedulers))
        width = 0.35
        
        # 1. Tempo Médio de Espera
        ax1 = axes[0, 0]
        single_wait = [self.data['single'][s]['wait_time'] for s in schedulers]
        multi_wait = [self.data['multi'][s]['wait_time'] for s in schedulers]
        
        ax1.bar(x - width/2, single_wait, width, label='Single-core', 
               color=COLORS['single'], alpha=0.8)
        ax1.bar(x + width/2, multi_wait, width, label='Multi-core', 
               color=COLORS['multi'], alpha=0.8)
        
        ax1.set_ylabel('Tempo Médio (ms)', fontweight='bold', fontsize=10)
        ax1.set_title('Tempo Médio de Espera\n(Menor = Melhor)', 
                     fontweight='bold', fontsize=11)
        ax1.set_xticks(x)
        ax1.set_xticklabels(schedulers, rotation=15, fontsize=9)
        ax1.legend()
        ax1.grid(axis='y', alpha=0.3)
        
        # 2. Tempo Médio de Retorno (Turnaround)
        ax2 = axes[0, 1]
        single_turn = [self.data['single'][s]['turnaround_time'] for s in schedulers]
        multi_turn = [self.data['multi'][s]['turnaround_time'] for s in schedulers]
        
        ax2.bar(x - width/2, single_turn, width, label='Single-core', 
               color=COLORS['single'], alpha=0.8)
        ax2.bar(x + width/2, multi_turn, width, label='Multi-core', 
               color=COLORS['multi'], alpha=0.8)
        
        ax2.set_ylabel('Tempo Médio (ms)', fontweight='bold', fontsize=10)
        ax2.set_title('Tempo Médio de Retorno (Turnaround)\n(Menor = Melhor)', 
                     fontweight='bold', fontsize=11)
        ax2.set_xticks(x)
        ax2.set_xticklabels(schedulers, rotation=15, fontsize=9)
        ax2.legend()
        ax2.grid(axis='y', alpha=0.3)
        
        # 3. Tempo Médio de Resposta
        ax3 = axes[0, 2]
        single_resp = [self.data['single'][s]['response_time'] for s in schedulers]
        multi_resp = [self.data['multi'][s]['response_time'] for s in schedulers]
        
        ax3.bar(x - width/2, single_resp, width, label='Single-core', 
               color=COLORS['single'], alpha=0.8)
        ax3.bar(x + width/2, multi_resp, width, label='Multi-core', 
               color=COLORS['multi'], alpha=0.8)
        
        ax3.set_ylabel('Tempo Médio (ms)', fontweight='bold', fontsize=10)
        ax3.set_title('Tempo Médio de Resposta\n(Menor = Melhor)', 
                     fontweight='bold', fontsize=11)
        ax3.set_xticks(x)
        ax3.set_xticklabels(schedulers, rotation=15, fontsize=9)
        ax3.legend()
        ax3.grid(axis='y', alpha=0.3)
        
        # 4. Utilização de CPU
        ax4 = axes[1, 0]
        single_cpu = [self.data['single'][s]['cpu_util'] for s in schedulers]
        multi_cpu = [self.data['multi'][s]['cpu_util'] for s in schedulers]
        
        ax4.bar(x - width/2, single_cpu, width, label='Single-core', 
               color=COLORS['single'], alpha=0.8)
        ax4.bar(x + width/2, multi_cpu, width, label='Multi-core', 
               color=COLORS['multi'], alpha=0.8)
        
        ax4.set_ylabel('Utilização (%)', fontweight='bold', fontsize=10)
        ax4.set_title('Utilização Média da CPU\n(Maior = Melhor)', 
                     fontweight='bold', fontsize=11)
        ax4.set_xticks(x)
        ax4.set_xticklabels(schedulers, rotation=15, fontsize=9)
        ax4.set_ylim(0, 100)
        ax4.legend()
        ax4.grid(axis='y', alpha=0.3)
        
        # 5. Throughput
        ax5 = axes[1, 1]
        single_thr = [self.data['single'][s]['throughput'] for s in schedulers]
        multi_thr = [self.data['multi'][s]['throughput'] for s in schedulers]
        
        ax5.bar(x - width/2, single_thr, width, label='Single-core', 
               color=COLORS['single'], alpha=0.8)
        ax5.bar(x + width/2, multi_thr, width, label='Multi-core', 
               color=COLORS['multi'], alpha=0.8)
        
        ax5.set_ylabel('Processos/segundo', fontweight='bold', fontsize=10)
        ax5.set_title('Taxa de Throughput Global\n(Maior = Melhor)', 
                     fontweight='bold', fontsize=11)
        ax5.set_xticks(x)
        ax5.set_xticklabels(schedulers, rotation=15, fontsize=9)
        ax5.legend()
        ax5.grid(axis='y', alpha=0.3)
        
        # 6. Eficiência
        ax6 = axes[1, 2]
        single_eff = [self.data['single'][s]['efficiency'] for s in schedulers]
        multi_eff = [self.data['multi'][s]['efficiency'] for s in schedulers]
        
        ax6.bar(x - width/2, single_eff, width, label='Single-core', 
               color=COLORS['single'], alpha=0.8)
        ax6.bar(x + width/2, multi_eff, width, label='Multi-core', 
               color=COLORS['multi'], alpha=0.8)
        
        ax6.set_ylabel('Eficiência', fontweight='bold', fontsize=10)
        ax6.set_title('Eficiência Global\n(Throughput/CtxSwitch - Maior = Melhor)', 
                     fontweight='bold', fontsize=11)
        ax6.set_xticks(x)
        ax6.set_xticklabels(schedulers, rotation=15, fontsize=9)
        ax6.legend()
        ax6.grid(axis='y', alpha=0.3)
        
        plt.tight_layout()
        plt.savefig(f'{self.output_dir}/03_scheduling_metrics.png', dpi=300, bbox_inches='tight')
        plt.close()
        print("  ✓ 03_scheduling_metrics.png")
    
    def plot_performance_radar(self):
        """4. Gráfico Radar: Comparação Individual de Escalonadores"""
        if 'single' not in self.data or 'multi' not in self.data:
            print("  ⚠ Dados insuficientes para gráfico radar")
            return
        
        schedulers = list(self.data['single'].keys())
        n_schedulers = len(schedulers)
        
        # Criar grid 2x2 para 4 escalonadores
        fig, axes = plt.subplots(2, 2, figsize=(16, 14), 
                                subplot_kw=dict(projection='polar'))
        axes = axes.flatten()
        
        fig.suptitle('Análise Radar: Comparação Single-Core vs Multi-Core por Escalonador\n' + 
                    '(Área externa = melhor desempenho)',
                    fontsize=16, fontweight='bold', y=0.98)
        
        categories = ['Throughput', 'Eficiência', 'CPU Util.', 
                     'Cache Hit', 'Rapidez Resp.', 'Rapidez Esp.']
        N = len(categories)
        angles = np.linspace(0, 2 * np.pi, N, endpoint=False).tolist()
        angles += angles[:1]
        
        for idx, scheduler in enumerate(schedulers):
            ax = axes[idx]
            
            # Single-core
            single = self.data['single'][scheduler]
            single_values = [
                min(single['throughput'] * 10, 100),
                min(single['efficiency'] * 10, 100),
                single['cpu_util'],
                single['cache_hit_rate'],
                100 - min(single['response_time'] / 10, 100),
                100 - min(single['wait_time'] / 10, 100)
            ]
            single_values += single_values[:1]
            
            # Multi-core
            multi = self.data['multi'][scheduler]
            multi_values = [
                min(multi['throughput'] * 10, 100),
                min(multi['efficiency'] * 10, 100),
                multi['cpu_util'],
                multi['cache_hit_rate'],
                100 - min(multi['response_time'] / 10, 100),
                100 - min(multi['wait_time'] / 10, 100)
            ]
            multi_values += multi_values[:1]
            
            # Plotar
            ax.plot(angles, single_values, 'o-', linewidth=2.5, 
                   label='Single-core', color=COLORS['single'], markersize=6)
            ax.fill(angles, single_values, alpha=0.15, color=COLORS['single'])
            
            ax.plot(angles, multi_values, 's-', linewidth=2.5, 
                   label='Multi-core', color=COLORS['multi'], markersize=6)
            ax.fill(angles, multi_values, alpha=0.15, color=COLORS['multi'])
            
            # Configurações
            ax.set_xticks(angles[:-1])
            ax.set_xticklabels(categories, size=9, fontweight='bold')
            ax.set_ylim(0, 100)
            ax.set_yticks([25, 50, 75, 100])
            ax.set_yticklabels(['25', '50', '75', '100'], size=8)
            
            # Título com cor do escalonador
            title_color = COLORS.get(scheduler, '#666666')
            ax.set_title(f'{scheduler}', fontsize=13, fontweight='bold', 
                        pad=20, color=title_color)
            
            # Legenda apenas no primeiro gráfico
            if idx == 0:
                ax.legend(loc='upper right', bbox_to_anchor=(1.35, 1.15), 
                         fontsize=10, framealpha=0.9)
            
            ax.grid(True, linewidth=0.5, alpha=0.7)
        
        plt.tight_layout()
        plt.savefig(f'{self.output_dir}/04_performance_radar.png', dpi=300, bbox_inches='tight')
        plt.close()
        print("  ✓ 04_performance_radar.png")
    
    def plot_time_anomalies(self):
        """5. Análise de Anomalias de Tempo (Debug FCFS)"""
        if 'single' not in self.data or 'multi' not in self.data:
            print("  ⚠ Dados insuficientes para análise de anomalias")
            return
        
        schedulers = list(self.data['single'].keys())
        
        fig, axes = plt.subplots(2, 2, figsize=(16, 12))
        fig.suptitle('Análise de Anomalias: Investigação de comportamento inesperado\n' +
                    '(FCFS multi-core apresenta tempo de espera anormal?)',
                    fontsize=16, fontweight='bold', y=0.99)
        
        x = np.arange(len(schedulers))
        width = 0.35
        
        # 1. Wait Time (escala linear)
        ax1 = axes[0, 0]
        single_wait = [self.data['single'][s]['wait_time'] for s in schedulers]
        multi_wait = [self.data['multi'][s]['wait_time'] for s in schedulers]
        
        bars1 = ax1.bar(x - width/2, single_wait, width, label='Single-core', 
                       color=COLORS['single'], alpha=0.8)
        bars2 = ax1.bar(x + width/2, multi_wait, width, label='Multi-core', 
                       color=COLORS['multi'], alpha=0.8)
        
        # Destacar valores muito diferentes
        for i, (s, m) in enumerate(zip(single_wait, multi_wait)):
            ratio = m / s if s > 0 else 0
            if ratio > 10 or (ratio < 0.1 and ratio > 0):  # Diferença > 10x
                ax1.text(i, max(s, m), f'⚠ {ratio:.1f}x', 
                        ha='center', va='bottom', fontsize=9, 
                        fontweight='bold', color='red')
            # Adicionar valores
            ax1.text(i - width/2, s, f'{s:.2f}', ha='center', va='bottom', fontsize=8)
            ax1.text(i + width/2, m, f'{m:.2f}', ha='center', va='bottom', fontsize=8)
        
        ax1.set_ylabel('Tempo Médio de Espera (ms)', fontweight='bold', fontsize=11)
        ax1.set_title('Wait Time: Escala Linear (valores com ⚠ = anomalia > 10x)',
                     fontweight='bold', fontsize=12)
        ax1.set_xticks(x)
        ax1.set_xticklabels(schedulers, rotation=15)
        ax1.legend()
        ax1.grid(axis='y', alpha=0.3)
        
        # 2. Wait Time (escala logarítmica)
        ax2 = axes[0, 1]
        ax2.bar(x - width/2, single_wait, width, label='Single-core', 
               color=COLORS['single'], alpha=0.8)
        ax2.bar(x + width/2, multi_wait, width, label='Multi-core', 
               color=COLORS['multi'], alpha=0.8)
        
        ax2.set_ylabel('Tempo Médio de Espera (ms)', fontweight='bold', fontsize=11)
        ax2.set_title('Wait Time: Escala Logarítmica (para visualizar diferenças extremas)',
                     fontweight='bold', fontsize=12)
        ax2.set_xticks(x)
        ax2.set_xticklabels(schedulers, rotation=15)
        ax2.set_yscale('log')
        ax2.legend()
        ax2.grid(axis='y', alpha=0.3)
        
        # 3. Ratio Wait Time (Multi/Single)
        ax3 = axes[1, 0]
        ratios = [multi_wait[i] / single_wait[i] if single_wait[i] > 0 else 0 
                 for i in range(len(schedulers))]
        
        colors_ratio = [COLORS.get(s, '#666666') for s in schedulers]
        bars = ax3.bar(schedulers, ratios, color=colors_ratio, alpha=0.9)
        ax3.axhline(y=1, color='red', linestyle='--', linewidth=2, label='Paridade (1.0x)')
        
        # Destacar anomalias
        for i, (s, ratio) in enumerate(zip(schedulers, ratios)):
            if ratio > 10 or ratio < 0.1:
                bars[i].set_edgecolor('red')
                bars[i].set_linewidth(3)
            ax3.text(i, ratio, f'{ratio:.1f}x', ha='center', va='bottom',
                    fontsize=10, fontweight='bold')
        
        ax3.set_ylabel('Ratio (Multi/Single)', fontweight='bold', fontsize=11)
        ax3.set_title('Ratio de Wait Time: Multi/Single (>1 = pior em multi-core, <1 = melhor)',
                     fontweight='bold', fontsize=12)
        ax3.set_xticklabels(schedulers, rotation=15)
        ax3.legend()
        ax3.grid(axis='y', alpha=0.3)
        
        # 4. Tabela de Diagnóstico
        ax4 = axes[1, 1]
        ax4.axis('off')
        
        table_data = [['Scheduler', 'Single', 'Multi', 'Ratio', 'Status']]
        for i, s in enumerate(schedulers):
            ratio = ratios[i]
            status = '✅ Normal' if 0.5 <= ratio <= 2.0 else '⚠️ ANOMALIA'
            table_data.append([
                s,
                f'{single_wait[i]:.2f}ms',
                f'{multi_wait[i]:.2f}ms',
                f'{ratio:.1f}x',
                status
            ])
        
        table = ax4.table(cellText=table_data, cellLoc='center', loc='center',
                         colWidths=[0.25, 0.18, 0.18, 0.15, 0.24])
        table.auto_set_font_size(False)
        table.set_fontsize(10)
        table.scale(1, 2.5)
        
        # Colorir header
        for i in range(5):
            table[(0, i)].set_facecolor('#4472C4')
            table[(0, i)].set_text_props(weight='bold', color='white')
        
        # Colorir linhas com anomalias
        for i, ratio in enumerate(ratios, start=1):
            if ratio > 2.0 or ratio < 0.5:
                for j in range(5):
                    table[(i, j)].set_facecolor('#FFE699')
        
        ax4.set_title('Diagnóstico de Anomalias de Tempo', 
                     fontweight='bold', fontsize=12, pad=20)
        
        plt.tight_layout()
        plt.savefig(f'{self.output_dir}/05_time_anomalies.png', dpi=300, bbox_inches='tight')
        plt.close()
        print("  ✓ 05_time_anomalies.png")
    
    def generate_summary_report(self):
        """5. Relatório Resumo com Conclusões"""
        if 'single' not in self.data or 'multi' not in self.data:
            return
        
        schedulers = list(self.data['single'].keys())
        
        report = []
        report.append("=" * 80)
        report.append("RELATÓRIO DE ANÁLISE DE DESEMPENHO - Simulador Von Neumann")
        report.append("=" * 80)
        report.append("")
        
        # Análise de Speedup
        report.append("1. ANÁLISE DE SPEEDUP (Multi-threading vs Single-core)")
        report.append("-" * 80)
        for s in schedulers:
            single_time = self.data['single'][s]['exec_time']
            multi_time = self.data['multi'][s]['exec_time']
            speedup = single_time / multi_time if multi_time > 0 else 0
            
            report.append(f"  {s:12s}: {speedup:5.2f}x speedup "
                         f"({single_time:7.2f}ms → {multi_time:7.2f}ms)")
        
        # Melhor escalonador por métrica
        report.append("")
        report.append("2. MELHOR ESCALONADOR POR MÉTRICA (Multi-core)")
        report.append("-" * 80)
        
        metrics = [
            ('Menor Tempo de Espera', 'wait_time', 'ms', 'min'),
            ('Menor Tempo de Resposta', 'response_time', 'ms', 'min'),
            ('Maior Throughput', 'throughput', 'proc/s', 'max'),
            ('Maior Eficiência', 'efficiency', '', 'max'),
            ('Maior Cache Hit Rate', 'cache_hit_rate', '%', 'max'),
            ('Maior CPU Utilization', 'cpu_util', '%', 'max')
        ]
        
        for name, key, unit, mode in metrics:
            if mode == 'min':
                best = min(schedulers, key=lambda s: self.data['multi'][s][key])
            else:
                best = max(schedulers, key=lambda s: self.data['multi'][s][key])
            
            value = self.data['multi'][best][key]
            report.append(f"  {name:30s}: {best:12s} ({value:7.2f} {unit})")
        
        # Salvar relatório
        report.append("")
        report.append("=" * 80)
        
        report_file = os.path.join(self.output_dir, 'performance_summary.txt')
        with open(report_file, 'w') as f:
            f.write('\n'.join(report))
        
        print(f"  ✓ performance_summary.txt")
        
        # Imprimir resumo no terminal
        print("\n" + "\n".join(report) + "\n")


def main():
    """Função principal"""
    print("=" * 80)
    print("  ANÁLISE UNIFICADA DE DESEMPENHO - Simulador Von Neumann")
    print("=" * 80)
    
    results_dir = 'output'
    if len(sys.argv) > 1:
        results_dir = sys.argv[1]
    
    output_dir = 'plots'
    if len(sys.argv) > 2:
        output_dir = sys.argv[2]
    
    analyzer = PerformanceAnalyzer(output_dir)
    
    if not analyzer.load_all_results(results_dir):
        print("\n❌ Nenhum resultado encontrado!")
        print("   Execute o simulador primeiro para gerar os arquivos CSV.")
        print("\n   Arquivos esperados:")
        print("     - output/metrics_single.csv")
        print("     - output/metrics_multi.csv")
        return 1
    
    print("\n📊 Gerando análises gráficas...\n")
    
    analyzer.plot_threading_impact()
    analyzer.plot_scheduler_comparison()
    analyzer.plot_scheduling_metrics()
    analyzer.plot_performance_radar()
    analyzer.plot_time_anomalies()
    analyzer.generate_summary_report()
    
    print(f"\n✅ Análise concluída! Gráficos salvos em: {output_dir}/\n")
    print("=" * 80)
    
    return 0


if __name__ == '__main__':
    sys.exit(main())

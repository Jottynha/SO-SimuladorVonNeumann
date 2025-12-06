# Análises Adicionais:
# 1. Performance Single vs Multi-core por escalonador (onde cada um perde desempenho)
# 2. Análise de políticas de cache (LRU vs FIFO) - quando implementado
# 3. Degradação de performance com escalonamento
# 4. Identificação de pontos de break-even


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
plt.rcParams['font.size'] = 10

# Paleta de cores por escalonador
COLORS = {
    'FCFS': '#2E86AB',
    'SJN': '#A23B72',
    'Priority': '#F18F01',
    'RoundRobin': '#C73E1D'
}

class ExtendedAnalyzer:
    """Análise estendida com foco em degradação e políticas"""
    
    def __init__(self, output_dir='plots'):
        self.output_dir = output_dir
        self.data = {}
        os.makedirs(output_dir, exist_ok=True)
    
    def load_csv(self, filepath: str) -> Dict:
        """Carrega dados de CSV"""
        results = {}
        
        if not os.path.exists(filepath):
            return results
        
        with open(filepath, 'r') as f:
            lines = [line for line in f if not line.strip().startswith('#')]
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
    
    def load_data(self, results_dir='output'):
        """Carrega todos os resultados"""
        single_file = os.path.join(results_dir, 'metrics_single.csv')
        if os.path.exists(single_file):
            self.data['single'] = self.load_csv(single_file)
        
        multi_file = os.path.join(results_dir, 'metrics_multi.csv')
        if os.path.exists(multi_file):
            self.data['multi'] = self.load_csv(multi_file)
        
        return len(self.data) > 0
    
    def plot_scheduler_degradation(self):
        """
        Análise 1: Onde cada escalonador perde desempenho
        Compara single-core vs multi-core para identificar ganhos/perdas
        """
        if 'single' not in self.data or 'multi' not in self.data:
            return
        
        schedulers = list(self.data['single'].keys())
        cores_multi = self.data['multi'][schedulers[0]]['cores']
        
        fig, axes = plt.subplots(2, 2, figsize=(16, 12))
        fig.suptitle(f'Análise de Degradação: Onde cada escalonador perde/ganha com {cores_multi} cores',
                    fontsize=16, fontweight='bold', y=0.995)
        
        # 1. Speedup Individual por Escalonador
        ax1 = axes[0, 0]
        speedups = []
        colors_list = []
        
        for s in schedulers:
            single_time = self.data['single'][s]['exec_time']
            multi_time = self.data['multi'][s]['exec_time']
            speedup = single_time / multi_time if multi_time > 0 else 0
            speedups.append(speedup)
            colors_list.append(COLORS.get(s, '#666666'))
        
        bars = ax1.barh(schedulers, speedups, color=colors_list, alpha=0.9)
        ax1.axvline(x=1, color='red', linestyle='--', linewidth=2, label='Break-even (1x)')
        ax1.set_xlabel('Speedup (Single-core / Multi-core)', fontweight='bold', fontsize=11)
        ax1.set_title(f'Speedup com {cores_multi} cores: >1 = Ganho, <1 = Perda de desempenho',
                     fontweight='bold', fontsize=12)
        ax1.legend()
        ax1.grid(axis='x', alpha=0.3)
        
        # Adicionar valores nas barras
        for i, (bar, speedup) in enumerate(zip(bars, speedups)):
            width = bar.get_width()
            label_color = 'green' if speedup > 1 else 'red' if speedup < 1 else 'gray'
            improvement = ((speedup - 1) * 100)
            ax1.text(width, bar.get_y() + bar.get_height()/2, 
                    f'  {speedup:.2f}x ({improvement:+.1f}%)',
                    ha='left', va='center', fontsize=10, fontweight='bold', color=label_color)
        
        # 2. Cache Hit Rate: Single vs Multi
        ax2 = axes[0, 1]
        single_cache = [self.data['single'][s]['cache_hit_rate'] for s in schedulers]
        multi_cache = [self.data['multi'][s]['cache_hit_rate'] for s in schedulers]
        cache_degradation = [s - m for s, m in zip(single_cache, multi_cache)]
        
        bars = ax2.barh(schedulers, cache_degradation, 
                       color=['red' if d > 0 else 'green' for d in cache_degradation], 
                       alpha=0.8)
        ax2.axvline(x=0, color='black', linestyle='-', linewidth=1)
        ax2.set_xlabel('Degradação de Cache Hit Rate (%)', fontweight='bold', fontsize=11)
        ax2.set_title('Poluição de Cache: Vermelho = Mais cache misses no multi-core',
                     fontweight='bold', fontsize=12)
        ax2.grid(axis='x', alpha=0.3)
        
        # Adicionar valores
        for bar, deg, s_cache, m_cache in zip(bars, cache_degradation, single_cache, multi_cache):
            width = bar.get_width()
            ax2.text(width, bar.get_y() + bar.get_height()/2,
                    f'  {deg:+.1f}% ({s_cache:.1f}%→{m_cache:.1f}%)',
                    ha='left' if width > 0 else 'right', 
                    va='center', fontsize=9)
        
        # 3. Throughput Comparison
        ax3 = axes[1, 0]
        single_thr = np.array([self.data['single'][s]['throughput'] for s in schedulers])
        multi_thr = np.array([self.data['multi'][s]['throughput'] for s in schedulers])
        thr_gain = ((multi_thr - single_thr) / single_thr) * 100
        
        bars = ax3.barh(schedulers, thr_gain,
                       color=['green' if g > 0 else 'red' for g in thr_gain],
                       alpha=0.8)
        ax3.axvline(x=0, color='black', linestyle='-', linewidth=1)
        ax3.set_xlabel('Ganho de Throughput (%)', fontweight='bold', fontsize=11)
        ax3.set_title('Ganho de Processamento: Verde = Mais processos/segundo no multi-core',
                     fontweight='bold', fontsize=12)
        ax3.grid(axis='x', alpha=0.3)
        
        # Adicionar valores
        for bar, gain, s_val, m_val in zip(bars, thr_gain, single_thr, multi_thr):
            width = bar.get_width()
            ax3.text(width, bar.get_y() + bar.get_height()/2,
                    f'  {gain:+.1f}% ({s_val:.0f}→{m_val:.0f})',
                    ha='left' if width > 0 else 'right',
                    va='center', fontsize=9)
        
        # 4. Resumo: Quem ganha e quem perde
        ax4 = axes[1, 1]
        ax4.axis('off')
        
        # Criar tabela de resumo
        summary_text = f"RESUMO: Analise Single-core vs Multi-core ({cores_multi} cores)\n"
        summary_text += "=" * 70 + "\n\n"
        
        # Ordenar por speedup
        scheduler_speedups = list(zip(schedulers, speedups))
        scheduler_speedups.sort(key=lambda x: x[1], reverse=True)
        
        summary_text += "RANKING DE SPEEDUP:\n"
        for i, (sched, speedup) in enumerate(scheduler_speedups, 1):
            improvement = ((speedup - 1) * 100)
            status = "GANHO" if speedup > 1 else "PERDA" if speedup < 1 else "NEUTRO"
            summary_text += f"{i}. {sched:12s}: {speedup:5.2f}x ({improvement:+6.1f}%) {status}\n"
        
        summary_text += "\n" + "-" * 70 + "\n"
        summary_text += "ANALISE:\n"
        
        # Melhor escalonador
        best_sched, best_speedup = scheduler_speedups[0]
        summary_text += f"• Melhor para multi-core: {best_sched} ({best_speedup:.2f}x)\n"
        
        # Pior escalonador
        worst_sched, worst_speedup = scheduler_speedups[-1]
        if worst_speedup < 1:
            summary_text += f"• Perde desempenho: {worst_sched} ({worst_speedup:.2f}x)\n"
        
        # Cache mais afetado
        cache_impact = list(zip(schedulers, cache_degradation))
        cache_impact.sort(key=lambda x: x[1], reverse=True)
        worst_cache_sched, worst_cache_deg = cache_impact[0]
        if worst_cache_deg > 0:
            summary_text += f"• Mais poluição de cache: {worst_cache_sched} ({worst_cache_deg:+.1f}%)\n"
        
        # Melhor throughput
        thr_impact = list(zip(schedulers, thr_gain))
        thr_impact.sort(key=lambda x: x[1], reverse=True)
        best_thr_sched, best_thr_gain = thr_impact[0]
        summary_text += f"• Maior ganho de throughput: {best_thr_sched} ({best_thr_gain:+.1f}%)\n"
        
        summary_text += "\n" + "-" * 70 + "\n"
        summary_text += "CONCLUSOES:\n"
        
        if best_speedup > 1:
            summary_text += f"• Multi-threading ({cores_multi} cores) traz ganhos reais de desempenho\n"
            summary_text += f"• Speedup médio: {np.mean(speedups):.2f}x\n"
        else:
            summary_text += f"• Multi-threading não mostra ganhos consistentes\n"
            summary_text += f"• Overhead de sincronização pode superar benefícios\n"
        
        avg_cache_deg = np.mean(cache_degradation)
        if avg_cache_deg > 2:
            summary_text += f"• Cache pollution significativa ({avg_cache_deg:.1f}% em média)\n"
        
        ax4.text(0.05, 0.95, summary_text, 
                transform=ax4.transAxes,
                fontsize=9,
                verticalalignment='top',
                fontfamily='monospace',
                bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.3))
        
        plt.tight_layout()
        plt.savefig(f'{self.output_dir}/extended_01_scheduler_degradation.png', 
                   dpi=300, bbox_inches='tight')
        plt.close()
    
    def plot_performance_breakdown(self):
        """
        Análise 2: Breakdown detalhado de métricas por escalonador
        Mostra todos os aspectos: tempo, throughput, cache, eficiência
        """
        if 'single' not in self.data or 'multi' not in self.data:
            return
        
        schedulers = list(self.data['single'].keys())
        
        fig, axes = plt.subplots(2, 2, figsize=(16, 12))
        fig.suptitle('Breakdown Completo de Performance: Todas as Métricas por Escalonador',
                    fontsize=16, fontweight='bold', y=0.995)
        
        # 1. Tempo de Execução (ms)
        ax1 = axes[0, 0]
        x = np.arange(len(schedulers))
        width = 0.35
        
        single_time = [self.data['single'][s]['exec_time'] for s in schedulers]
        multi_time = [self.data['multi'][s]['exec_time'] for s in schedulers]
        
        bars1 = ax1.bar(x - width/2, single_time, width, label='Single-core', alpha=0.8, color='#2E86AB')
        bars2 = ax1.bar(x + width/2, multi_time, width, label='Multi-core', alpha=0.8, color='#F18F01')
        
        ax1.set_ylabel('Tempo (ms)', fontweight='bold')
        ax1.set_title('Tempo de Execução: Menor é melhor', fontweight='bold')
        ax1.set_xticks(x)
        ax1.set_xticklabels(schedulers, rotation=15)
        ax1.legend()
        ax1.grid(axis='y', alpha=0.3)
        
        # Adicionar diferença percentual
        for i, (s, m) in enumerate(zip(single_time, multi_time)):
            diff = ((m - s) / s) * 100
            color = 'green' if diff < 0 else 'red'
            ax1.text(i, max(s, m), f'{diff:+.0f}%', 
                    ha='center', va='bottom', fontsize=8, color=color, fontweight='bold')
        
        # 2. Cache Hit Rate (%)
        ax2 = axes[0, 1]
        single_cache = [self.data['single'][s]['cache_hit_rate'] for s in schedulers]
        multi_cache = [self.data['multi'][s]['cache_hit_rate'] for s in schedulers]
        
        bars1 = ax2.bar(x - width/2, single_cache, width, label='Single-core', alpha=0.8, color='#2E86AB')
        bars2 = ax2.bar(x + width/2, multi_cache, width, label='Multi-core', alpha=0.8, color='#F18F01')
        
        ax2.set_ylabel('Cache Hit Rate (%)', fontweight='bold')
        ax2.set_title('Taxa de Acerto de Cache: Maior é melhor', fontweight='bold')
        ax2.set_xticks(x)
        ax2.set_xticklabels(schedulers, rotation=15)
        ax2.set_ylim(0, max(single_cache + multi_cache) * 1.2)
        ax2.legend()
        ax2.grid(axis='y', alpha=0.3)
        
        # 3. Throughput (processos/s)
        ax3 = axes[1, 0]
        single_thr = [self.data['single'][s]['throughput'] for s in schedulers]
        multi_thr = [self.data['multi'][s]['throughput'] for s in schedulers]
        
        bars1 = ax3.bar(x - width/2, single_thr, width, label='Single-core', alpha=0.8, color='#2E86AB')
        bars2 = ax3.bar(x + width/2, multi_thr, width, label='Multi-core', alpha=0.8, color='#F18F01')
        
        ax3.set_ylabel('Throughput (proc/s)', fontweight='bold')
        ax3.set_title('Throughput: Maior é melhor', fontweight='bold')
        ax3.set_xticks(x)
        ax3.set_xticklabels(schedulers, rotation=15)
        ax3.legend()
        ax3.grid(axis='y', alpha=0.3)
        
        # 4. Tempos Médios (Wait, Turnaround, Response)
        ax4 = axes[1, 1]
        
        metrics = ['wait_time', 'turnaround_time', 'response_time']
        metric_names = ['Espera', 'Retorno', 'Resposta']
        colors_metric = ['#2E86AB', '#A23B72', '#F18F01']
        
        x_pos = np.arange(len(schedulers))
        bar_width = 0.25
        
        for i, (metric, name, color) in enumerate(zip(metrics, metric_names, colors_metric)):
            multi_vals = [self.data['multi'][s][metric] for s in schedulers]
            ax4.bar(x_pos + i*bar_width, multi_vals, bar_width, label=name, 
                   alpha=0.8, color=color)
        
        ax4.set_ylabel('Tempo (ms)', fontweight='bold')
        ax4.set_title('Tempos Médios (Multi-core): Menor é melhor', fontweight='bold')
        ax4.set_xticks(x_pos + bar_width)
        ax4.set_xticklabels(schedulers, rotation=15)
        ax4.legend()
        ax4.grid(axis='y', alpha=0.3)
        
        plt.tight_layout()
        plt.savefig(f'{self.output_dir}/extended_02_performance_breakdown.png',
                   dpi=300, bbox_inches='tight')
        plt.close()
    
    def plot_cache_policy_comparison(self):
        """
        Análise 3: Comparação de políticas de cache (FIFO vs LRU)
        Detecta automaticamente se há dados de diferentes políticas
        """
        # Tentar detectar dados de diferentes políticas de cache
        parent_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
        output_base = os.path.join(parent_dir, 'output')
        
        # Procurar por diretórios com fifo/lru
        fifo_dirs = []
        lru_dirs = []
        
        if os.path.exists(output_base):
            for item in os.listdir(output_base):
                item_path = os.path.join(output_base, item)
                if os.path.isdir(item_path):
                    if 'fifo' in item.lower():
                        fifo_dirs.append(item_path)
                    elif 'lru' in item.lower():
                        lru_dirs.append(item_path)
        
        # Se não encontrou dados, mostrar placeholder
        if not fifo_dirs or not lru_dirs:
            fig, ax = plt.subplots(figsize=(12, 8))
            ax.text(0.5, 0.5, 
                   'COMPARAÇÃO DE POLÍTICAS DE CACHE\n\n' + 
                   'Nenhum dado encontrado para comparação FIFO vs LRU\n\n' +
                   'Execute o script de teste:\n' +
                   '  $ scripts/test_cache_policies.sh\n\n' +
                   'Ou execute manualmente:\n' +
                   '  $ ./simulador --replacement FIFO --output output/fifo_test\n' +
                   '  $ ./simulador --replacement LRU --output output/lru_test',
                   ha='center', va='center', fontsize=14,
                   bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.5))
            ax.axis('off')
            plt.tight_layout()
            plt.savefig(f'{self.output_dir}/extended_03_cache_policy_comparison.png',
                       dpi=300, bbox_inches='tight')
            plt.close()
            return
        
        # Carregar dados FIFO e LRU
        fifo_data = {}
        lru_data = {}
        
        # Priorizar dados de 1 core e 8 cores
        for fdir in fifo_dirs:
            for csv_name in ['metrics_single.csv', 'metrics_multi.csv']:
                csv_path = os.path.join(fdir, csv_name)
                if os.path.exists(csv_path):
                    data = self.load_csv(csv_path)
                    core_type = '1core' if 'single' in csv_name else '8cores'
                    if fdir not in fifo_data:
                        fifo_data[fdir] = {}
                    fifo_data[fdir][core_type] = data
        
        for ldir in lru_dirs:
            for csv_name in ['metrics_single.csv', 'metrics_multi.csv']:
                csv_path = os.path.join(ldir, csv_name)
                if os.path.exists(csv_path):
                    data = self.load_csv(csv_path)
                    core_type = '1core' if 'single' in csv_name else '8cores'
                    if ldir not in lru_data:
                        lru_data[ldir] = {}
                    lru_data[ldir][core_type] = data
        
        if not fifo_data or not lru_data:
            return
        
        # Criar gráfico comparativo
        fig = plt.figure(figsize=(18, 10))
        fig.suptitle('Comparação FIFO vs LRU: Políticas de Substituição de Cache',
                    fontsize=16, fontweight='bold', y=0.98)
        
        gs = fig.add_gridspec(2, 3, hspace=0.3, wspace=0.3)
        
        # Pegar primeiro diretório de cada
        fifo_dir = list(fifo_data.keys())[0]
        lru_dir = list(lru_data.keys())[0]
        
        fifo_name = os.path.basename(fifo_dir)
        lru_name = os.path.basename(lru_dir)
        
        # 1. Cache Hit Rate Comparison
        ax1 = fig.add_subplot(gs[0, 0])
        for core_type in ['1core', '8cores']:
            if core_type in fifo_data[fifo_dir] and core_type in lru_data[lru_dir]:
                schedulers = list(fifo_data[fifo_dir][core_type].keys())
                x = np.arange(len(schedulers))
                width = 0.35
                
                fifo_hits = [fifo_data[fifo_dir][core_type][s]['cache_hit_rate'] for s in schedulers]
                lru_hits = [lru_data[lru_dir][core_type][s]['cache_hit_rate'] for s in schedulers]
                
                offset = -width/2 if core_type == '1core' else width/2
                label_suffix = ' (1 core)' if core_type == '1core' else ' (8 cores)'
                
                ax1.bar(x + offset - width/2, fifo_hits, width/2, label=f'FIFO{label_suffix}', 
                       alpha=0.8, color='#FF6B6B')
                ax1.bar(x + offset, lru_hits, width/2, label=f'LRU{label_suffix}',
                       alpha=0.8, color='#4ECDC4')
        
        ax1.set_xlabel('Escalonador')
        ax1.set_ylabel('Cache Hit Rate (%)')
        ax1.set_title('Taxa de Acerto na Cache')
        ax1.set_xticks(x)
        ax1.set_xticklabels(schedulers, rotation=45, ha='right')
        ax1.legend()
        ax1.grid(True, alpha=0.3)
        
        # 2. Execution Time Comparison
        ax2 = fig.add_subplot(gs[0, 1])
        for core_type in ['1core', '8cores']:
            if core_type in fifo_data[fifo_dir] and core_type in lru_data[lru_dir]:
                schedulers = list(fifo_data[fifo_dir][core_type].keys())
                x = np.arange(len(schedulers))
                width = 0.35
                
                fifo_times = [fifo_data[fifo_dir][core_type][s]['exec_time'] for s in schedulers]
                lru_times = [lru_data[lru_dir][core_type][s]['exec_time'] for s in schedulers]
                
                offset = -width/2 if core_type == '1core' else width/2
                label_suffix = ' (1 core)' if core_type == '1core' else ' (8 cores)'
                
                ax2.bar(x + offset - width/2, fifo_times, width/2, label=f'FIFO{label_suffix}',
                       alpha=0.8, color='#FF6B6B')
                ax2.bar(x + offset, lru_times, width/2, label=f'LRU{label_suffix}',
                       alpha=0.8, color='#4ECDC4')
        
        ax2.set_xlabel('Escalonador')
        ax2.set_ylabel('Tempo de Execução (ms)')
        ax2.set_title('Tempo de Execução')
        ax2.set_xticks(x)
        ax2.set_xticklabels(schedulers, rotation=45, ha='right')
        ax2.legend()
        ax2.grid(True, alpha=0.3)
        
        # 3. Improvement Percentage
        ax3 = fig.add_subplot(gs[0, 2])
        improvements = []
        labels = []
        
        for core_type in ['1core', '8cores']:
            if core_type in fifo_data[fifo_dir] and core_type in lru_data[lru_dir]:
                schedulers = list(fifo_data[fifo_dir][core_type].keys())
                for s in schedulers:
                    fifo_hit = fifo_data[fifo_dir][core_type][s]['cache_hit_rate']
                    lru_hit = lru_data[lru_dir][core_type][s]['cache_hit_rate']
                    improvement = lru_hit - fifo_hit
                    improvements.append(improvement)
                    label = f"{s}\n({'1c' if core_type == '1core' else '8c'})"
                    labels.append(label)
        
        colors = ['#4ECDC4' if imp > 0 else '#FF6B6B' for imp in improvements]
        x = np.arange(len(improvements))
        ax3.bar(x, improvements, color=colors, alpha=0.8)
        ax3.axhline(y=0, color='black', linestyle='-', linewidth=0.8)
        ax3.set_xlabel('Escalonador')
        ax3.set_ylabel('Melhoria na Taxa de Hit (%)')
        ax3.set_title('LRU vs FIFO: Ganho/Perda')
        ax3.set_xticks(x)
        ax3.set_xticklabels(labels, rotation=45, ha='right', fontsize=8)
        ax3.grid(True, alpha=0.3, axis='y')
        
        # 4. Throughput Comparison
        ax4 = fig.add_subplot(gs[1, :])
        for core_type in ['1core', '8cores']:
            if core_type in fifo_data[fifo_dir] and core_type in lru_data[lru_dir]:
                schedulers = list(fifo_data[fifo_dir][core_type].keys())
                x = np.arange(len(schedulers))
                width = 0.2
                
                fifo_tput = [fifo_data[fifo_dir][core_type][s]['throughput'] for s in schedulers]
                lru_tput = [lru_data[lru_dir][core_type][s]['throughput'] for s in schedulers]
                
                offset = -width if core_type == '1core' else width
                label_suffix = ' (1 core)' if core_type == '1core' else ' (8 cores)'
                
                ax4.bar(x + offset - width/2, fifo_tput, width, label=f'FIFO{label_suffix}',
                       alpha=0.8, color='#FF6B6B')
                ax4.bar(x + offset + width/2, lru_tput, width, label=f'LRU{label_suffix}',
                       alpha=0.8, color='#4ECDC4')
        
        ax4.set_xlabel('Escalonador')
        ax4.set_ylabel('Throughput (processos/segundo)')
        ax4.set_title('Throughput: FIFO vs LRU')
        ax4.set_xticks(x)
        ax4.set_xticklabels(schedulers, rotation=45, ha='right')
        ax4.legend(loc='upper left', ncol=4)
        ax4.grid(True, alpha=0.3)
        
        plt.savefig(f'{self.output_dir}/extended_03_cache_policy_comparison.png',
                   dpi=300, bbox_inches='tight')
        plt.close()
    
    def generate_detailed_report(self):
        """Gera relatório textual detalhado"""
        if 'single' not in self.data or 'multi' not in self.data:
            return
        
        schedulers = list(self.data['single'].keys())
        cores_multi = self.data['multi'][schedulers[0]]['cores']
        
        report = []
        report.append("=" * 80)
        report.append("  RELATÓRIO DETALHADO DE ANÁLISE DE DESEMPENHO")
        report.append("  Simulador de Arquitetura Multicore Von Neumann")
        report.append("="  * 80)
        report.append("")
        
        report.append("CONFIGURACAO:")
        report.append(f"  - Baseline: Single-core (1 core, sem threading)")
        report.append(f"  - Multi-core: {cores_multi} cores (com threading habilitado)")
        report.append(f"  - Escalonadores testados: {', '.join(schedulers)}")
        report.append(f"  - Processos simulados: {self.data['single'][schedulers[0]]['processes']}")
        report.append("")
        
        report.append("=" * 80)
        report.append("ANALISE DE SPEEDUP (Single-core como baseline)")
        report.append("=" * 80)
        report.append("")
        
        speedups = []
        for s in schedulers:
            single_time = self.data['single'][s]['exec_time']
            multi_time = self.data['multi'][s]['exec_time']
            speedup = single_time / multi_time if multi_time > 0 else 0
            improvement = ((speedup - 1) * 100)
            speedups.append((s, speedup, improvement, single_time, multi_time))
        
        speedups.sort(key=lambda x: x[1], reverse=True)
        
        report.append(f"{'Escalonador':<15} {'Speedup':>10} {'Melhoria':>12} {'Single(ms)':>12} {'Multi(ms)':>12}")
        report.append("-" * 80)
        
        for sched, speedup, improvement, s_time, m_time in speedups:
            status = "OK" if speedup > 1 else "LOSS" if speedup < 1 else "NEUTRAL"
            report.append(f"{sched:<15} {speedup:>9.2f}x {improvement:>10.1f}% {s_time:>11.2f} {m_time:>11.2f}  {status}")
        
        avg_speedup = np.mean([x[1] for x in speedups])
        report.append("-" * 80)
        report.append(f"{'Média':<15} {avg_speedup:>9.2f}x")
        report.append("")
        
        report.append("=" * 80)
        report.append("ANALISE DE CACHE")
        report.append("=" * 80)
        report.append("")
        
        cache_data = []
        for s in schedulers:
            s_cache = self.data['single'][s]['cache_hit_rate']
            m_cache = self.data['multi'][s]['cache_hit_rate']
            degradation = s_cache - m_cache
            cache_data.append((s, s_cache, m_cache, degradation))
        
        cache_data.sort(key=lambda x: x[3], reverse=True)
        
        report.append(f"{'Escalonador':<15} {'Single(%)':>12} {'Multi(%)':>12} {'Degradação':>14}")
        report.append("-" * 80)
        
        for sched, s_cache, m_cache, deg in cache_data:
            status = "HIGH" if deg > 2 else "LOW" if deg < -1 else "NORMAL"
            report.append(f"{sched:<15} {s_cache:>11.2f} {m_cache:>11.2f} {deg:>12.2f}%  {status}")
        
        avg_deg = np.mean([x[3] for x in cache_data])
        report.append("-" * 80)
        report.append(f"{'Média':<15} {'':>12} {'':>12} {avg_deg:>12.2f}%")
        report.append("")
        
        report.append("INTERPRETACAO:")
        if avg_deg > 2:
            report.append("  - Cache pollution significativa no multi-core")
            report.append("  - Context switches frequentes causam mais misses")
        elif avg_deg < -1:
            report.append("  - Multi-core melhora aproveitamento da cache")
            report.append("  - Threads mantem dados relevantes em cache")
        else:
            report.append("  - Impacto neutro na cache entre single e multi-core")
        report.append("")
        
        report.append("=" * 80)
        report.append("ANALISE DE THROUGHPUT")
        report.append("=" * 80)
        report.append("")
        
        thr_data = []
        for s in schedulers:
            s_thr = self.data['single'][s]['throughput']
            m_thr = self.data['multi'][s]['throughput']
            gain = ((m_thr - s_thr) / s_thr) * 100
            thr_data.append((s, s_thr, m_thr, gain))
        
        thr_data.sort(key=lambda x: x[3], reverse=True)
        
        report.append(f"{'Escalonador':<15} {'Single':>12} {'Multi':>12} {'Ganho(%)':>12}")
        report.append("-" * 80)
        
        for sched, s_thr, m_thr, gain in thr_data:
            status = "HIGH" if gain > 10 else "LOW" if gain < -10 else "NORMAL"
            report.append(f"{sched:<15} {s_thr:>11.1f} {m_thr:>11.1f} {gain:>11.1f}  {status}")
        
        avg_gain = np.mean([x[3] for x in thr_data])
        report.append("-" * 80)
        report.append(f"{'Média':<15} {'':>12} {'':>12} {avg_gain:>11.1f}")
        report.append("")
        
        report.append("=" * 80)
        report.append("RECOMENDACOES")
        report.append("=" * 80)
        report.append("")
        
        best_sched, best_speedup, _, _, _ = speedups[0]
        
        report.append("DESEMPENHO:")
        report.append(f"  - Melhor escalonador para multi-core: {best_sched}")
        report.append(f"    Speedup de {best_speedup:.2f}x com {cores_multi} cores")
        report.append("")
        
        if avg_speedup > 1.5:
            report.append("  - Multi-threading traz ganhos significativos")
            report.append(f"    Recomenda-se usar {cores_multi} cores para esta carga")
        elif avg_speedup > 1:
            report.append("  - Multi-threading traz ganhos modestos")
            report.append(f"    Considere overhead de sincronizacao vs ganho de {avg_speedup:.1f}x")
        else:
            report.append("  - Multi-threading nao traz ganhos")
            report.append("    Single-core pode ser mais eficiente para esta carga")
        report.append("")
        
        report.append("CACHE:")
        worst_cache_sched = cache_data[0][0]
        worst_cache_deg = cache_data[0][3]
        
        if worst_cache_deg > 2:
            report.append(f"  - {worst_cache_sched} sofre mais com cache pollution ({worst_cache_deg:.1f}%)")
            report.append("    Considere aumentar tamanho da cache ou ajustar politica")
        else:
            report.append("  - Cache mantem boa taxa de hit no multi-core")
        report.append("")
        
        report.append("=" * 80)
        report.append("")
        
        # Salvar relatório
        report_file = os.path.join(self.output_dir, 'extended_analysis_report.txt')
        with open(report_file, 'w', encoding='utf-8') as f:
            f.write('\n'.join(report))


def main():
    """Função principal"""
    results_dir = 'output'
    if len(sys.argv) > 1:
        results_dir = sys.argv[1]
    
    output_dir = 'plots'
    if len(sys.argv) > 2:
        output_dir = sys.argv[2]
    
    analyzer = ExtendedAnalyzer(output_dir)
    
    if not analyzer.load_data(results_dir):
        return 1
    
    analyzer.plot_scheduler_degradation()
    analyzer.plot_performance_breakdown()
    analyzer.plot_cache_policy_comparison()
    analyzer.generate_detailed_report()
    
    return 0


if __name__ == '__main__':
    sys.exit(main())

#!/usr/bin/env python3
"""
Script simplificado para comparar diferentes políticas de escalonamento
Foco: Demonstrar impacto dos escalonadores em métricas chave
"""

import os
import sys
import re
import matplotlib.pyplot as plt
import numpy as np
from pathlib import Path

# Configurações
plt.style.use('seaborn-v0_8-darkgrid')
COLORS = {
    'FCFS': '#2E86AB',
    'SJN': '#A23B72',
    'Priority': '#F18F01',
    'RoundRobin': '#C73E1D',
}

class SchedulerComparator:
    """Comparador de políticas de escalonamento"""
    
    def __init__(self):
        self.schedulers = {}
    
    def load_comparison_file(self, filepath):
        """Carrega arquivo de comparação gerado pelo simulador"""
        if not os.path.exists(filepath):
            return False
        
        with open(filepath, 'r', encoding='utf-8') as f:
            content = f.read()
        
        # Encontrar seções de métricas
        self._parse_performance_section(content)
        self._parse_scheduling_section(content)
        self._parse_memory_section(content)
        
        return len(self.schedulers) > 0
    
    def _parse_performance_section(self, content):
        """Parseia seção de desempenho geral"""
        section_match = re.search(r'=== DESEMPENHO GERAL ===(.*?)(?===|$)', content, re.DOTALL)
        if not section_match:
            return
        
        section = section_match.group(1)
        lines = section.split('\n')
        
        for line in lines:
            parts = line.split()
            if len(parts) >= 6 and parts[0] in ['FCFS', 'SJN', 'Priority', 'RoundRobin']:
                name = parts[0]
                if name not in self.schedulers:
                    self.schedulers[name] = {}
                
                self.schedulers[name]['exec_time'] = float(parts[1])
                self.schedulers[name]['throughput'] = float(parts[2])
                self.schedulers[name]['processes'] = int(parts[3])
                self.schedulers[name]['ctx_switches'] = int(parts[4])
                self.schedulers[name]['cpu_cycles'] = int(parts[5])
    
    def _parse_scheduling_section(self, content):
        """Parseia seção de métricas de escalonamento"""
        section_match = re.search(r'=== MÉTRICAS DE ESCALONAMENTO ===(.*?)(?===|$)', content, re.DOTALL)
        if not section_match:
            return
        
        section = section_match.group(1)
        lines = section.split('\n')
        
        for line in lines:
            parts = line.split()
            if len(parts) >= 5 and parts[0] in ['FCFS', 'SJN', 'Priority', 'RoundRobin']:
                name = parts[0]
                if name in self.schedulers:
                    self.schedulers[name]['wait_time'] = float(parts[1])
                    self.schedulers[name]['turnaround_time'] = float(parts[2])
                    self.schedulers[name]['response_time'] = float(parts[3])
                    self.schedulers[name]['cpu_util'] = float(parts[4])
    
    def _parse_memory_section(self, content):
        """Parseia seção de eficiência de cache"""
        section_match = re.search(r'=== EFICIÊNCIA DE CACHE E MEMÓRIA ===(.*?)(?===|$)', content, re.DOTALL)
        if not section_match:
            return
        
        section = section_match.group(1)
        lines = section.split('\n')
        
        for line in lines:
            parts = line.split()
            if len(parts) >= 2 and parts[0] in ['FCFS', 'SJN', 'Priority', 'RoundRobin']:
                name = parts[0]
                if name in self.schedulers:
                    self.schedulers[name]['cache_hit_rate'] = float(parts[1])
    
    def generate_plots(self, output_dir='plots'):
        """Gera gráficos comparativos"""
        os.makedirs(output_dir, exist_ok=True)
        
        print(f"\n📊 Gerando comparações para {len(self.schedulers)} escalonadores...\n")
        
        self.plot_time_comparison(output_dir)
        self.plot_cache_comparison(output_dir)
        self.plot_efficiency_radar(output_dir)
        self.plot_summary_matrix(output_dir)
        
        print(f"\n✅ Gráficos comparativos salvos em: {output_dir}/\n")
    
    def plot_time_comparison(self, output_dir):
        """Compara tempos entre escalonadores"""
        fig, ax = plt.subplots(figsize=(12, 6))
        
        names = list(self.schedulers.keys())
        x = np.arange(len(names))
        width = 0.25
        
        wait = [self.schedulers[n].get('wait_time', 0) for n in names]
        response = [self.schedulers[n].get('response_time', 0) for n in names]
        turnaround = [self.schedulers[n].get('turnaround_time', 0) for n in names]
        
        colors = [COLORS.get(n, '#666666') for n in names]
        
        ax.bar(x - width, wait, width, label='Tempo Espera', alpha=0.8)
        ax.bar(x, response, width, label='Tempo Resposta', alpha=0.8)
        ax.bar(x + width, turnaround, width, label='Tempo Retorno', alpha=0.8)
        
        ax.set_xlabel('Escalonador', fontsize=12, fontweight='bold')
        ax.set_ylabel('Tempo Médio (ms)', fontsize=12, fontweight='bold')
        ax.set_title('Comparação de Tempos Entre Escalonadores', fontsize=14, fontweight='bold')
        ax.set_xticks(x)
        ax.set_xticklabels(names)
        ax.legend()
        ax.grid(axis='y', alpha=0.3)
        
        plt.tight_layout()
        plt.savefig(f'{output_dir}/comp_1_times.png', dpi=300, bbox_inches='tight')
        plt.close()
        print("  ✓ comp_1_times.png")
    
    def plot_cache_comparison(self, output_dir):
        """Compara performance de cache"""
        fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5))
        
        names = list(self.schedulers.keys())
        cache_rates = [self.schedulers[n].get('cache_hit_rate', 0) for n in names]
        ctx_switches = [self.schedulers[n].get('ctx_switches', 0) for n in names]
        
        colors = [COLORS.get(n, '#666666') for n in names]
        
        # Taxa de cache hit
        bars1 = ax1.bar(names, cache_rates, color=colors, alpha=0.9)
        ax1.set_ylabel('Taxa de Cache Hit (%)', fontsize=11, fontweight='bold')
        ax1.set_title('Eficiência de Cache por Escalonador', fontsize=13, fontweight='bold')
        ax1.set_ylim(0, max(cache_rates) * 1.2)
        ax1.grid(axis='y', alpha=0.3)
        
        # Adicionar valores
        for bar, rate in zip(bars1, cache_rates):
            height = bar.get_height()
            ax1.text(bar.get_x() + bar.get_width()/2., height,
                    f'{rate:.1f}%', ha='center', va='bottom', fontsize=10, fontweight='bold')
        
        # Context switches
        bars2 = ax2.bar(names, ctx_switches, color=colors, alpha=0.9)
        ax2.set_ylabel('Número de Trocas de Contexto', fontsize=11, fontweight='bold')
        ax2.set_title('Context Switches por Escalonador', fontsize=13, fontweight='bold')
        ax2.grid(axis='y', alpha=0.3)
        
        # Adicionar valores
        for bar, switches in zip(bars2, ctx_switches):
            height = bar.get_height()
            ax2.text(bar.get_x() + bar.get_width()/2., height,
                    f'{switches}', ha='center', va='bottom', fontsize=10, fontweight='bold')
        
        plt.tight_layout()
        plt.savefig(f'{output_dir}/comp_2_cache_switches.png', dpi=300, bbox_inches='tight')
        plt.close()
        print("  ✓ comp_2_cache_switches.png")
    
    def plot_efficiency_radar(self, output_dir):
        """Gráfico radar comparativo (normalizado: maior = melhor)"""
        categories = ['Tempo\nResposta', 'Tempo\nEspera', 'Tempo\nRetorno', 
                     'Cache\nHit', 'CPU\nUtil.']
        
        fig, ax = plt.subplots(figsize=(10, 10), subplot_kw=dict(projection='polar'))
        
        angles = np.linspace(0, 2 * np.pi, len(categories), endpoint=False).tolist()
        angles += angles[:1]  # Fechar o polígono
        
        # Normalizar métricas (inverter tempos: menor é melhor)
        max_response = max(self.schedulers[n].get('response_time', 1) for n in self.schedulers)
        max_wait = max(self.schedulers[n].get('wait_time', 1) for n in self.schedulers)
        max_turnaround = max(self.schedulers[n].get('turnaround_time', 1) for n in self.schedulers)
        
        for name in self.schedulers.keys():
            metrics = self.schedulers[name]
            
            # Normalizar: 100 = melhor, 0 = pior
            values = [
                100 - (metrics.get('response_time', 0) / max_response * 100),  # Inverter
                100 - (metrics.get('wait_time', 0) / max_wait * 100),  # Inverter
                100 - (metrics.get('turnaround_time', 0) / max_turnaround * 100),  # Inverter
                metrics.get('cache_hit_rate', 0),  # Já é %
                metrics.get('cpu_util', 0),  # Já é %
            ]
            values += values[:1]
            
            color = COLORS.get(name, '#666666')
            ax.plot(angles, values, 'o-', linewidth=2, label=name, color=color)
            ax.fill(angles, values, alpha=0.15, color=color)
        
        ax.set_xticks(angles[:-1])
        ax.set_xticklabels(categories, size=11)
        ax.set_ylim(0, 100)
        ax.set_yticks([20, 40, 60, 80, 100])
        ax.set_yticklabels(['20%', '40%', '60%', '80%', '100%'])
        ax.legend(loc='upper right', bbox_to_anchor=(1.3, 1.1))
        ax.grid(True)
        ax.set_title('Comparação de Desempenho Normalizado\n(Pontos mais distantes = Melhor)', 
                    size=14, fontweight='bold', pad=20)
        
        plt.tight_layout()
        plt.savefig(f'{output_dir}/comp_3_radar.png', dpi=300, bbox_inches='tight')
        plt.close()
        print("  ✓ comp_3_radar.png")
    
    def plot_summary_matrix(self, output_dir):
        """Matriz resumo de todas as métricas"""
        fig = plt.figure(figsize=(14, 10))
        gs = fig.add_gridspec(3, 2, hspace=0.35, wspace=0.3)
        
        names = list(self.schedulers.keys())
        colors = [COLORS.get(n, '#666666') for n in names]
        
        # 1. Tempo de execução
        ax1 = fig.add_subplot(gs[0, 0])
        exec_times = [self.schedulers[n].get('exec_time', 0) for n in names]
        ax1.bar(names, exec_times, color=colors, alpha=0.9)
        ax1.set_ylabel('Tempo (ms)', fontweight='bold')
        ax1.set_title('Tempo de Execução Total', fontweight='bold', fontsize=12)
        ax1.grid(axis='y', alpha=0.3)
        
        # 2. Throughput
        ax2 = fig.add_subplot(gs[0, 1])
        throughput = [self.schedulers[n].get('throughput', 0) for n in names]
        ax2.bar(names, throughput, color=colors, alpha=0.9)
        ax2.set_ylabel('Processos/segundo', fontweight='bold')
        ax2.set_title('Throughput', fontweight='bold', fontsize=12)
        ax2.grid(axis='y', alpha=0.3)
        
        # 3. Tempo médio de resposta
        ax3 = fig.add_subplot(gs[1, 0])
        response = [self.schedulers[n].get('response_time', 0) for n in names]
        ax3.bar(names, response, color=colors, alpha=0.9)
        ax3.set_ylabel('Tempo (ms)', fontweight='bold')
        ax3.set_title('Tempo Médio de Resposta', fontweight='bold', fontsize=12)
        ax3.grid(axis='y', alpha=0.3)
        
        # 4. Context switches
        ax4 = fig.add_subplot(gs[1, 1])
        switches = [self.schedulers[n].get('ctx_switches', 0) for n in names]
        ax4.bar(names, switches, color=colors, alpha=0.9)
        ax4.set_ylabel('Quantidade', fontweight='bold')
        ax4.set_title('Context Switches', fontweight='bold', fontsize=12)
        ax4.grid(axis='y', alpha=0.3)
        
        # 5. Taxa de cache hit
        ax5 = fig.add_subplot(gs[2, 0])
        cache = [self.schedulers[n].get('cache_hit_rate', 0) for n in names]
        bars = ax5.bar(names, cache, color=colors, alpha=0.9)
        ax5.set_ylabel('Taxa (%)', fontweight='bold')
        ax5.set_title('Taxa de Cache Hit', fontweight='bold', fontsize=12)
        ax5.set_ylim(0, 100)
        ax5.grid(axis='y', alpha=0.3)
        
        # 6. Utilização de CPU
        ax6 = fig.add_subplot(gs[2, 1])
        cpu_util = [self.schedulers[n].get('cpu_util', 0) for n in names]
        ax6.bar(names, cpu_util, color=colors, alpha=0.9)
        ax6.set_ylabel('Utilização (%)', fontweight='bold')
        ax6.set_title('Utilização de CPU', fontweight='bold', fontsize=12)
        ax6.set_ylim(0, 100)
        ax6.grid(axis='y', alpha=0.3)
        
        plt.suptitle('Matriz Comparativa Completa de Escalonadores', 
                    fontsize=16, fontweight='bold', y=0.99)
        plt.savefig(f'{output_dir}/comp_4_matrix.png', dpi=300, bbox_inches='tight')
        plt.close()
        print("  ✓ comp_4_matrix.png")


def main():
    """Função principal"""
    print("=" * 70)
    print("   COMPARADOR DE ESCALONADORES - Simulador Von Neumann")
    print("=" * 70)
    
    # Procurar arquivo de comparação
    possible_files = [
        'output/comparacao_escalonadores.txt',
        'output/comparacao_escalonadores_multicore_2cores.txt',
        'output/comparacao_escalonadores_multicore_4cores.txt',
        'output/comparacao_escalonadores_multicore_8cores.txt',
        'build/output/comparacao_escalonadores.txt',
    ]
    
    comparison_file = None
    if len(sys.argv) > 1:
        comparison_file = sys.argv[1]
    else:
        for f in possible_files:
            if os.path.exists(f):
                comparison_file = f
                break
    
    if not comparison_file:
        print("\n❌ Arquivo de comparação não encontrado!")
        print("   Execute o simulador com opção '5' (comparar todos)")
        print("   ou especifique: python compare_schedulers.py <arquivo.txt>")
        return 1
    
    print(f"\n📂 Arquivo: {comparison_file}")
    
    # Carregar e comparar
    comparator = SchedulerComparator()
    if not comparator.load_comparison_file(comparison_file):
        print("\n❌ Erro ao carregar arquivo de comparação!")
        return 1
    
    print(f"   {len(comparator.schedulers)} escalonadores encontrados")
    
    # Gerar gráficos
    output_dir = 'plots'
    if len(sys.argv) > 2:
        output_dir = sys.argv[2]
    
    comparator.generate_plots(output_dir)
    
    print("=" * 70)
    print("✅ Comparação concluída!")
    print("=" * 70)
    
    return 0


if __name__ == '__main__':
    sys.exit(main())

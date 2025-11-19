# Script para gerar gráficos dos resultados do Simulador Multicore Von Neumann


import matplotlib.pyplot as plt
import numpy as np
import os
import sys
import re
from pathlib import Path

# Configurações de estilo
plt.style.use('seaborn-v0_8-darkgrid')
COLORS = ['#2E86AB', '#A23B72', '#F18F01', '#C73E1D', '#6A994E']

class ResultsParser:
    # Parser para arquivos de resultados do simulador
    
    def __init__(self, results_file):
        self.results_file = results_file
        self.processes = []
        
    def parse(self):
        # Lê e parseia o arquivo de resultados
        if not os.path.exists(self.results_file):
            print(f"❌ Arquivo não encontrado: {self.results_file}")
            return False
            
        with open(self.results_file, 'r', encoding='utf-8') as f:
            content = f.read()
            
        # Divide por processos
        process_blocks = content.split('--- METRICAS FINAIS DO PROCESSO')
        
        for block in process_blocks[1:]:  # Pula o primeiro (vazio)
            process_data = self._parse_process_block(block)
            if process_data:
                self.processes.append(process_data)
                
        return len(self.processes) > 0
    
    def _parse_process_block(self, block):
        # Parseia um bloco de processo individual
        data = {}
        
        # PID
        pid_match = re.search(r'(\d+) ---', block)
        if pid_match:
            data['pid'] = int(pid_match.group(1))
        
        # Nome
        name_match = re.search(r'Nome do Processo:\s+(.+)', block)
        if name_match:
            data['name'] = name_match.group(1).strip()
        
        # Prioridade
        priority_match = re.search(r'Prioridade:\s+(\d+)', block)
        if priority_match:
            data['priority'] = int(priority_match.group(1))
        
        # Quantum
        quantum_match = re.search(r'Quantum:\s+(\d+)', block)
        if quantum_match:
            data['quantum'] = int(quantum_match.group(1))
        
        # Métricas de tempo
        wait_match = re.search(r'Tempo de Espera:\s+(\d+)', block)
        if wait_match:
            data['wait_time'] = int(wait_match.group(1))
        
        response_match = re.search(r'Tempo de Resposta:\s+(\d+)', block)
        if response_match:
            data['response_time'] = int(response_match.group(1))
        
        turnaround_match = re.search(r'Tempo de Retorno:\s+(\d+)', block)
        if turnaround_match:
            data['turnaround_time'] = int(turnaround_match.group(1))
        
        # Métricas de CPU e Memória
        pipeline_match = re.search(r'Ciclos de Pipeline:\s+(\d+)', block)
        if pipeline_match:
            data['pipeline_cycles'] = int(pipeline_match.group(1))
        
        mem_accesses_match = re.search(r'Total de Acessos a Mem:\s+(\d+)', block)
        if mem_accesses_match:
            data['mem_accesses'] = int(mem_accesses_match.group(1))
        
        cache_hits_match = re.search(r'Cache Hits:\s+(\d+)', block)
        if cache_hits_match:
            data['cache_hits'] = int(cache_hits_match.group(1))
        
        cache_misses_match = re.search(r'Cache Misses:\s+(\d+)', block)
        if cache_misses_match:
            data['cache_misses'] = int(cache_misses_match.group(1))
        
        # Taxa de cache
        if 'cache_hits' in data and 'mem_accesses' in data and data['mem_accesses'] > 0:
            data['cache_hit_rate'] = (data['cache_hits'] / data['mem_accesses']) * 100
        else:
            data['cache_hit_rate'] = 0.0
        
        return data if data else None


class PlotGenerator:
    # Gerador de gráficos
    
    def __init__(self, output_dir='plots'):
        self.output_dir = output_dir
        os.makedirs(output_dir, exist_ok=True)
    
    def generate_all_plots(self, processes):
        # Gera todos os gráficos
        print(f"\nGerando gráficos para {len(processes)} processos...")
        
        self.plot_execution_times(processes)
        self.plot_memory_metrics(processes)
        self.plot_cache_performance(processes)
        self.plot_pipeline_cycles(processes)
        self.plot_comparison_matrix(processes)
        
        print(f"Gráficos salvos em: {self.output_dir}/")
    
    def plot_execution_times(self, processes):
        # Gráfico de tempos de execução
        fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5))
        
        names = [p['name'] for p in processes]
        wait_times = [p.get('wait_time', 0) for p in processes]
        turnaround_times = [p.get('turnaround_time', 0) for p in processes]
        
        # Gráfico de barras - Tempo de Espera
        bars1 = ax1.bar(range(len(names)), wait_times, color=COLORS[0], alpha=0.8)
        ax1.set_xlabel('Processo', fontsize=11, fontweight='bold')
        ax1.set_ylabel('Tempo (ms)', fontsize=11, fontweight='bold')
        ax1.set_title('Tempo de Espera por Processo', fontsize=13, fontweight='bold')
        ax1.set_xticks(range(len(names)))
        ax1.set_xticklabels([f"P{p['pid']}" for p in processes], rotation=45)
        ax1.grid(axis='y', alpha=0.3)
        
        # Adicionar valores nas barras
        for bar in bars1:
            height = bar.get_height()
            ax1.text(bar.get_x() + bar.get_width()/2., height,
                    f'{int(height)}',
                    ha='center', va='bottom', fontsize=9)
        
        # Gráfico de barras - Tempo de Retorno
        bars2 = ax2.bar(range(len(names)), turnaround_times, color=COLORS[1], alpha=0.8)
        ax2.set_xlabel('Processo', fontsize=11, fontweight='bold')
        ax2.set_ylabel('Tempo (ms)', fontsize=11, fontweight='bold')
        ax2.set_title('Tempo de Retorno por Processo', fontsize=13, fontweight='bold')
        ax2.set_xticks(range(len(names)))
        ax2.set_xticklabels([f"P{p['pid']}" for p in processes], rotation=45)
        ax2.grid(axis='y', alpha=0.3)
        
        # Adicionar valores nas barras
        for bar in bars2:
            height = bar.get_height()
            ax2.text(bar.get_x() + bar.get_width()/2., height,
                    f'{int(height)}',
                    ha='center', va='bottom', fontsize=9)
        
        plt.tight_layout()
        plt.savefig(f'{self.output_dir}/execution_times.png', dpi=300, bbox_inches='tight')
        plt.close()
        print("execution_times.png")
    
    def plot_memory_metrics(self, processes):
        # Gráfico de métricas de memória
        fig, ax = plt.subplots(figsize=(12, 6))
        
        names = [f"P{p['pid']}" for p in processes]
        mem_accesses = [p.get('mem_accesses', 0) for p in processes]
        
        bars = ax.bar(range(len(names)), mem_accesses, color=COLORS[2], alpha=0.8)
        ax.set_xlabel('Processo', fontsize=11, fontweight='bold')
        ax.set_ylabel('Número de Acessos', fontsize=11, fontweight='bold')
        ax.set_title('Acessos à Memória por Processo', fontsize=13, fontweight='bold')
        ax.set_xticks(range(len(names)))
        ax.set_xticklabels(names, rotation=45)
        ax.grid(axis='y', alpha=0.3)
        
        # Adicionar valores nas barras
        for bar in bars:
            height = bar.get_height()
            ax.text(bar.get_x() + bar.get_width()/2., height,
                    f'{int(height)}',
                    ha='center', va='bottom', fontsize=9)
        
        plt.tight_layout()
        plt.savefig(f'{self.output_dir}/memory_accesses.png', dpi=300, bbox_inches='tight')
        plt.close()
        print("memory_accesses.png")
    
    def plot_cache_performance(self, processes):
        # Gráfico de desempenho de cache
        fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5))
        
        names = [f"P{p['pid']}" for p in processes]
        cache_hits = [p.get('cache_hits', 0) for p in processes]
        cache_misses = [p.get('cache_misses', 0) for p in processes]
        hit_rates = [p.get('cache_hit_rate', 0) for p in processes]
        
        # Gráfico empilhado - Hits vs Misses
        x = np.arange(len(names))
        width = 0.6
        
        bars1 = ax1.bar(x, cache_hits, width, label='Cache Hits', color=COLORS[3], alpha=0.8)
        bars2 = ax1.bar(x, cache_misses, width, bottom=cache_hits, 
                       label='Cache Misses', color=COLORS[4], alpha=0.8)
        
        ax1.set_xlabel('Processo', fontsize=11, fontweight='bold')
        ax1.set_ylabel('Número de Acessos', fontsize=11, fontweight='bold')
        ax1.set_title('Cache Hits vs Cache Misses', fontsize=13, fontweight='bold')
        ax1.set_xticks(x)
        ax1.set_xticklabels(names, rotation=45)
        ax1.legend()
        ax1.grid(axis='y', alpha=0.3)
        
        # Gráfico de linha - Taxa de Hit
        ax2.plot(range(len(names)), hit_rates, marker='o', linewidth=2, 
                markersize=8, color=COLORS[0])
        ax2.fill_between(range(len(names)), hit_rates, alpha=0.3, color=COLORS[0])
        ax2.set_xlabel('Processo', fontsize=11, fontweight='bold')
        ax2.set_ylabel('Taxa de Hit (%)', fontsize=11, fontweight='bold')
        ax2.set_title('Taxa de Cache Hit por Processo', fontsize=13, fontweight='bold')
        ax2.set_xticks(range(len(names)))
        ax2.set_xticklabels(names, rotation=45)
        ax2.set_ylim(0, max(hit_rates) * 1.2 if hit_rates else 100)
        ax2.grid(alpha=0.3)
        
        # Adicionar valores nos pontos
        for i, rate in enumerate(hit_rates):
            ax2.text(i, rate + 1, f'{rate:.1f}%', ha='center', fontsize=9)
        
        plt.tight_layout()
        plt.savefig(f'{self.output_dir}/cache_performance.png', dpi=300, bbox_inches='tight')
        plt.close()
        print("cache_performance.png")
    
    def plot_pipeline_cycles(self, processes):
        # Gráfico de ciclos de pipeline
        fig, ax = plt.subplots(figsize=(12, 6))
        
        names = [f"P{p['pid']}" for p in processes]
        pipeline_cycles = [p.get('pipeline_cycles', 0) for p in processes]
        
        bars = ax.barh(range(len(names)), pipeline_cycles, color=COLORS[1], alpha=0.8)
        ax.set_ylabel('Processo', fontsize=11, fontweight='bold')
        ax.set_xlabel('Ciclos de Pipeline', fontsize=11, fontweight='bold')
        ax.set_title('Ciclos de Pipeline por Processo', fontsize=13, fontweight='bold')
        ax.set_yticks(range(len(names)))
        ax.set_yticklabels(names)
        ax.grid(axis='x', alpha=0.3)
        
        # Adicionar valores nas barras
        for i, bar in enumerate(bars):
            width = bar.get_width()
            ax.text(width + 0.5, bar.get_y() + bar.get_height()/2.,
                    f'{int(width)}',
                    ha='left', va='center', fontsize=9)
        
        plt.tight_layout()
        plt.savefig(f'{self.output_dir}/pipeline_cycles.png', dpi=300, bbox_inches='tight')
        plt.close()
        print("pipeline_cycles.png")
    
    def plot_comparison_matrix(self, processes):
        # Matriz de comparação de múltiplas métricas
        fig, ((ax1, ax2), (ax3, ax4)) = plt.subplots(2, 2, figsize=(14, 10))
        
        names = [f"P{p['pid']}" for p in processes]
        
        # 1. Tempo de Espera
        wait_times = [p.get('wait_time', 0) for p in processes]
        ax1.bar(range(len(names)), wait_times, color=COLORS[0], alpha=0.7)
        ax1.set_title('Tempo de Espera (ms)', fontweight='bold')
        ax1.set_xticks(range(len(names)))
        ax1.set_xticklabels(names, rotation=45)
        ax1.grid(axis='y', alpha=0.3)
        
        # 2. Acessos à Memória
        mem_accesses = [p.get('mem_accesses', 0) for p in processes]
        ax2.bar(range(len(names)), mem_accesses, color=COLORS[2], alpha=0.7)
        ax2.set_title('Acessos à Memória', fontweight='bold')
        ax2.set_xticks(range(len(names)))
        ax2.set_xticklabels(names, rotation=45)
        ax2.grid(axis='y', alpha=0.3)
        
        # 3. Taxa de Cache Hit
        hit_rates = [p.get('cache_hit_rate', 0) for p in processes]
        ax3.bar(range(len(names)), hit_rates, color=COLORS[3], alpha=0.7)
        ax3.set_title('Taxa de Cache Hit (%)', fontweight='bold')
        ax3.set_xticks(range(len(names)))
        ax3.set_xticklabels(names, rotation=45)
        ax3.grid(axis='y', alpha=0.3)
        
        # 4. Ciclos de Pipeline
        pipeline_cycles = [p.get('pipeline_cycles', 0) for p in processes]
        ax4.bar(range(len(names)), pipeline_cycles, color=COLORS[1], alpha=0.7)
        ax4.set_title('Ciclos de Pipeline', fontweight='bold')
        ax4.set_xticks(range(len(names)))
        ax4.set_xticklabels(names, rotation=45)
        ax4.grid(axis='y', alpha=0.3)
        
        plt.suptitle('Matriz de Comparação de Métricas', fontsize=15, fontweight='bold', y=0.995)
        plt.tight_layout()
        plt.savefig(f'{self.output_dir}/comparison_matrix.png', dpi=300, bbox_inches='tight')
        plt.close()
        print("comparison_matrix.png")


def main():
    """Função principal"""
    print("=" * 70)
    print("GERADOR DE GRÁFICOS - SIMULADOR MULTICORE VON NEUMANN")
    print("=" * 70)
    
    # Determinar diretório de resultados
    if len(sys.argv) > 1:
        results_file = sys.argv[1]
    else:
        # Procura em locais padrões
        possible_paths = [
            'output/resultados.dat',
            'build/output/resultados.dat',
            '../build/output/resultados.dat'
        ]
        
        results_file = None
        for path in possible_paths:
            if os.path.exists(path):
                results_file = path
                break
        
        if not results_file:
            print("Arquivo de resultados não encontrado!")
            print("\nUso: python3 plot_results.py [caminho/para/resultados.dat]")
            print("\nOu execute o simulador primeiro para gerar os resultados.")
            return 1
    
    print(f"\nArquivo de resultados: {results_file}")
    
    # Parser
    parser = ResultsParser(results_file)
    if not parser.parse():
        print("❌ Erro ao parsear arquivo de resultados!")
        return 1
    
    print(f"{len(parser.processes)} processos encontrados")
    
    # Gerar gráficos
    output_dir = 'plots'
    if len(sys.argv) > 2:
        output_dir = sys.argv[2]
    
    generator = PlotGenerator(output_dir)
    generator.generate_all_plots(parser.processes)
    
    print("\n" + "=" * 70)
    print("Gráficos gerados com sucesso!")
    print(f"Localização: {output_dir}/")
    print("=" * 70)
    
    return 0


if __name__ == '__main__':
    sys.exit(main())

# Script para visualizar uso de memória ao longo do tempo


import matplotlib.pyplot as plt
import numpy as np
import os
import sys
import re
from pathlib import Path
import glob

# Configurações de estilo
plt.style.use('seaborn-v0_8-darkgrid')
COLORS = plt.cm.tab20.colors

class MemoryVisualizer:
    # Visualizador de uso de memória
    
    def __init__(self, output_dir='plots'):
        self.output_dir = output_dir
        self.memory_data = []
        os.makedirs(output_dir, exist_ok=True)
    
    def load_memory_files(self, directory):
        # Carrega arquivos de uso de memória
        pattern = os.path.join(directory, 'memory_usage_*.txt')
        files = glob.glob(pattern)
        
        if not files:
            print(f"Nenhum arquivo de memória encontrado em {directory}")
            return False
        
        print(f"\nEncontrados {len(files)} arquivos de memória")
        
        for file in sorted(files):
            data = self._parse_memory_file(file)
            if data:
                self.memory_data.append(data)
        
        return len(self.memory_data) > 0
    
    def _parse_memory_file(self, filepath):
        # Parseia um arquivo de memória individual
        with open(filepath, 'r', encoding='utf-8') as f:
            content = f.read()
        
        # Extrair nome e PID
        name_match = re.search(r'Processo:\s+(.+?)\s+\(PID:\s+(\d+)\)', content)
        if not name_match:
            return None
        
        process_name = name_match.group(1).strip()
        pid = int(name_match.group(2))
        
        # Extrair snapshots
        snapshots = []
        lines = content.split('\n')
        
        in_data = False
        for line in lines:
            if 'Tempo(ms)' in line:
                in_data = True
                continue
            
            if in_data and line.strip() and not line.startswith('==='):
                parts = line.split()
                if len(parts) >= 5:
                    try:
                        timestamp = int(parts[0])
                        cache = int(parts[1])
                        ram = int(parts[2])
                        accesses = int(parts[3])
                        hit_rate = float(parts[4])
                        
                        snapshots.append({
                            'timestamp': timestamp,
                            'cache': cache,
                            'ram': ram,
                            'accesses': accesses,
                            'hit_rate': hit_rate
                        })
                    except (ValueError, IndexError):
                        continue
        
        if not snapshots:
            return None
        
        return {
            'name': process_name,
            'pid': pid,
            'snapshots': snapshots
        }
    
    def plot_memory_usage_over_time(self):
        # Gráfico de uso de memória ao longo do tempo
        if not self.memory_data:
            return
        
        fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(14, 10))
        
        for i, process in enumerate(self.memory_data):
            if not process['snapshots']:
                continue
            
            # Normalizar timestamps (começar do 0)
            times = [s['timestamp'] for s in process['snapshots']]
            if times:
                min_time = min(times)
                times = [t - min_time for t in times]
            
            cache_usage = [s['cache'] for s in process['snapshots']]
            ram_usage = [s['ram'] for s in process['snapshots']]
            
            label = f"P{process['pid']} - {process['name']}"
            color = COLORS[i % len(COLORS)]
            
            # Gráfico 1: Cache
            ax1.plot(times, cache_usage, marker='o', linewidth=2, 
                    markersize=5, label=label, color=color, alpha=0.7)
            
            # Gráfico 2: RAM
            ax2.plot(times, ram_usage, marker='s', linewidth=2, 
                    markersize=5, label=label, color=color, alpha=0.7)
        
        # Configurar eixos - Cache
        ax1.set_xlabel('Tempo Relativo (ms)', fontsize=11, fontweight='bold')
        ax1.set_ylabel('Uso de Cache (bytes)', fontsize=11, fontweight='bold')
        ax1.set_title('Uso de Cache L1 ao Longo do Tempo', fontsize=13, fontweight='bold')
        ax1.legend(bbox_to_anchor=(1.05, 1), loc='upper left', fontsize=8)
        ax1.grid(alpha=0.3)
        
        # Configurar eixos - RAM
        ax2.set_xlabel('Tempo Relativo (ms)', fontsize=11, fontweight='bold')
        ax2.set_ylabel('Uso de RAM (bytes)', fontsize=11, fontweight='bold')
        ax2.set_title('Uso de RAM ao Longo do Tempo', fontsize=13, fontweight='bold')
        ax2.legend(bbox_to_anchor=(1.05, 1), loc='upper left', fontsize=8)
        ax2.grid(alpha=0.3)
        
        plt.tight_layout()
        plt.savefig(f'{self.output_dir}/memory_usage_timeline.png', 
                   dpi=300, bbox_inches='tight')
        plt.close()
        print("memory_usage_timeline.png")
    
    def plot_cache_hit_rate_evolution(self):
        """Evolução da taxa de cache hit"""
        if not self.memory_data:
            return
        
        fig, ax = plt.subplots(figsize=(14, 6))
        
        for i, process in enumerate(self.memory_data):
            if not process['snapshots']:
                continue
            
            # Normalizar timestamps
            times = [s['timestamp'] for s in process['snapshots']]
            if times:
                min_time = min(times)
                times = [t - min_time for t in times]
            
            hit_rates = [s['hit_rate'] for s in process['snapshots']]
            
            label = f"P{process['pid']} - {process['name']}"
            color = COLORS[i % len(COLORS)]
            
            ax.plot(times, hit_rates, marker='o', linewidth=2, 
                   markersize=5, label=label, color=color, alpha=0.7)
        
        ax.set_xlabel('Tempo Relativo (ms)', fontsize=12, fontweight='bold')
        ax.set_ylabel('Taxa de Cache Hit (%)', fontsize=12, fontweight='bold')
        ax.set_title('Evolução da Taxa de Cache Hit por Processo', 
                    fontsize=14, fontweight='bold')
        ax.legend(bbox_to_anchor=(1.05, 1), loc='upper left', fontsize=8)
        ax.grid(alpha=0.3)
        ax.set_ylim(0, 100)
        
        plt.tight_layout()
        plt.savefig(f'{self.output_dir}/cache_hit_rate_evolution.png', 
                   dpi=300, bbox_inches='tight')
        plt.close()
        print("cache_hit_rate_evolution.png")
    
    def plot_memory_heatmap(self):
        # Mapa de calor de uso de memória
        if not self.memory_data:
            return
        
        # Preparar dados para heatmap
        max_snapshots = max(len(p['snapshots']) for p in self.memory_data)
        cache_matrix = np.zeros((len(self.memory_data), max_snapshots))
        
        process_labels = []
        
        for i, process in enumerate(self.memory_data):
            process_labels.append(f"P{process['pid']}")
            for j, snapshot in enumerate(process['snapshots']):
                cache_matrix[i, j] = snapshot['cache']
        
        fig, ax = plt.subplots(figsize=(14, 8))
        
        im = ax.imshow(cache_matrix, cmap='YlOrRd', aspect='auto', interpolation='nearest')
        
        # Configurar eixos
        ax.set_xlabel('Snapshot Index', fontsize=12, fontweight='bold')
        ax.set_ylabel('Processo', fontsize=12, fontweight='bold')
        ax.set_title('Mapa de Calor: Uso de Cache por Processo ao Longo do Tempo', 
                    fontsize=14, fontweight='bold')
        
        ax.set_yticks(range(len(process_labels)))
        ax.set_yticklabels(process_labels)
        
        # Colorbar
        cbar = plt.colorbar(im, ax=ax)
        cbar.set_label('Cache Usage (bytes)', fontsize=11, fontweight='bold')
        
        plt.tight_layout()
        plt.savefig(f'{self.output_dir}/memory_heatmap.png', 
                   dpi=300, bbox_inches='tight')
        plt.close()
        print("memory_heatmap.png")
    
    def plot_final_memory_summary(self):
        # Resumo final de uso de memória
        if not self.memory_data:
            return
        
        fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))
        
        names = [f"P{p['pid']}" for p in self.memory_data]
        max_cache = [max(s['cache'] for s in p['snapshots']) if p['snapshots'] else 0 
                    for p in self.memory_data]
        max_ram = [max(s['ram'] for s in p['snapshots']) if p['snapshots'] else 0 
                  for p in self.memory_data]
        
        # Gráfico 1: Cache máxima
        bars1 = ax1.bar(range(len(names)), max_cache, color=COLORS[0], alpha=0.8)
        ax1.set_xlabel('Processo', fontsize=11, fontweight='bold')
        ax1.set_ylabel('Bytes', fontsize=11, fontweight='bold')
        ax1.set_title('Uso Máximo de Cache por Processo', fontsize=13, fontweight='bold')
        ax1.set_xticks(range(len(names)))
        ax1.set_xticklabels(names, rotation=45)
        ax1.grid(axis='y', alpha=0.3)
        
        for bar in bars1:
            height = bar.get_height()
            if height > 0:
                ax1.text(bar.get_x() + bar.get_width()/2., height,
                        f'{int(height)}',
                        ha='center', va='bottom', fontsize=8)
        
        # Gráfico 2: RAM máxima
        bars2 = ax2.bar(range(len(names)), max_ram, color=COLORS[3], alpha=0.8)
        ax2.set_xlabel('Processo', fontsize=11, fontweight='bold')
        ax2.set_ylabel('Bytes', fontsize=11, fontweight='bold')
        ax2.set_title('Uso Máximo de RAM por Processo', fontsize=13, fontweight='bold')
        ax2.set_xticks(range(len(names)))
        ax2.set_xticklabels(names, rotation=45)
        ax2.grid(axis='y', alpha=0.3)
        
        for bar in bars2:
            height = bar.get_height()
            if height > 0:
                ax2.text(bar.get_x() + bar.get_width()/2., height,
                        f'{int(height)}',
                        ha='center', va='bottom', fontsize=8)
        
        plt.tight_layout()
        plt.savefig(f'{self.output_dir}/memory_final_summary.png', 
                   dpi=300, bbox_inches='tight')
        plt.close()
        print("memory_final_summary.png")


def main():
    """Função principal"""
    print("=" * 70)
    print("VISUALIZADOR DE USO DE MEMÓRIA")
    print("=" * 70)
    
    # Procura diretório de resultados
    possible_dirs = ['output', 'build/output', '../build/output']
    
    results_dir = None
    for directory in possible_dirs:
        if os.path.exists(directory):
            # Verifica se tem arquivos de memória
            pattern = os.path.join(directory, 'memory_usage_*.txt')
            if glob.glob(pattern):
                results_dir = directory
                break
    
    if not results_dir:
        print("\nNenhum arquivo de memória encontrado!")
        print("\nExecute o simulador primeiro para gerar os arquivos de memória.")
        return 1
    
    print(f"\nDiretório de resultados: {results_dir}")
    
    visualizer = MemoryVisualizer()
    
    if not visualizer.load_memory_files(results_dir):
        print("Erro ao carregar arquivos de memória!")
        return 1
    
    print(f"{len(visualizer.memory_data)} processos carregados")
    print("\nGerando gráficos de memória...")
    
    visualizer.plot_memory_usage_over_time()
    visualizer.plot_cache_hit_rate_evolution()
    visualizer.plot_memory_heatmap()
    visualizer.plot_final_memory_summary()
    
    print("\n" + "=" * 70)
    print("Gráficos de memória gerados com sucesso!")
    print(f"Localização: plots/")
    print("=" * 70)
    
    return 0


if __name__ == '__main__':
    sys.exit(main())


# Script para comparar métricas entre diferentes políticas de escalonamento

import matplotlib.pyplot as plt
import numpy as np
import os
import sys
import re
from pathlib import Path

# Configurações de estilo
plt.style.use('seaborn-v0_8-darkgrid')
COLORS = ['#2E86AB', '#A23B72', '#F18F01', '#C73E1D', '#6A994E']

class SchedulerComparison:
    # Comparador de políticas de escalonamento
    
    def __init__(self, output_dir='plots'):
        self.output_dir = output_dir
        self.policies = {}
        os.makedirs(output_dir, exist_ok=True)
    
    def load_policy_results(self, policy_name, results_file):
        """Carrega resultados de uma política"""
        if not os.path.exists(results_file):
            print(f"Arquivo não encontrado: {results_file}")
            return False
        
        with open(results_file, 'r', encoding='utf-8') as f:
            content = f.read()
        
        metrics = {
            'policy_name': policy_name,
            'avg_wait_time': 0,
            'avg_turnaround_time': 0,
            'avg_response_time': 0,
            'total_pipeline_cycles': 0,
            'total_mem_accesses': 0,
            'avg_cache_hit_rate': 0,
            'process_count': 0
        }
        
        # Extrai métricas agregadas
        wait_times = re.findall(r'Tempo de Espera:\s+(\d+)', content)
        turnaround_times = re.findall(r'Tempo de Retorno:\s+(\d+)', content)
        response_times = re.findall(r'Tempo de Resposta:\s+(\d+)', content)
        pipeline_cycles = re.findall(r'Ciclos de Pipeline:\s+(\d+)', content)
        mem_accesses = re.findall(r'Total de Acessos a Mem:\s+(\d+)', content)
        cache_hits = re.findall(r'Cache Hits:\s+(\d+)', content)
        
        if wait_times:
            metrics['avg_wait_time'] = sum(map(int, wait_times)) / len(wait_times)
            metrics['process_count'] = len(wait_times)
        
        if turnaround_times:
            metrics['avg_turnaround_time'] = sum(map(int, turnaround_times)) / len(turnaround_times)
        
        if response_times:
            metrics['avg_response_time'] = sum(map(int, response_times)) / len(response_times)
        
        if pipeline_cycles:
            metrics['total_pipeline_cycles'] = sum(map(int, pipeline_cycles))
        
        if mem_accesses:
            metrics['total_mem_accesses'] = sum(map(int, mem_accesses))
            # Calcular taxa de cache hit média
            if cache_hits and len(cache_hits) == len(mem_accesses):
                hit_rates = []
                for i in range(len(mem_accesses)):
                    total = int(mem_accesses[i])
                    hits = int(cache_hits[i])
                    if total > 0:
                        hit_rates.append((hits / total) * 100)
                if hit_rates:
                    metrics['avg_cache_hit_rate'] = sum(hit_rates) / len(hit_rates)
        
        self.policies[policy_name] = metrics
        return True
    
    def plot_time_comparison(self):
        #Compara tempos entre políticas
        if not self.policies:
            return
        fig, ax = plt.subplots(figsize=(12, 6))
        policy_names = list(self.policies.keys())
        x = np.arange(len(policy_names))
        width = 0.25
        wait_times = [self.policies[p]['avg_wait_time'] for p in policy_names]
        turnaround_times = [self.policies[p]['avg_turnaround_time'] for p in policy_names]
        response_times = [self.policies[p]['avg_response_time'] for p in policy_names]
        bars1 = ax.bar(x - width, wait_times, width, label='Tempo de Espera', 
                      color=COLORS[0], alpha=0.8)
        bars2 = ax.bar(x, turnaround_times, width, label='Tempo de Retorno', 
                      color=COLORS[1], alpha=0.8)
        bars3 = ax.bar(x + width, response_times, width, label='Tempo de Resposta', 
                      color=COLORS[2], alpha=0.8)        
        ax.set_xlabel('Política de Escalonamento', fontsize=12, fontweight='bold')
        ax.set_ylabel('Tempo Médio (ms)', fontsize=12, fontweight='bold')
        ax.set_title('Comparação de Tempos Entre Políticas de Escalonamento', 
                    fontsize=14, fontweight='bold')
        ax.set_xticks(x)
        ax.set_xticklabels(policy_names)
        ax.legend()
        ax.grid(axis='y', alpha=0.3)
        # Adicionar valores nas barras
        for bars in [bars1, bars2, bars3]:
            for bar in bars:
                height = bar.get_height()
                if height > 0:
                    ax.text(bar.get_x() + bar.get_width()/2., height,
                           f'{int(height)}',
                           ha='center', va='bottom', fontsize=8)
        
        plt.tight_layout()
        plt.savefig(f'{self.output_dir}/scheduler_time_comparison.png', 
                   dpi=300, bbox_inches='tight')
        plt.close()
        print("scheduler_time_comparison.png")
    
    def plot_efficiency_comparison(self):
        """Compara eficiência entre políticas"""
        if not self.policies:
            return
        
        fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5))
        
        policy_names = list(self.policies.keys())
        pipeline_cycles = [self.policies[p]['total_pipeline_cycles'] for p in policy_names]
        cache_hit_rates = [self.policies[p]['avg_cache_hit_rate'] for p in policy_names]
        
        # Gráfico 1: Ciclos de Pipeline Totais
        bars1 = ax1.bar(range(len(policy_names)), pipeline_cycles, 
                       color=COLORS[3], alpha=0.8)
        ax1.set_xlabel('Política de Escalonamento', fontsize=11, fontweight='bold')
        ax1.set_ylabel('Ciclos Totais', fontsize=11, fontweight='bold')
        ax1.set_title('Ciclos de Pipeline Totais', fontsize=13, fontweight='bold')
        ax1.set_xticks(range(len(policy_names)))
        ax1.set_xticklabels(policy_names)
        ax1.grid(axis='y', alpha=0.3)
        
        for bar in bars1:
            height = bar.get_height()
            ax1.text(bar.get_x() + bar.get_width()/2., height,
                    f'{int(height)}',
                    ha='center', va='bottom', fontsize=9)
        
        # Gráfico 2: Taxa Média de Cache Hit
        bars2 = ax2.bar(range(len(policy_names)), cache_hit_rates, 
                       color=COLORS[4], alpha=0.8)
        ax2.set_xlabel('Política de Escalonamento', fontsize=11, fontweight='bold')
        ax2.set_ylabel('Taxa de Hit (%)', fontsize=11, fontweight='bold')
        ax2.set_title('Taxa Média de Cache Hit', fontsize=13, fontweight='bold')
        ax2.set_xticks(range(len(policy_names)))
        ax2.set_xticklabels(policy_names)
        ax2.set_ylim(0, max(cache_hit_rates) * 1.2 if cache_hit_rates else 100)
        ax2.grid(axis='y', alpha=0.3)
        
        for bar in bars2:
            height = bar.get_height()
            ax2.text(bar.get_x() + bar.get_width()/2., height,
                    f'{height:.1f}%',
                    ha='center', va='bottom', fontsize=9)
        
        plt.tight_layout()
        plt.savefig(f'{self.output_dir}/scheduler_efficiency_comparison.png', 
                   dpi=300, bbox_inches='tight')
        plt.close()
        print("scheduler_efficiency_comparison.png")
    
    def plot_summary_radar(self):
        """Gráfico radar comparativo (normalizado)"""
        if not self.policies or len(self.policies) < 2:
            return
        
        # Métricas para o radar (quanto menor, melhor para tempos)
        categories = ['Tempo\nEspera', 'Tempo\nRetorno', 'Tempo\nResposta', 
                     'Ciclos\nPipeline', 'Cache\nHit Rate']
        
        fig, ax = plt.subplots(figsize=(10, 10), subplot_kw=dict(projection='polar'))
        
        angles = np.linspace(0, 2 * np.pi, len(categories), endpoint=False).tolist()
        angles += angles[:1]  # Fechar o círculo
        
        policy_names = list(self.policies.keys())
        
        for i, policy_name in enumerate(policy_names):
            metrics = self.policies[policy_name]
            
            # Normalizar métricas (0-100, onde 100 é melhor)
            # Para tempos: inverter (menor é melhor)
            max_wait = max([p['avg_wait_time'] for p in self.policies.values()]) or 1
            max_turn = max([p['avg_turnaround_time'] for p in self.policies.values()]) or 1
            max_resp = max([p['avg_response_time'] for p in self.policies.values()]) or 1
            max_cycles = max([p['total_pipeline_cycles'] for p in self.policies.values()]) or 1
            
            values = [
                100 - (metrics['avg_wait_time'] / max_wait * 100),
                100 - (metrics['avg_turnaround_time'] / max_turn * 100),
                100 - (metrics['avg_response_time'] / max_resp * 100),
                100 - (metrics['total_pipeline_cycles'] / max_cycles * 100),
                metrics['avg_cache_hit_rate']  # Já está em %
            ]
            
            values += values[:1]  # Fechar o círculo
            
            ax.plot(angles, values, 'o-', linewidth=2, label=policy_name, 
                   color=COLORS[i % len(COLORS)])
            ax.fill(angles, values, alpha=0.15, color=COLORS[i % len(COLORS)])
        
        ax.set_xticks(angles[:-1])
        ax.set_xticklabels(categories, fontsize=10)
        ax.set_ylim(0, 100)
        ax.set_yticks([20, 40, 60, 80, 100])
        ax.set_yticklabels(['20', '40', '60', '80', '100'])
        ax.grid(True)
        
        plt.legend(loc='upper right', bbox_to_anchor=(1.3, 1.1))
        plt.title('Comparação Normalizada de Políticas de Escalonamento\n(100 = Melhor)', 
                 fontsize=14, fontweight='bold', pad=20)
        
        plt.tight_layout()
        plt.savefig(f'{self.output_dir}/scheduler_radar_comparison.png', 
                   dpi=300, bbox_inches='tight')
        plt.close()
        print("scheduler_radar_comparison.png")


def main():
    """Função principal"""
    print("=" * 70)
    print("COMPARADOR DE POLÍTICAS DE ESCALONAMENTO")
    print("=" * 70)
    
    # Procura arquivos de resultados
    base_paths = ['output', 'build/output', '../build/output']
    policies_files = {
        'FCFS': 'resultados_FCFS.dat',
        'SJN': 'resultados_SJN.dat',
        'Priority': 'resultados_Priority.dat',
        'Round Robin': 'resultados_RoundRobin.dat'
    }
    
    comparator = SchedulerComparison()
    found_files = 0
    
    print("\n🔍 Procurando arquivos de resultados...")
    
    for policy_name, filename in policies_files.items():
        for base_path in base_paths:
            full_path = os.path.join(base_path, filename)
            if os.path.exists(full_path):
                print(f"{policy_name}: {full_path}")
                comparator.load_policy_results(policy_name, full_path)
                found_files += 1
                break
    
    if found_files == 0:
        print("\nNenhum arquivo de resultados encontrado!")
        print("\nExecute o simulador com opção 5 (Executar TODOS e Comparar)")
        print("para gerar os arquivos de comparação.")
        return 1
    
    if found_files < 2:
        print(f"\nApenas {found_files} política encontrada.")
        print("São necessárias pelo menos 2 políticas para comparação.")
        return 1
    
    print(f"\n{found_files} políticas carregadas")
    print("\nGerando gráficos comparativos...")
    
    comparator.plot_time_comparison()
    comparator.plot_efficiency_comparison()
    comparator.plot_summary_radar()
    
    print("\n" + "=" * 70)
    print("Gráficos de comparação gerados com sucesso!")
    print(f"Localização: plots/")
    print("=" * 70)
    
    return 0


if __name__ == '__main__':
    sys.exit(main())

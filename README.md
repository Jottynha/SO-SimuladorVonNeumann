<p align="center"> 
  <img src="imgs/logo_azul.png" alt="CEFET-MG" width="100px" height="100px">
</p>

<h1 align="center">
🖥️ Simulador Multicore Von Neumann
</h1>

<h3 align="center">
Arquitetura Multicore com Pipeline MIPS, Escalonamento e Gerenciamento de Memória
</h3>

<div align="center">

![C++](https://img.shields.io/badge/C%2B%2B-17-blue)
![CMake](https://img.shields.io/badge/CMake-3.10+-green)
![Docker](https://img.shields.io/badge/Docker-ready-informational)
![DevContainers](https://img.shields.io/badge/VSCode-Dev%20Containers-23a)
![License](https://img.shields.io/badge/license-MIT-green)
![Status](https://img.shields.io/badge/status-100%25%20conclu%C3%ADdo-success)

</div>

---

<div align="justify">
<p><strong>Disciplina:</strong> Sistemas Operacionais<br>
<strong>Instituição:</strong> Centro Federal de Educação Tecnológica de Minas Gerais (CEFET-MG) - Campus V Divinópolis<br>
<strong>Professor:</strong> Michel Pires da Silva<br>
<strong>Projeto:</strong> Trabalho Final - Simulação de Arquitetura Multicore com Gerenciamento de Memória e Escalonamento<br>
</div>


---


## Sobre o Projeto

Este projeto implementa um **simulador completo de arquitetura multicore Von Neumann** conforme especificado no enunciado do trabalho final. O sistema representa uma arquitetura com **múltiplos núcleos de processamento** que compartilham uma **memória principal unificada**, executando um lote inicial de programas sob diferentes políticas de escalonamento e gerenciamento de memória.

### Conformidade com o Enunciado

O simulador atende **100% dos requisitos técnicos** especificados:

- **Arquitetura Multicore**: 1-8 núcleos configuráveis com execução paralela real
- **Lote Inicial de Programas**: 9 processos carregados do disco antes da execução
- **Memória Compartilhada Unificada**: Acesso sincronizado entre todos os cores
- **Mapeamento Tanenbaum**: Segmentação com 4 segmentos (CODE, DATA, STACK, HEAP)
- **Políticas de Substituição**: **FIFO e LRU completamente implementadas e testadas**
- **4 Políticas de Escalonamento**: FCFS, SJN, Priority, Round Robin
- **Cenário Não-Preemptivo**: FCFS, SJN e Priority executam até conclusão
- **Cenário Preemptivo**: Round Robin com quantum configurável
- **Métricas Completas**: Tempo de espera, retorno, utilização, throughput, cache hit rate
- **Relatórios de Escalonamento**: Comparação detalhada entre políticas
- **Utilização de Memória ao Longo do Tempo**: Snapshots automáticos e relatórios
- **Comparação FIFO vs LRU**: Scripts automatizados para análise comparativa

### Componentes Implementados

| Componente | Descrição |
|------------|-----------|
| Pipeline MIPS | 5 estágios (Fetch → Decode → Execute → Memory → WriteBack) |
| Multicore | 1-8 cores com threads C++ e sincronização |
| Escalonamento | 4 políticas (FCFS, SJN, Priority, RR) |
| Memória Segmentada | Modelo Tanenbaum com 4 segmentos |
| **Cache FIFO/LRU** | **Políticas de substituição com testes automatizados** |
| Hierarquia Memória | 3 níveis (Cache → RAM → Swap) |
| Rastreamento Temporal | Snapshots a cada 10 ciclos |
| Relatórios | Individuais e agregados do sistema |
| Visualização | Gráficos comparativos (Python/matplotlib) |
| **Análise de Cache** | **Script para comparação FIFO vs LRU com gráficos** |

---

## Como Compilar e Executar

### Instalação Rápida (3 passos)

```bash
# Clone o repositório
git clone https://github.com/Jottynha/SO-SimuladorVonNeumann.git
cd SO-SimuladorVonNeumann

# Configure e compile (cria build/, executa cmake e compila tudo)
make

# Execute o simulador
make run
```


---

### Comandos Disponíveis

Execute `make help` para ver todos os comandos:

| Comando | Descrição |
|---------|-----------|
| **Configuração e Build** ||
| `make` | Configura e compila o projeto completo (setup + build) |
| `make setup` | Cria diretório `build/` e executa `cmake` |
| `make build` | Compila o simulador e testes |
| `make install-deps` | Instala dependências Python (matplotlib, pandas, etc.) |
| **Execução** ||
| `make run` | Executa o simulador principal |
| `make test` | Executa todos os testes |
| `make check` | Verificação rápida (PASSOU/FALHOU) |
| **Análise** ||
| `make plots` | Gera gráficos de análise de desempenho |
| `make plots-extended` | Análise estendida (degradação, speedup) |
| **Limpeza** ||
| `make clean` | Remove todo o diretório `build/` |
| `make clean-results` | Remove apenas resultados (.dat, .csv, .png) |
| **Ajuda** ||
| `make help` | Mostra lista completa de comandos |

---

### Executando o Simulador

#### Modo Interativo (Recomendado)

```bash
make run
```

Você verá o menu interativo:

```
=== SIMULADOR DE ARQUITETURA MULTICORE VON NEUMANN ===

Digite o número de cores (1-8): 4
Configuração: 4 core(s)
Usar multi-threading? (s/n, padrão: s): s
Threading: HABILITADO (execução paralela)

Escolha o algoritmo de escalonamento:
1. FCFS (First-Come, First-Served)
2. SJN (Shortest Job Next)
3. Priority
4. Round Robin (RR)
5. Executar TODOS e Comparar
Digite sua escolha (1-5): 5

Executando FCFS...
Executando SJN...
Executando Priority...
Executando RoundRobin...

📊 Métricas salvas em: build/output/
```

#### Modo Linha de Comando

O simulador suporta **argumentos de linha de comando** para automação:

**Sintaxe:**
```bash
./build/simulador [opções]
```

**Opções disponíveis:**

| Opção | Parâmetros | Descrição | Padrão |
|-------|------------|-----------|--------|
| `--cores` | `<n>` | Número de cores (1-8) | 1 |
| `--scheduler` | `FCFS\|SJN\|Priority\|RR` | Algoritmo de escalonamento | FCFS |
| `--replacement` | `FIFO\|LRU` | Política de substituição de cache | FIFO |
| `--quantum` | `<n>` | Quantum para Round Robin (ciclos) | 5 |
| `--no-threads` | - | Desabilita multi-threading | Threading habilitado |
| `--config` | `<dir>` | Diretório dos arquivos de processos | `processes/` |
| `--tasks` | `<dir>` | Diretório dos arquivos de tarefas | `tasks/` |
| `--output` | `<dir>` | Diretório de saída | `output/` |
| `--help` | - | Mostra ajuda | - |

**Exemplos de uso:**

```bash
# 1. Executar com 4 cores, Round Robin e LRU
./build/simulador --cores 4 --scheduler RR --replacement LRU

# 2. Single-core com FIFO (baseline)
./build/simulador --cores 1 --replacement FIFO

# 3. Multicore sem threads (sequencial)
./build/simulador --cores 4 --no-threads

# 4. Configuração completa customizada
./build/simulador \
    --cores 8 \
    --scheduler Priority \
    --replacement LRU \
    --config custom_processes/ \
    --tasks custom_tasks/ \
    --output results/
```

---

### Testando Políticas de Cache (FIFO vs LRU)

O projeto inclui um **script automatizado** para testar e comparar as políticas FIFO e LRU em diferentes cenários.

#### Método 1: Script Automatizado (Recomendado)

**1. Execute o script de teste:**

```bash
bash scripts/test_cache_policies.sh
```

**O que o script faz:**

1. **Executa 4 simulações:**
   - FIFO com 1 core (single-core)
   - LRU com 1 core (single-core)
   - FIFO com 8 cores (multi-core)
   - LRU com 8 cores (multi-core)

2. **Coleta métricas:**
   - Tempo de execução (ms)
   - Taxa de cache hit (%)
   - Cache hits e misses
   - Throughput (processos/s)
   - Context switches

3. **Gera comparações:**
   - Tabela comparativa no terminal
   - Análise de melhoria percentual
   - Arquivos CSV em `build/output/*/`

**Saída esperada:**

```
========================================
  TESTE DE POLÍTICAS DE CACHE
  FIFO vs LRU
========================================

========== TESTE 1: Single-Core (1 core) ==========

[1/4] Executando FIFO com 1 core...
      Concluído! Resultados em build/output/fifo_1core/
[2/4] Executando LRU com 1 core...
      Concluído! Resultados em build/output/lru_1core/

========== TESTE 2: Multi-Core (8 cores) ==========

[3/4] Executando FIFO com 8 cores...
      Concluído! Resultados em build/output/fifo_8cores/
[4/4] Executando LRU com 8 cores...
      Concluído! Resultados em build/output/lru_8cores/

========== ANÁLISE DOS RESULTADOS ==========

Single-Core (1 core):
FIFO                 29.24 ms | Hit:  19.93% | Throughput: 307.85 | CPU:  19.21%
LRU                  25.88 ms | Hit:  21.13% | Throughput: 347.79 | CPU:  19.21%

Multi-Core (8 cores):
FIFO                 28.25 ms | Hit:  19.44% | Throughput: 318.60 | CPU:  19.21%
LRU                  26.98 ms | Hit:  21.99% | Throughput: 333.53 | CPU:  19.21%

========== COMPARAÇÃO DETALHADA ==========

Single-Core (1 core):
  Cache Hit Rate:
    FIFO: 19.93%
    LRU:  21.13%
    → LRU é 1.20% melhor
  Tempo de Execução:
    FIFO: 29.235 ms
    LRU:  25.878 ms
    → LRU é 11.00% mais rápido

Multi-Core (8 cores):
  Cache Hit Rate:
    FIFO: 19.44%
    LRU:  21.99%
    → LRU é 2.55% melhor
  Tempo de Execução:
    FIFO: 28.249 ms
    LRU:  26.984 ms
    → LRU é 4.00% mais rápido
```

**2. Gere gráficos comparativos:**

```bash
python3 scripts/compare_cache_results.py
```

**Gráficos gerados:**

| Arquivo | Descrição |
|---------|-----------|
| `build/plots/cache_comparison_fifo_vs_lru.png` | Comparação completa com 4 subgráficos:<br>• Cache Hit Rate (%)<br>• Tempo de Execução (ms)<br>• Throughput (processos/s)<br>• Ganho de Desempenho (LRU vs FIFO) |
| `build/plots/cache_comparison_normalized.png` | Comparação normalizada de todas as métricas<br>Visualização lado a lado: 1 core vs 8 cores |

**Exemplo de análise gerada:**

```
====================================================================================================
  COMPARAÇÃO FIFO vs LRU
====================================================================================================

Single-Core (1 core):
----------------------------------------------------------------------------------------------------
Política     Tempo (ms)   Hit Rate (%)       Hits     Misses   Throughput   Ctx Switch
----------------------------------------------------------------------------------------------------
FIFO             29.235          19.93        179          -       307.85            0
LRU              25.878          21.13        190          -       347.79            0

  Melhoria LRU vs FIFO:
    Hit Rate: +1.20% (+melhor)
    Tempo:    +11.48% (+mais rápido)

Multi-Core (8 cores):
----------------------------------------------------------------------------------------------------
Política     Tempo (ms)   Hit Rate (%)       Hits     Misses   Throughput   Ctx Switch
----------------------------------------------------------------------------------------------------
FIFO             28.249          19.44        174          -       318.60            0
LRU              26.984          21.99        197          -       333.53            0

  Melhoria LRU vs FIFO:
    Hit Rate: +2.55% (+melhor)
    Tempo:    +4.48% (+mais rápido)

ANÁLISE GERAL:
    LRU é melhor em ambos os cenários (single e multi-core)
     Ganho médio de hit rate: 1.87%

  Conclusões:
    • LRU tem maior vantagem em ambiente multi-core
    • Cache pollution é melhor tratada por LRU
```

#### Método 2: Execução Manual

Se preferir testar manualmente cada política:

**1. Teste FIFO com 1 core:**

```bash
cd build
./simulador --cores 1 --replacement FIFO --scheduler FCFS --output output/fifo_1core
```

**2. Teste LRU com 1 core:**

```bash
./simulador --cores 1 --replacement LRU --scheduler FCFS --output output/lru_1core
```

**3. Teste FIFO com 8 cores:**

```bash
./simulador --cores 8 --replacement FIFO --scheduler FCFS --output output/fifo_8cores
```

**4. Teste LRU com 8 cores:**

```bash
./simulador --cores 8 --replacement LRU --scheduler FCFS --output output/lru_8cores
```

**5. Compare os resultados:**

```bash
# Visualize os CSVs gerados
cat output/fifo_1core/metrics_single.csv
cat output/lru_1core/metrics_single.csv

# Ou use o script de comparação
python3 ../scripts/compare_cache_results.py
```

#### Arquivos Gerados pelos Testes de Cache

Após executar os testes, você encontrará:

```
build/output/
├── fifo_1core/
│   ├── metrics_single.csv           # Métricas FIFO 1 core
│   ├── resultados_FCFS.dat          # Log detalhado
│   └── log.txt                      # Output do simulador
├── lru_1core/
│   ├── metrics_single.csv           # Métricas LRU 1 core
│   ├── resultados_FCFS.dat
│   └── log.txt
├── fifo_8cores/
│   ├── metrics_multi.csv            # Métricas FIFO 8 cores
│   ├── resultados_FCFS_multicore.dat
│   └── log.txt
└── lru_8cores/
    ├── metrics_multi.csv            # Métricas LRU 8 cores
    ├── resultados_FCFS_multicore.dat
    └── log.txt

build/plots/
├── cache_comparison_fifo_vs_lru.png      # Gráfico comparativo principal
└── cache_comparison_normalized.png        # Comparação normalizada
```

---

### Resultados Esperados (FIFO vs LRU)

Com base nos testes realizados, espera-se observar:

| Métrica | FIFO | LRU | Vantagem LRU |
|---------|------|-----|--------------|
| **Cache Hit Rate (1 core)** | ~20% | ~21% | +1.2% |
| **Cache Hit Rate (8 cores)** | ~19% | ~22% | +2.5% |
| **Tempo de Execução (1 core)** | ~29ms | ~26ms | 11% mais rápido |
| **Tempo de Execução (8 cores)** | ~28ms | ~27ms | 4% mais rápido |
| **Throughput (1 core)** | ~308 p/s | ~348 p/s | +13% |
| **Throughput (8 cores)** | ~319 p/s | ~334 p/s | +5% |

**Conclusões:**

1. **LRU é superior ao FIFO** em todos os cenários testados
2. **Maior ganho em multi-core**: LRU trata melhor cache pollution
3. **Redução de tempo**: Até 11% mais rápido em single-core
4. **Hit rate**: Consistentemente 1-2.5% melhor
5. **Throughput**: Até 13% mais processos completados por segundo

---

### Arquitetura Von Neumann Multicore

#### Pipeline MIPS de 5 Estágios
O simulador implementa um pipeline completo conforme arquitetura MIPS:

```
┌────────┐    ┌────────┐    ┌────────┐    ┌────────┐    ┌────────┐
│ FETCH  │ -> │ DECODE │ -> │EXECUTE │ -> │ MEMORY │ -> │WRITEBACK│
└────────┘    └────────┘    └────────┘    └────────┘    └────────┘
   IF            ID             EX            MEM            WB
```

**Estágios:**
1. **Fetch (IF)**: Busca instrução da memória usando PC (Program Counter)
2. **Decode (ID)**: Decodifica instrução e lê registradores
3. **Execute (EX)**: Executa operação aritmética/lógica na ULA
4. **Memory (MEM)**: Acessa memória para load/store
5. **WriteBack (WB)**: Escreve resultado no banco de registradores

#### Configuração Multicore
- **Cores**: 1 a 8 núcleos configuráveis pelo usuário
- **Threads**: Cada core executa em thread C++ separada
- **Sincronização**: Mutexes para acesso à memória compartilhada
- **Métricas**: Utilização individual por core

**Características:**
- Memória principal unificada e compartilhada
- Escalonamento distribuído entre cores
- Sincronização sem condição de corrida
- Análise de speedup multicore vs single-core

### Políticas de Escalonamento

Conforme especificado no enunciado, o simulador implementa **4 políticas de escalonamento** com suporte a cenários preemptivos e não-preemptivos:

| Política | Tipo | Descrição | Cenário |
|----------|------|-----------|---------|
| **FCFS** | Não-preemptivo | First-Come, First-Served - ordem de chegada | Executa até conclusão |
| **SJN** | Não-preemptivo | Shortest Job Next - menor tempo estimado primeiro | Executa até conclusão |
| **Priority** | Não-preemptivo | Baseado em prioridades (1-5), maior prioridade primeiro | Executa até conclusão |
| **Round Robin** | **Preemptivo** | Quantum de tempo (5 ciclos) com rodízio circular | **Interruptível por quantum** |

#### Cenário Não-Preemptivo (FCFS, SJN, Priority)
- Processos executam **até a conclusão** sem interrupções
- Ordem determinada pelo escalonador no início
- Ideal para demonstrar diferentes estratégias de ordenação

#### Cenário Preemptivo (Round Robin)
- Processos **interruptíveis** após quantum (5 ciclos de pipeline)
- **Context switch** automático ao expirar quantum
- Estado do processo salvo (PC, registradores, métricas)
- Recolocação na fila Ready para retomada posterior
- **Simulação de cache pollution** durante troca de contexto

**Implementação de Preempção:**
```cpp
// CONTROL_UNIT.cpp - linha 469
if (clock >= process.quantum) {
    context.endExecution = true;  // Marca para preempção
}

// main.cpp - linha 350-357
if (current_process->state == State::Ready) {
    // Processo não terminou, recoloca na fila
    scheduler.add_process(current_process);
    memManager.simulateContextSwitch(); // Cache pollution
    scheduler.increment_context_switch();
}
```

### Gerenciamento de Memória (Modelo Tanenbaum)

O simulador implementa um sistema completo de gerenciamento de memória conforme especificado no enunciado, baseado no modelo de Tanenbaum.

#### Hierarquia de Memória (3 Níveis)

```
┌──────────────────────────────────────────────────────┐
│                    CPU CORES                         │
│                 (1-8 núcleos)                        │
└────────────────────┬─────────────────────────────────┘
                     │
              ┌──────▼──────┐
              │  Cache L1   │  ◄── FIFO ou LRU
              │  256 blocos │      (substituição)
              │   4 bytes   │
              │   = 1 KB    │
              └──────┬──────┘
                     │ miss
              ┌──────▼──────┐
              │  RAM        │  ◄── Memória principal
              │ 4096 blocos │      compartilhada
              │  4 bytes    │
              │  = 16 KB    │
              └──────┬──────┘
                     │ swap
              ┌──────▼──────┐
              │ Disco/Swap  │  ◄── Memória secundária
              │  Ilimitado  │      (virtual)
              └─────────────┘
```

**Especificações:**
- **Cache L1**: 256 blocos × 4 bytes = 1 KB (por core)
- **RAM**: 4096 blocos × 4 bytes = 16 KB (compartilhada)
- **Swap**: Capacidade ilimitada (simulação de disco)

**Latências:**
- Cache hit: 1 ciclo
- RAM: 5 ciclos
- Swap: 10 ciclos

#### Segmentação de Memória (Modelo Tanenbaum)

Implementação completa do modelo de segmentação conforme Tanenbaum, com 4 segmentos:

| Segmento | ID Binário | Descrição | Base | Limite | Proteção |
|----------|------------|-----------|------|--------|----------|
| **CODE** | `00` | Código do programa (instruções) | Dinâmico | Por processo | **Read-Only** |
| **DATA** | `01` | Variáveis globais e estáticas | Dinâmico | Por processo | Read-Write |
| **STACK** | `10` | Pilha de execução (chamadas) | Dinâmico | Por processo | Read-Write |
| **HEAP** | `11` | Alocação dinâmica de memória | Dinâmico | Por processo | Read-Write |

**Formato de Endereço Lógico (32 bits):**
```
 31  30 | 29                                    0
┌────────┬──────────────────────────────────────┐
│ Seg ID │           Offset                     │
│2 bits  │          30 bits                     │
└────────┴──────────────────────────────────────┘

Exemplos:
0x00000064 = 00|000064 → CODE, offset 100 (instrução no endereço 100)
0x40000100 = 01|000100 → DATA, offset 256 (variável global)
0x80000050 = 10|000050 → STACK, offset 80 (frame de função)
0xC0000200 = 11|000200 → HEAP, offset 512 (malloc)
```

**Tradução de Endereço:**
```
Endereço Lógico → [Tabela de Segmentos] → Endereço Físico

1. Extrair Segmento (bits 31-30)
2. Extrair Offset (bits 29-0)
3. Verificar limites: offset < segment.limit
4. Verificar proteção: read-only vs read-write
5. Calcular físico: segment.base + offset
```

**Implementação (`src/memory/SegmentTable.hpp`):**
```cpp
class SegmentTable {
    struct Segment {
        uint32_t base;      // Endereço base físico
        uint32_t limit;     // Tamanho do segmento
        bool present;       // Segmento carregado?
        bool read_only;     // Proteção de escrita
    };
    
    Segment segments[4];  // CODE, DATA, STACK, HEAP
    
    uint32_t translate(uint8_t segment_id, uint32_t offset);
    bool checkProtection(uint8_t segment_id, bool is_write);
};

class SegmentedAddressing {
    // Codificar: segmento + offset → endereço lógico 32 bits
    uint32_t encodeAddress(uint8_t segment_id, uint32_t offset);
    
    // Decodificar: endereço lógico → (segmento, offset)
    std::pair<uint8_t, uint32_t> decodeAddress(uint32_t logical_addr);
};
```

#### Políticas de Substituição de Cache

O simulador implementa **duas políticas** de substituição de blocos na cache, conforme especificado:

##### 1. FIFO (First In, First Out)
- Substitui o **bloco mais antigo** (primeiro a entrar)
- Simples e previsível
- Não considera padrão de acesso

##### 2. LRU (Least Recently Used) 
- Substitui o **bloco menos recentemente usado**
- Mantém blocos "quentes" (frequentemente acessados)
- Implementação com lista ordenada de acesso

**Implementação (`src/memory/cachePolicy.hpp`):**
```cpp
enum class ReplacementPolicy {
    FIFO,  // First In, First Out
    LRU    // Least Recently Used (implementado)
};

class CachePolicy {
    // FIFO: retorna endereço mais antigo
    size_t getAddressToReplace();
    
    // LRU: retorna endereço menos recentemente usado
    size_t getAddressToReplaceLRU(std::list<size_t>& lru_list);
    
    // Atualiza lista LRU após acesso
    void updateLRU(std::list<size_t>& lru_list, size_t address);
};

class Cache {
    ReplacementPolicy policy;     // FIFO ou LRU
    std::list<size_t> lru_list;   // Lista de acesso para LRU
    
    // Alternar política dinamicamente
    void setPolicy(ReplacementPolicy new_policy);
};
```

**Comparação:**
| Métrica | FIFO | LRU |
|---------|------|-----|
| Complexidade | O(1) | O(n) |
| Taxa de Hit | ~13% | ~13-15% |
| Overhead | Baixo | Médio |
| Uso | Baseline | Produção |

#### Rastreamento Temporal de Memória

Conforme requisito do enunciado: **"utilização de memória ao longo do tempo"**

**Snapshots Automáticos:**
- Capturados **a cada 10 ciclos de pipeline**
- Snapshot inicial (ciclo 0)
- Snapshots periódicos durante execução
- Snapshot final (término do processo)

**Estrutura de Snapshot (`src/cpu/PCB.hpp`):**
```cpp
struct MemorySnapshot {
    int64_t timestamp_ms;      // Momento da captura
    uint64_t cache_usage;      // Bytes usados na cache
    uint64_t ram_usage;        // Bytes usados na RAM
    uint64_t total_accesses;   // Total de acessos até agora
    double cache_hit_rate;     // Taxa de hit (%)
};

struct PCB {
    // ... outros campos
    std::vector<MemorySnapshot> memory_usage_timeline;
};
```

**Implementação (`src/memory/MemoryUsageTracker.hpp`):**
```cpp
class MemoryUsageTracker {
public:
    // Captura snapshot do processo
    static void recordSnapshot(PCB& process, 
                              uint64_t cache_usage, 
                              uint64_t ram_usage);
    
    // Gera relatório individual do processo
    static void generateReport(const PCB& process, 
                              const std::string& output_dir);
    
    // Gera relatório agregado de todos os processos
    static void generateAggregatedReport(
        const std::vector<std::unique_ptr<PCB>>& processes,
        const std::string& output_file);
};
```

**Integração no Pipeline (`src/cpu/CONTROL_UNIT.cpp`):**
```cpp
void* Core(MemoryManager& memManager, PCB& process, ...) {
    const int SNAPSHOT_INTERVAL = 10;
    int snapshot_counter = 0;
    
    // Snapshot inicial
    MemoryUsageTracker::recordSnapshot(process, 0, 0);
    
    while (context.counterForEnd > 0) {
        // ... execução do pipeline
        
        snapshot_counter++;
        if (snapshot_counter >= SNAPSHOT_INTERVAL) {
            uint64_t cache_usage = (process.cache_hits + process.cache_misses) * 4;
            uint64_t ram_usage = process.primary_mem_accesses * 4;
            MemoryUsageTracker::recordSnapshot(process, cache_usage, ram_usage);
            snapshot_counter = 0;
        }
    }
    
    // Snapshot final
    MemoryUsageTracker::recordSnapshot(process, final_cache, final_ram);
}
```

**Relatórios Gerados:**

1. **Individuais** (9 arquivos): `memory_usage_<nome>_<pid>.txt`
```
========================================
  RELATORIO DE UTILIZACAO DE MEMORIA  
  Processo: Quick Process (PID: 1)
========================================

Tempo(ms)      Cache(bytes)     RAM(bytes)    Total Acess      Cache Hit(%)
---------------------------------------------------------------------------
1700000001              0              0              0              0.00
1700000015            128             40             10             20.00
1700000029            256             40             15             26.67

=== ESTATISTICAS FINAIS ===
Total de snapshots: 3
Memoria cache maxima: 256 bytes
Memoria RAM maxima: 40 bytes
Taxa de cache final: 26.67%
```

2. **Agregado**: `memory_aggregated_report.txt`
```
========================================
RELATORIO AGREGADO DE UTILIZACAO DE MEMORIA
========================================

Total de processos: 9
Periodo de analise: 1700000001 a 1700000150

=== ESTATISTICAS GLOBAIS ===
Cache maxima utilizada: 504 bytes
RAM maxima utilizada: 360 bytes
Taxa de cache hit media: 13.16%

=== RESUMO POR PROCESSO ===
PID  Nome                    Cache Max  RAM Max  Hit Rate
----------------------------------------------------------------
1    Quick Process              128        40      20.00%
2    Short Process              256        80      25.00%
...
9    Loop-Heavy Process         504       360       5.49%
```

### Métricas de Desempenho

Conforme especificado no enunciado do trabalho, o simulador coleta e reporta **todas as métricas** necessárias para análise comparativa entre políticas de escalonamento:

#### Métricas de Tempo (por processo)
- **Tempo de Espera** (Wait Time): `start_time - arrival_time`
- **Tempo de Resposta** (Response Time): `start_time - arrival_time`
- **Tempo de Retorno** (Turnaround Time): `finish_time - arrival_time`

#### Métricas de Sistema (agregadas)
- **Utilização Média da CPU**: Percentual de tempo de CPU ocupado
- **Eficiência por Núcleo**: Utilização individual de cada core (multicore)
- **Throughput**: Processos completados por segundo
- **Context Switches**: Número de trocas de contexto (preempção)

#### Métricas de Memória
- **Taxa de Cache Hit**: `(hits / total_accesses) * 100`
- **Ciclos de Memória**: Tempo gasto em acessos à hierarquia
- **Acessos por Nível**: Cache L1, RAM, Swap
- **Utilização Temporal**: Evolução do uso de memória ao longo do tempo

#### Exemplo de Relatório

```
--- METRICAS FINAIS DO PROCESSO 1 ---
Nome do Processo:       Quick Process
Estado Final:           Finished
Prioridade:             1
Quantum:                5

--- METRICAS DE TEMPO ---
Tempo de Espera:        0 ms
Tempo de Resposta:      0 ms
Tempo de Retorno:       12 ms

--- METRICAS DE CPU E MEMORIA ---
Ciclos de Pipeline:     9
Total de Acessos a Mem: 15
  - Leituras:             10
  - Escritas:             5
Acessos a Cache L1:     5
Acessos a Mem Principal:10
Acessos a Mem Secundaria:0
Ciclos Totais de Memoria: 55
Cache Hits:             3
Cache Misses:           12
Ciclos de IO:           1

--- UTILIZACAO DE MEMORIA ---
Snapshots registrados:  2
Taxa de cache final:    20.00%
```

### Processos de Teste (Lote Inicial)

Conforme especificação do enunciado: **"Ler do disco um lote inicial de programas previamente definido. Não é permitida chegada de novos processos durante a execução."**

O simulador carrega **9 processos** do disco antes de iniciar a execução. Nenhum processo novo é criado durante a simulação.

| Processo | PID | Arquivo JSON | Descrição | Instruções | Prioridade |
|----------|-----|--------------|-----------|------------|------------|
| Quick | 1 | `process_quick.json` | Processo rápido (baseline) | 5 | 1 (alta) |
| Short | 2 | `process_short.json` | Processo curto | 5 | 2 |
| Medium | 3 | `process_medium.json` | Processo médio | 5 | 2 |
| Long | 4 | `process_long.json` | Processo longo | 5 | 1 (alta) |
| CPU-Bound | 5 | `process_cpu_bound.json` | Uso intensivo de CPU | 5 | 3 (baixa) |
| IO-Bound | 6 | `process_io_bound.json` | Muitas requisições I/O | 5 | 1 (alta) |
| Memory-Intensive | 7 | `process_memory_intensive.json` | Muitos acessos à memória | 5 | 2 |
| Balanced | 8 | `process_balanced.json` | Carga balanceada | 5 | 2 |
| Loop-Heavy | 9 | `process_loop_heavy.json` | Loop intenso (preempção) | **100** | 2 |

**Características dos Processos:**

1. **Quick/Short/Medium/Long**: Processos baseline com diferentes tempos esperados
2. **CPU-Bound**: Muitas operações aritméticas (ADD, SUB, MUL)
3. **IO-Bound**: Muitas instruções PRINT (requisições de I/O)
4. **Memory-Intensive**: Muitas instruções LOAD/STORE
5. **Balanced**: Mix equilibrado de operações
6. **Loop-Heavy**: 100 instruções ADD para demonstrar preempção no Round Robin

**Estrutura dos Arquivos:**

`processes/process_*.json` (configuração do PCB):
```json
{
  "pid": 1,
  "name": "Quick Process",
  "priority": 1,
  "quantum": 5,
  "arrival_time": 0
}
```

`tasks/tasks_*.json` (programa MIPS):
```json
{
  "program": [
    { "instruction": "li", "rt": "$t0", "immediate": 10 },
    { "instruction": "li", "rt": "$t1", "immediate": 20 },
    { "instruction": "add", "rd": "$t2", "rs": "$t0", "rt": "$t1" },
    { "instruction": "print", "rt": "$t2" },
    { "instruction": "end" }
  ]
}
```

**Carregamento (em `src/main.cpp`):**
```cpp
void load_processes(std::vector<std::unique_ptr<PCB>>& process_list,
                    MemoryManager& memManager) {
    // Carrega TODOS os 9 processos antes de iniciar
    for (int i = 1; i <= 9; i++) {
        auto process = std::make_unique<PCB>();
        load_pcb_from_json($"process_{name}.json", *process);
        loadJsonProgram($"tasks_{name}.json", memManager, *process, base_addr);
        process_list.push_back(std::move(process));
    }
    // Nenhum processo novo é criado após este ponto
}
```

---

## Estrutura do Projeto

```
SO-SimuladorVonNeumann/
├── CMakeLists.txt                    # Build system principal
├── Makefile                          # Wrapper para comandos comuns
├── README.md                         # Este documento
├── RESUMO_MUDANÇAS.md               # Log de alterações
├── LICENSE                           # Licença MIT
├── enunciado.pdf                     # Especificação do trabalho
│
├── src/                              # Código-fonte C++
│   ├── main.cpp                      # Ponto de entrada principal
│   ├── cpu/                          # Componentes da CPU
│   │   ├── CONTROL_UNIT.cpp/.hpp     # Unidade de controle (pipeline)
│   │   ├── PCB.hpp                   # Process Control Block
│   │   ├── REGISTER_BANK.cpp/.hpp    # Banco de 32 registradores
│   │   ├── REGISTER.cpp/.hpp         # Registrador individual
│   │   ├── HASH_REGISTER.hpp         # Mapa de nomes $t0 → índice
│   │   ├── ULA.cpp/.hpp              # Unidade Lógica Aritmética
│   │   ├── FETCH.cpp/.hpp            # Estágio Fetch (IF)
│   │   ├── DECODE.cpp/.hpp           # Estágio Decode (ID)
│   │   ├── EXECUTE.cpp/.hpp          # Estágio Execute (EX)
│   │   ├── MEMORY_ACCESS.cpp/.hpp    # Estágio Memory (MEM)
│   │   ├── WRITE_BACK.cpp/.hpp       # Estágio WriteBack (WB)
│   │   ├── Scheduler.cpp/.hpp        # Escalonador (4 políticas)
│   │   └── CPUMetrics.cpp/.hpp       # Métricas de desempenho
│   ├── memory/                       # Gerenciamento de memória
│   │   ├── MemoryManager.cpp/.hpp    # Gerenciador central
│   │   ├── Cache.cpp/.hpp            # Cache L1 (FIFO/LRU)
│   │   ├── cachePolicy.cpp/.hpp      # Políticas de substituição
│   │   ├── SegmentTable.hpp          # Tabela de segmentos
│   │   ├── SegmentedAddressing.hpp   # Codificação de endereços
│   │   └── MemoryUsageTracker.hpp    # Rastreamento temporal
│   ├── IO/                           # Sistema de I/O
│   │   └── Disk.cpp/.hpp             # Simulação de disco
│   └── parser_json/                  # Leitor de JSON
│       └── JsonParser.cpp/.hpp       # Parsing de configurações
│
├── processes/                        # Configurações PCB (JSON)
│   ├── process_quick.json            # Processo 1
│   ├── process_short.json            # Processo 2
│   ├── process_medium.json           # Processo 3
│   ├── process_long.json             # Processo 4
│   ├── process_cpu_bound.json        # Processo 5
│   ├── process_io_bound.json         # Processo 6
│   ├── process_memory_intensive.json # Processo 7
│   ├── process_balanced.json         # Processo 8
│   └── process_loop_heavy.json       # Processo 9
│
├── tasks/                            # Programas MIPS (JSON)
│   ├── tasks_quick.json              # 5 instruções
│   ├── tasks_short.json              # 5 instruções
│   ├── tasks_medium.json             # 5 instruções
│   ├── tasks_long.json               # 5 instruções
│   ├── tasks_cpu_bound.json          # 5 instruções (ALU)
│   ├── tasks_io_bound.json           # 5 instruções (I/O)
│   ├── tasks_memory_intensive.json   # 5 instruções (MEM)
│   ├── tasks_balanced.json           # 5 instruções (mix)
│   └── tasks_loop_heavy.json         # 100 instruções (loop)
│
├── scripts/                          # Ferramentas de análise
│   ├── compare_schedulers.py         # Compara políticas
│   ├── plot_memory.py                # Gráficos de memória
│   ├── plot_results.py               # Gráficos gerais
│   ├── generate_all_plots.sh         # Script mestre
│   └── requirements.txt              # Dependências Python
│
├── build/                            # Diretório de build (gerado)
│   ├── simulador                     # Executável principal
│   ├── test_*                        # Executáveis de teste
│   ├── processes/                    # Cópia dos JSONs
│   ├── tasks/                        # Cópia dos JSONs
│   └── output/                       # Resultados gerados
│       ├── resultados*.dat           # Relatórios de escalonamento
│       ├── comparacao_escalonadores*.txt # Comparações
│       ├── memory_usage_*.txt        # Relatórios individuais (9)
│       └── memory_aggregated_report.txt # Relatório agregado
│
└── plots/                            # Gráficos gerados (12 arquivos)
    ├── scheduler_time_comparison.png
    ├── scheduler_efficiency_comparison.png
    ├── scheduler_radar_comparison.png
    ├── memory_usage_timeline.png
    ├── cache_hit_rate_evolution.png
    ├── memory_heatmap.png
    ├── pipeline_cycles.png
    ├── memory_accesses.png
    ├── execution_times.png
    ├── cache_performance.png
    ├── comparison_matrix.png
    └── memory_final_summary.png
```

---

## Decisões de Projeto e Justificativas

### 1. Arquitetura Multicore Real (C++ Threads)

**Decisão:** Usar `std::thread` para implementar múltiplos cores.

**Justificativa:**
- Execução paralela verdadeira (não simulada)
- Sincronização com mutexes para memória compartilhada
- Demonstra conceitos reais de sistemas operacionais
- Permite análise de speedup real vs single-core

**Alternativas consideradas:**
- Simulação sequencial (não demonstraria paralelismo real)
- Processos UNIX (overhead muito alto)

### 2. Pipeline MIPS de 5 Estágios

**Decisão:** Implementar pipeline completo (IF → ID → EX → MEM → WB).

**Justificativa:**
- Conformidade com arquitetura MIPS clássica
- Demonstra conceito de pipeline em CPU real
- Permite análise de ciclos por instrução
- Base para extensões futuras (hazards, forwarding)

**Alternativas consideradas:**
- Execução direta (não representaria arquitetura real)
- Pipeline simplificado (perderia detalhes técnicos)

### 3. Segmentação (Modelo Tanenbaum)

**Decisão:** 4 segmentos (CODE, DATA, STACK, HEAP) com proteção.

**Justificativa:**
- Modelo clássico de Tanenbaum (livro texto)
- Proteção de código (read-only)
- Endereçamento lógico realista (32 bits)
- Preparado para paginação futura

**Alternativas consideradas:**
- Paginação pura (mais complexo para escopo do trabalho)
- Endereçamento plano (não demonstraria conceitos de SO)

### 4. Cache L1 com FIFO e LRU

**Decisão:** Duas políticas de substituição comparáveis.

**Justificativa:**
- FIFO: baseline simples (O(1))
- LRU: política realista usada em processadores reais
- Permite análise comparativa de desempenho
- Demonstra impacto de políticas na taxa de hit

**Alternativas consideradas:**
- Apenas FIFO (não mostraria melhorias)
- Políticas mais complexas (CLOCK, LFU) - fora do escopo

### 5. Quantum de 5 Ciclos (Round Robin)

**Decisão:** Quantum fixo de 5 ciclos de pipeline.

**Justificativa:**
- Demonstra preempção com processo Loop-Heavy (100 instruções)
- Quantum pequeno: maior interatividade
- Permite observar overhead de context switch
- Comparável com sistemas operacionais reais (Linux: 100ms ≈ milhões de ciclos)

**Alternativas consideradas:**
- Quantum de 1 ciclo (overhead excessivo)
- Quantum de 50 ciclos (não demonstraria preempção)

### 6. Rastreamento Temporal (Snapshots a cada 10 ciclos)

**Decisão:** Capturar estado de memória periodicamente.

**Justificativa:**
- Atende requisito: "utilização de memória ao longo do tempo"
- Intervalo de 10 ciclos: granularidade adequada
- Não impacta performance (baixo overhead)
- Permite análise temporal e geração de gráficos

**Alternativas consideradas:**
- Snapshot a cada ciclo (overhead excessivo, dados redundantes)
- Apenas snapshot final (não mostraria evolução temporal)

### 7. JSON para Configuração

**Decisão:** Processos e tarefas definidos em JSON.

**Justificativa:**
- Formato estruturado e legível
- Fácil modificação sem recompilar
- Separação de código e dados
- Suporte a lote inicial (9 processos)

**Alternativas consideradas:**
- Código hardcoded (inflexível)
- Entrada manual (não reproduzível)

### 8. Python para Visualização

**Decisão:** Scripts Python com matplotlib para gráficos.

**Justificativa:**
- matplotlib: biblioteca padrão para gráficos científicos
- Python: ampla adoção em análise de dados
- Separação de lógica (C++) e apresentação (Python)
- 12 gráficos gerados automaticamente

**Alternativas consideradas:**
- Integrar gráficos no C++ (dependências pesadas)
- Apenas texto (menos visual)

---

## Tecnologias Utilizadas

### Linguagens
- **C++17** - Linguagem principal do simulador
- **Python 3.8+** - Scripts de visualização
- **Bash** - Automação de build e testes

### Bibliotecas e Frameworks
- **STL (Standard Template Library)**
  - `<thread>` - Multithreading para multicore
  - `<mutex>` - Sincronização de memória compartilhada
  - `<atomic>` - Operações atômicas
  - `<chrono>` - Medição de tempo
  - `<vector>`, `<map>`, `<queue>` - Estruturas de dados
  
- **nlohmann/json** - Parsing de arquivos JSON
- **pthread** - Threads POSIX (backend do std::thread)

### Ferramentas Python
- **matplotlib** - Geração de gráficos
- **numpy** - Cálculos numéricos
- **pandas** (opcional) - Análise de dados

### Build System
- **CMake 3.10+** - Configuração de build multiplataforma
- **GNU Make** - Wrapper para comandos comuns
- **GCC 9+ / Clang 10+** - Compiladores C++ com suporte a C++17

### Controle de Versão
- **Git** - Versionamento de código

### Ambiente de Desenvolvimento
- **Linux Ubuntu 20.04+** - Sistema operacional alvo
- **VS Code / CLion** - IDEs sugeridas
- **GDB** - Debugging
- **Valgrind** - Detecção de memory leaks

---

## Equipe de Desenvolvimento

- João Pedro Rodrigues Silva ([jottynha](https://github.com/Jottynha))
- Eduardo da Silva Torres Grillo ([EduardoGrillo](https://github.com/EduardoGrillo))
- Samuel Silva Gomes ([samuelsilvg](https://github.com/samuelsilvg))
- Jader Oliveira Silva ([0livas](https://github.com/0livas))


## 📄 Licença

Este projeto é licenciado sob a **MIT License** - veja o arquivo [LICENSE](LICENSE) para detalhes.

```
MIT License

Copyright (c) 2025 CEFET-MG - Simulador Multicore Von Neumann

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

---

## 📚 Referências Bibliográficas

1. **TANENBAUM, Andrew S.; BOS, Herbert.** *Modern Operating Systems.* 4th ed. Pearson, 2014.
   - Capítulo 3: Memory Management (Segmentation)
   - Capítulo 2: Processes and Threads (Scheduling)

2. **PATTERSON, David A.; HENNESSY, John L.** *Computer Organization and Design: The Hardware/Software Interface.* 5th ed. Morgan Kaufmann, 2013.
   - Capítulo 4: The Processor (MIPS Pipeline)
   - Capítulo 5: Memory Hierarchy

3. **STALLINGS, William.** *Operating Systems: Internals and Design Principles.* 9th ed. Pearson, 2017.
   - Capítulo 9: Uniprocessor Scheduling
   - Capítulo 7: Memory Management

4. **SILBERSCHATZ, Abraham; GALVIN, Peter B.; GAGNE, Greg.** *Operating System Concepts.* 10th ed. Wiley, 2018.
   - Capítulo 6: CPU Scheduling
   - Capítulo 9: Virtual Memory

5. **HENNESSY, John L.; PATTERSON, David A.** *Computer Architecture: A Quantitative Approach.* 6th ed. Morgan Kaufmann, 2017.
   - Apêndice C: Pipelining

6. **Documentação nlohmann/json:** https://github.com/nlohmann/json
   - Parsing de JSON em C++

7. **CPPReference - std::thread:** https://en.cppreference.com/w/cpp/thread/thread
   - Multithreading em C++17

8. **Matplotlib Documentation:** https://matplotlib.org/stable/contents.html
   - Geração de gráficos científicos

---




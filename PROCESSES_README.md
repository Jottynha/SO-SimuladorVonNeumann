# 📋 Processos do Simulador Von Neumann

Este diretório contém as configurações dos processos disponíveis para simulação.

## 📂 Estrutura

- **processes/** - Arquivos JSON com configurações (PID, prioridade, quantum)
- **tasks/** - Arquivos JSON com programas MIPS (instruções)

## 🎯 Processos Disponíveis

### 1. Quick Process (Rápido)
- **Arquivo**: `process_quick.json` + `tasks_quick.json`
- **PID**: 1
- **Prioridade**: 1
- **Quantum**: 50
- **Características**: Pouquíssimas instruções (5 instruções)
- **Uso**: Teste de processos extremamente rápidos

### 2. Short Process (Curto)
- **Arquivo**: `process_short.json` + `tasks_short.json`
- **PID**: 2
- **Prioridade**: 2
- **Quantum**: 100
- **Características**: Alta localidade de cache (11 instruções)
- **Uso**: Processo curto com boa eficiência de cache

### 3. Medium Process (Médio)
- **Arquivo**: `process_medium.json` + `tasks_medium.json`
- **PID**: 3
- **Prioridade**: 2
- **Quantum**: 80
- **Características**: Balanceado entre CPU e memória (14 instruções)
- **Uso**: Processo de referência para comparações

### 4. Long Process (Longo)
- **Arquivo**: `process_long.json` + `tasks_long.json`
- **PID**: 4
- **Prioridade**: 1
- **Quantum**: 100
- **Características**: Alto reuso de cache, múltiplas leituras (19 instruções)
- **Uso**: Teste de eficiência com processos longos

### 5. CPU-Bound Process (Intensivo em CPU)
- **Arquivo**: `process_cpu_bound.json` + `tasks_cpu_bound.json`
- **PID**: 5
- **Prioridade**: 3
- **Quantum**: 150
- **Características**: Muitas operações aritméticas (19 instruções)
- **Uso**: Simula processos computacionalmente intensivos

### 6. IO-Bound Process (Intensivo em I/O)
- **Arquivo**: `process_io_bound.json` + `tasks_io_bound.json`
- **PID**: 6
- **Prioridade**: 1
- **Quantum**: 60
- **Características**: Muitos acessos à memória, mesma localidade (16 instruções)
- **Uso**: Simula processos que fazem muito I/O

### 7. Memory-Intensive Process (Intensivo em Memória)
- **Arquivo**: `process_memory_intensive.json` + `tasks_memory_intensive.json`
- **PID**: 7
- **Prioridade**: 2
- **Quantum**: 120
- **Características**: Acessos espalhados na memória (14 instruções)
- **Uso**: Teste de cache com baixa localidade espacial

### 8. Balanced Process (Balanceado)
- **Arquivo**: `process_balanced.json` + `tasks_balanced.json`
- **PID**: 8
- **Prioridade**: 2
- **Quantum**: 90
- **Características**: Mix equilibrado de operações (15 instruções)
- **Uso**: Processo de uso geral

## 📊 Comparação de Características

| Processo | Instruções | Quantum | Prioridade | Tipo | Cache Hit Esperado |
|----------|------------|---------|------------|------|-------------------|
| Quick | 5 | 50 | 1 | Rápido | Médio |
| Short | 11 | 100 | 2 | Curto | Alto |
| Medium | 14 | 80 | 2 | Médio | Médio-Alto |
| Long | 19 | 100 | 1 | Longo | Muito Alto |
| CPU-Bound | 19 | 150 | 3 | Computação | Alto |
| IO-Bound | 16 | 60 | 1 | I/O | Muito Alto |
| Memory-Intensive | 14 | 120 | 2 | Memória | Baixo |
| Balanced | 15 | 90 | 2 | Geral | Médio |

## 🎮 Como Usar

1. Os processos são carregados automaticamente pelo simulador
2. Escolha o algoritmo de escalonamento (FCFS, SJN, Priority)
3. Compare os resultados em `output/comparacao_escalonadores.txt`

## 📝 Formato dos Arquivos

### process_*.json
```json
{
  "pid": 1,
  "name": "Nome do Processo",
  "quantum": 100,
  "priority": 1
}
```

### tasks_*.json
```json
{
  "program": [
    { "instruction": "li", "rt": "$t0", "immediate": 10 },
    { "instruction": "end" }
  ]
}
```

## 🔧 Criando Novos Processos

Para adicionar um novo processo:
1. Crie `processes/process_nome.json` com a configuração
2. Crie `tasks/tasks_nome.json` com as instruções
3. Atualize `src/main.cpp` para carregar o novo processo
4. Recompile: `cd build && make`

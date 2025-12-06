#!/bin/bash
# Script para testar e comparar políticas de cache FIFO vs LRU

set -e

# Garantir que números usem ponto decimal
export LC_NUMERIC=C

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"
SIMULADOR="$BUILD_DIR/simulador"

echo "========================================"
echo "  TESTE DE POLÍTICAS DE CACHE"
echo "  FIFO vs LRU"
echo "========================================"
echo

# Verificar se o simulador existe
if [ ! -f "$SIMULADOR" ]; then
    echo "Erro: Simulador não encontrado em $SIMULADOR"
    echo "Execute 'make' no diretório build primeiro."
    exit 1
fi

# Criar diretórios de saída
mkdir -p "$PROJECT_ROOT/build/output/fifo_1core"
mkdir -p "$PROJECT_ROOT/build/output/lru_1core"
mkdir -p "$PROJECT_ROOT/build/output/fifo_8cores"
mkdir -p "$PROJECT_ROOT/build/output/lru_8cores"

echo "========== TESTE 1: Single-Core (1 core) =========="
echo

# FIFO - 1 core
echo "[1/4] Executando FIFO com 1 core..."
cd "$BUILD_DIR"
./simulador --cores 1 --replacement FIFO --scheduler FCFS \
    --output "$PROJECT_ROOT/build/output/fifo_1core" \
    > "$PROJECT_ROOT/build/output/fifo_1core/log.txt" 2>&1
echo "      Concluído! Resultados em build/output/fifo_1core/"

# LRU - 1 core
echo "[2/4] Executando LRU com 1 core..."
./simulador --cores 1 --replacement LRU --scheduler FCFS \
    --output "$PROJECT_ROOT/build/output/lru_1core" \
    > "$PROJECT_ROOT/build/output/lru_1core/log.txt" 2>&1
echo "      Concluído! Resultados em build/output/lru_1core/"

echo
echo "========== TESTE 2: Multi-Core (8 cores) =========="
echo

# FIFO - 8 cores
echo "[3/4] Executando FIFO com 8 cores..."
./simulador --cores 8 --replacement FIFO --scheduler FCFS \
    --output "$PROJECT_ROOT/build/output/fifo_8cores" \
    > "$PROJECT_ROOT/build/output/fifo_8cores/log.txt" 2>&1
echo "      Concluído! Resultados em build/output/fifo_8cores/"

# LRU - 8 cores
echo "[4/4] Executando LRU com 8 cores..."
./simulador --cores 8 --replacement LRU --scheduler FCFS \
    --output "$PROJECT_ROOT/build/output/lru_8cores" \
    > "$PROJECT_ROOT/build/output/lru_8cores/log.txt" 2>&1
echo "      Concluído! Resultados em build/output/lru_8cores/"

echo
echo "========== ANÁLISE DOS RESULTADOS =========="
echo

# Função para extrair métricas do CSV
extract_metrics() {
    local csv_file="$1"
    local policy_name="$2"
    
    if [ ! -f "$csv_file" ]; then
        echo "Arquivo não encontrado: $csv_file"
        return
    fi
    
    # Pular comentários e cabeçalho, pegar linha de dados (FCFS)
    local line=$(grep -v '^#' "$csv_file" | tail -n +2 | head -n 1)
    
    if [ -z "$line" ]; then
        echo "Nenhum dado encontrado em $csv_file"
        return
    fi
    
    # Novo formato: Scheduler,ExecTime_ms,Throughput,Processes,ContextSwitches,AvgWaitTime_ms,AvgTurnaroundTime_ms,AvgResponseTime_ms,CPUUtilization,CacheHitRate,Efficiency,Cores,Threading
    IFS=',' read -r scheduler time tput procs ctx_sw wait turn resp cpu_util hit_rate eff cores threading <<< "$line"
    
    printf "%-15s %10.2f ms | Hit: %6.2f%% | Throughput: %6.2f | CPU: %6.2f%% | Ctx Sw: %8s\n" \
        "$policy_name" "$time" "$hit_rate" "$tput" "$cpu_util" "$ctx_sw"
}

echo "Single-Core (1 core):"
extract_metrics "$PROJECT_ROOT/build/output/fifo_1core/metrics_single.csv" "FIFO"
extract_metrics "$PROJECT_ROOT/build/output/lru_1core/metrics_single.csv" "LRU"

echo
echo "Multi-Core (8 cores):"
extract_metrics "$PROJECT_ROOT/build/output/fifo_8cores/metrics_multi.csv" "FIFO"
extract_metrics "$PROJECT_ROOT/build/output/lru_8cores/metrics_multi.csv" "LRU"

echo
echo "========== COMPARAÇÃO DETALHADA =========="
echo

# Calcular diferenças
calculate_improvement() {
    local fifo_csv="$1"
    local lru_csv="$2"
    local cores_label="$3"
    
    if [ ! -f "$fifo_csv" ] || [ ! -f "$lru_csv" ]; then
        echo "Arquivos não encontrados para comparação"
        return
    fi
    
    # Extrair dados (ignorar comentários)
    local fifo_line=$(grep -v '^#' "$fifo_csv" | tail -n +2 | head -n 1)
    local lru_line=$(grep -v '^#' "$lru_csv" | tail -n +2 | head -n 1)
    
    # Extrair hit rates (campo 10) e tempos (campo 2)
    local fifo_hit=$(echo "$fifo_line" | cut -d',' -f10)
    local lru_hit=$(echo "$lru_line" | cut -d',' -f10)
    
    local fifo_time=$(echo "$fifo_line" | cut -d',' -f2)
    local lru_time=$(echo "$lru_line" | cut -d',' -f2)
    
    echo "$cores_label:"
    echo "  Cache Hit Rate:"
    echo "    FIFO: $fifo_hit%"
    echo "    LRU:  $lru_hit%"
    
    # Calcular melhoria usando bc
    local hit_improvement=$(echo "scale=2; $lru_hit - $fifo_hit" | bc)
    if (( $(echo "$hit_improvement > 0" | bc -l) )); then
        echo "    → LRU é ${hit_improvement}% melhor"
    else
        echo "    → FIFO é $(echo "scale=2; -1 * $hit_improvement" | bc)% melhor"
    fi
    
    echo "  Tempo de Execução:"
    echo "    FIFO: ${fifo_time} ms"
    echo "    LRU:  ${lru_time} ms"
    
    local time_improvement=$(echo "scale=2; (($fifo_time - $lru_time) / $fifo_time) * 100" | bc)
    if (( $(echo "$time_improvement > 0" | bc -l) )); then
        echo "    → LRU é ${time_improvement}% mais rápido"
    else
        echo "    → FIFO é $(echo "scale=2; -1 * $time_improvement" | bc)% mais rápido"
    fi
    echo
}

calculate_improvement "$PROJECT_ROOT/build/output/fifo_1core/metrics_single.csv" \
                     "$PROJECT_ROOT/build/output/lru_1core/metrics_single.csv" \
                     "Single-Core (1 core)"

calculate_improvement "$PROJECT_ROOT/build/output/fifo_8cores/metrics_multi.csv" \
                     "$PROJECT_ROOT/build/output/lru_8cores/metrics_multi.csv" \
                     "Multi-Core (8 cores)"

echo "=========================================="
echo "Teste concluído!"
echo "Logs detalhados em build/output/*/log.txt"
echo "=========================================="

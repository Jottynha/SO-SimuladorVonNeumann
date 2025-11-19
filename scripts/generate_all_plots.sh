#!/bin/bash
# Script mestre para gerar todos os gráficos do simulador
# Autor: João
# Data: 19/11/2025

set -e  # Parar em caso de erro

echo "═══════════════════════════════════════════════════════════════════════"
echo "GERADOR DE GRÁFICOS - SIMULADOR MULTICORE VON NEUMANN"
echo "═══════════════════════════════════════════════════════════════════════"



# Verificar se Python3 está instalado
if ! command -v python3 &> /dev/null; then
    echo -e "$ Python3 não encontrado!${NC}"
    echo "Por favor, instale Python3 primeiro."
    exit 1
fi

# Verificar se matplotlib está instalado
python3 -c "import matplotlib" 2>/dev/null || {
    echo -e "$ matplotlib não encontrado. Instalando...${NC}"
    pip3 install matplotlib numpy --user
}

# Diretório de scripts
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUTPUT_DIR="plots"

# Criar diretório de plots se não existir
mkdir -p "$OUTPUT_DIR"

echo ""
echo "Diretório de scripts: $SCRIPT_DIR"
echo "Diretório de saída: $OUTPUT_DIR"
echo ""

# 1. Gráficos de resultados gerais
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Gerando gráficos de resultados gerais..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
if python3 "$SCRIPT_DIR/plot_results.py"; then
    echo -e "$ Gráficos de resultados gerados!${NC}"
else
    echo -e "$ Falha ao gerar gráficos de resultados${NC}"
fi

echo ""

# 2. Comparação de escalonadores
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Gerando comparação entre escalonadores..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
if python3 "$SCRIPT_DIR/compare_schedulers.py"; then
    echo -e "$ Gráficos de comparação gerados!${NC}"
else
    echo -e "$  Falha ao gerar comparação (execute o simulador com opção 5)${NC}"
fi

echo ""

# 3. Visualização de memória
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo " Gerando gráficos de uso de memória..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
if python3 "$SCRIPT_DIR/plot_memory.py"; then
    echo -e "$ Gráficos de memória gerados!${NC}"
else
    echo -e "$  Falha ao gerar gráficos de memória${NC}"
fi

echo ""
echo "═══════════════════════════════════════════════════════════════════════"
echo -e "$ GERAÇÃO DE GRÁFICOS CONCLUÍDA!${NC}"
echo "═══════════════════════════════════════════════════════════════════════"
echo ""
echo "Todos os gráficos foram salvos em: $OUTPUT_DIR/"
echo ""
echo "Gráficos gerados:"
echo "  • execution_times.png - Tempos de execução"
echo "  • memory_accesses.png - Acessos à memória"
echo "  • cache_performance.png - Desempenho de cache"
echo "  • pipeline_cycles.png - Ciclos de pipeline"
echo "  • comparison_matrix.png - Matriz comparativa"
echo "  • scheduler_time_comparison.png - Comparação de tempos"
echo "  • scheduler_efficiency_comparison.png - Eficiência"
echo "  • scheduler_radar_comparison.png - Gráfico radar"
echo "  • memory_usage_timeline.png - Timeline de memória"
echo "  • cache_hit_rate_evolution.png - Evolução cache hit"
echo "  • memory_heatmap.png - Mapa de calor"
echo "  • memory_final_summary.png - Resumo final"


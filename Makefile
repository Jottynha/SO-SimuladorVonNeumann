# ============================================================================
# Makefile - SO-SimuladorVonNeumann
# Wrapper simplificado para configuração e build do projeto com CMake
# ============================================================================

# Diretórios
BUILD_DIR := build
SCRIPTS_DIR := scripts

# Alvos principais
.PHONY: all setup build run clean help test check plots install-deps

# Alvo padrão: configura e compila o projeto
all: setup build

# Alvo padrão: configura e compila o projeto
all: setup build

# ============================================================================
# SETUP - Configuração inicial do projeto (cria build/ e executa cmake)
# ============================================================================
setup:
	@echo "🔧 Configurando o projeto..."
	@if [ ! -d "$(BUILD_DIR)" ]; then \
		echo "  📁 Criando diretório build/..."; \
		mkdir -p $(BUILD_DIR); \
	fi
	@echo "  ⚙️  Executando cmake..."
	@cd $(BUILD_DIR) && cmake .. > /dev/null
	@echo "✅ Configuração concluída!"

# ============================================================================
# BUILD - Compila o simulador e testes
# ============================================================================
build:
	@echo "🔨 Compilando o projeto..."
	@cd $(BUILD_DIR) && $(MAKE) --no-print-directory
	@echo "✅ Compilação concluída!"

# ============================================================================
# RUN - Executa o simulador principal
# ============================================================================
run:
	@if [ ! -f "$(BUILD_DIR)/simulador" ]; then \
		echo "❌ Simulador não encontrado. Execute 'make' primeiro."; \
		exit 1; \
	fi
	@echo "🚀 Executando o simulador...\n"
	@cd $(BUILD_DIR) && ./simulador

# ============================================================================
# TEST - Executa todos os testes
# ============================================================================
test:
	@echo "🧪 Executando testes..."
	@cd $(BUILD_DIR) && $(MAKE) test-all --no-print-directory

# ============================================================================
# CHECK - Verificação rápida dos componentes
# ============================================================================
check:
	@echo "✔️  Verificando componentes..."
	@cd $(BUILD_DIR) && $(MAKE) check --no-print-directory

# ============================================================================
# PLOTS - Gera gráficos de análise de desempenho
# ============================================================================
plots:
	@echo "📊 Gerando gráficos..."
	@cd $(BUILD_DIR) && $(MAKE) plots --no-print-directory

# PLOTS-EXTENDED - Análise estendida com degradação e comparações
# ============================================================================
plots-extended:
	@echo "📊 Gerando análise estendida de desempenho..."
	@if [ ! -f "$(BUILD_DIR)/output/metrics_single.csv" ] || [ ! -f "$(BUILD_DIR)/output/metrics_multi.csv" ]; then \
		echo "❌ Arquivos CSV não encontrados."; \
		echo "   Execute o simulador primeiro:"; \
		echo "     1. make run  # Digite: 1, n, 5 (single-core)"; \
		echo "     2. make run  # Digite: 8, y, 5 (multi-core)"; \
		exit 1; \
	fi
	@cd $(BUILD_DIR) && python3 ../scripts/analyze_performance_extended.py output plots
	@echo "✅ Análise estendida concluída! Veja os gráficos em build/plots/"

# ============================================================================
# CLEAN - Remove arquivos de build
# ============================================================================
clean:
	@echo "🧹 Limpando arquivos de build..."
	@rm -rf $(BUILD_DIR)
	@echo "✅ Limpeza concluída!"

# ============================================================================
# CLEAN-RESULTS - Remove apenas resultados de simulação
# ============================================================================
clean-results:
	@echo "🧹 Limpando resultados de simulação..."
	@rm -rf $(BUILD_DIR)/output/*.dat $(BUILD_DIR)/output/*.txt $(BUILD_DIR)/output/*.csv
	@rm -rf plots/*.png
	@echo "✅ Resultados removidos!"

# ============================================================================
# INSTALL-DEPS - Instala dependências do Python para análise
# ============================================================================
install-deps:
	@echo "📦 Instalando dependências Python..."
	@pip3 install -r $(SCRIPTS_DIR)/requirements.txt
	@echo "✅ Dependências instaladas!"

# ============================================================================
# HELP - Mostra comandos disponíveis
# ============================================================================
help:
	@echo ""
	@echo "╔════════════════════════════════════════════════════════════════╗"
	@echo "║        SO-SimuladorVonNeumann - Comandos Disponíveis          ║"
	@echo "╚════════════════════════════════════════════════════════════════╝"
	@echo ""
	@echo "📦 CONFIGURAÇÃO E BUILD:"
	@echo "  make                 - Configura e compila o projeto (setup + build)"
	@echo "  make setup           - Cria build/ e executa cmake"
	@echo "  make build           - Compila o simulador e testes"
	@echo "  make install-deps    - Instala dependências Python (matplotlib, etc.)"
	@echo ""
	@echo "🚀 EXECUÇÃO:"
	@echo "  make run             - Executa o simulador principal"
	@echo "  make test            - Executa todos os testes"
	@echo "  make check           - Verificação rápida (PASSOU/FALHOU)"
	@echo ""
	@echo "📊 ANÁLISE:"
	@echo "  make plots           - Gera gráficos de desempenho"
	@echo "  make plots-extended  - Análise estendida (degradação, speedup, comparações)"
	@echo ""
	@echo "🧹 LIMPEZA:"
	@echo "  make clean           - Remove diretório build/ completo"
	@echo "  make clean-results   - Remove apenas resultados (.dat, .csv, .png)"
	@echo ""
	@echo "ℹ️  AJUDA:"
	@echo "  make help            - Mostra esta mensagem"
	@echo ""
	@echo "📋 EXEMPLO DE USO RÁPIDO:"
	@echo "  git clone <repo>"
	@echo "  cd SO-SimuladorVonNeumann"
	@echo "  make              # Configura e compila"
	@echo "  make run          # Executa o simulador"
	@echo ""

.DEFAULT_GOAL := all

# Compilador e flags
CXX := g++
CXXFLAGS := -Wall -Wextra -g -std=c++17 -Isrc

# Alvos principais
TARGET := teste
TARGET_HASH := test_hash_register
TARGET_BANK := test_register_bank

# Fontes principais
SRC := src/teste.cpp src/cpu/ULA.cpp
OBJ := $(SRC:.cpp=.o)

# Fontes para teste do hash register
SRC_HASH := src/test_hash_register.cpp
OBJ_HASH := $(SRC_HASH:.cpp=.o)

# Fontes para teste do register bank
SRC_BANK := src/test_register_bank.cpp src/cpu/REGISTER_BANK.cpp
OBJ_BANK := $(SRC_BANK:.cpp=.o)

# Make clean -> make -> make run
all: clean $(TARGET) run

# Regra para o programa principal
$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ)

# Regra para o teste do hash register
$(TARGET_HASH): $(OBJ_HASH)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ_HASH)

# Regra para o teste do register bank
$(TARGET_BANK): $(OBJ_BANK)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ_BANK)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	@echo "Limpando arquivos antigos..."
	@rm -f $(OBJ) $(OBJ_HASH) $(TARGET) $(TARGET_HASH) $(TARGET_BANK)

# Limpa apenas os gráficos gerados
clean-plots:
	@echo "Limpando gráficos..."
	@rm -rf plots/*.png
	@echo "Gráficos removidos (plots/*.png)"

# Limpa apenas os resultados de simulação
clean-output:
	@echo "Limpando resultados de simulação..."
	@rm -rf output/*.dat output/*.txt build/output/*.dat build/output/*.txt
	@echo "Resultados removidos (output/*.dat, output/*.txt)"

# Limpa tanto plots quanto outputs
clean-results: clean-plots clean-output
	@echo "Todos os resultados de simulação foram removidos!"

run:
	@echo "Executando o programa..."
	@./$(TARGET)

# Teste específico para hash register
test-hash: clean $(TARGET_HASH)
	@echo "Executando teste do Hash Register..."
	@./$(TARGET_HASH)

# Teste específico para register bank
test-bank: clean $(TARGET_BANK)
	@echo "Executando teste do Register Bank..."
	@./$(TARGET_BANK)

# Testa ambos os programas
test-all: clean $(TARGET) $(TARGET_HASH)
	@echo "Executando programa principal..."
	@./$(TARGET)
	@echo ""
	@echo "Executando teste do Hash Register..."
	@./$(TARGET_HASH)
	@echo ""
	@echo "Executando teste do Register Bank..."
	@./$(TARGET_BANK)

# Comando de ajuda
help:
	@echo "SO-SimuladorVonNeumann - Comandos Disponíveis:"
	@echo ""
	@echo "  make / make all    - Compila e executa programa principal"
	@echo "  make clean         - Remove arquivos gerados (.o, executáveis)"
	@echo "  make run          - Executa programa principal (sem recompilar)"
	@echo "  make teste        - Compila apenas o programa principal"
	@echo "  make test-hash    - Compila e testa sistema de registradores"
	@echo "  make test-bank    - Compila e testa o banco de registradores"
	@echo "  make test-all     - Executa todos os testes disponíveis"
	@echo "  make check        - Verificação rápida de todos os componentes"
	@echo "  make debug        - Build com símbolos de debug (-g -O0)"
	@echo "  make help         - Mostra esta mensagem de ajuda"
	@echo ""
	@echo "Limpeza de Resultados:"
	@echo "  make clean-plots   - Remove todos os gráficos (plots/*.png)"
	@echo "  make clean-output  - Remove resultados de simulação (*.dat, *.txt)"
	@echo "  make clean-results - Remove plots + outputs (limpeza completa)"
	@echo ""
	@echo "Informações do Projeto:"
	@echo "  Compilador: $(CXX)"
	@echo "  Flags: $(CXXFLAGS)"
	@echo "  Arquivos fonte: $(words $(SRC) $(SRC_HASH)) arquivos"

# Verificação rápida de todos os componentes
check: $(TARGET) $(TARGET_HASH)
	@echo "Executando verificações rápidas..."
	@echo -n "  Teste principal: "; ./$(TARGET) >/dev/null 2>&1 && echo "PASSOU" || echo "FALHOU"
	@echo -n "  Teste hash register: "; ./$(TARGET_HASH) >/dev/null 2>&1 && echo "PASSOU" || echo "FALHOU"
	@echo -n "  Teste register bank: "; ./$(TARGET_BANK) >/dev/null 2>&1 && echo "PASSOU" || echo "FALHOU"
	@echo "Verificação concluída!"

# Build com debug symbols
debug: CXXFLAGS += -DDEBUG -O0 -ggdb3
debug: clean $(TARGET)
	@echo "Build de debug criado com símbolos completos"
	@echo "   Use: gdb ./$(TARGET) para debug"

# Lista arquivos do projeto
list-files:
	@echo "Arquivos do projeto:"
	@echo "  Fontes principais: $(SRC)"
	@echo "  Fontes de teste: $(SRC_HASH)"
	@echo "  Headers: $(shell find src -name '*.hpp' 2>/dev/null)"

.PHONY: all clean run test-hash test-all help check debug list-files clean-plots clean-output clean-results

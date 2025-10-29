#ifndef SEGMENT_TABLE_HPP
#define SEGMENT_TABLE_HPP

#include <cstdint>
#include <vector>
#include <string>

// Estrutura de um segmento de memória (Modelo Tanenbaum)
struct Segment {
    uint32_t base;          // Endereço base do segmento
    uint32_t limit;         // Tamanho do segmento
    bool present;           // Segmento está carregado na memória?
    bool read_only;         // Segmento é somente leitura?
    std::string name;       // Nome do segmento (CODE, DATA, STACK, HEAP)
    
    Segment() : base(0), limit(0), present(false), read_only(false), name("UNDEFINED") {}
    
    Segment(uint32_t b, uint32_t l, bool p, bool ro, const std::string& n)
        : base(b), limit(l), present(p), read_only(ro), name(n) {}
    
    // Verifica se um endereço está dentro dos limites do segmento
    bool isValidOffset(uint32_t offset) const {
        return offset < limit;
    }
    
    // Calcula o endereço físico a partir do offset
    uint32_t getPhysicalAddress(uint32_t offset) const {
        if (!present) {
            throw std::runtime_error("Segment not present in memory (segment fault)");
        }
        if (!isValidOffset(offset)) {
            throw std::runtime_error("Segmentation fault: offset exceeds segment limit");
        }
        return base + offset;
    }
};

// Tabela de Segmentos de um Processo
class SegmentTable {
private:
    std::vector<Segment> segments;
    int process_id;

public:
    SegmentTable(int pid) : process_id(pid) {
        // Inicializa com 4 segmentos padrão: CODE, DATA, STACK, HEAP
        segments.resize(4);
    }
    
    // Define um segmento específico
    void setSegment(int segment_number, uint32_t base, uint32_t limit, 
                    bool read_only, const std::string& name) {
        if (segment_number >= 0 && segment_number < static_cast<int>(segments.size())) {
            segments[segment_number] = Segment(base, limit, true, read_only, name);
        }
    }
    
    // Traduz endereço lógico (segmento:offset) para endereço físico
    uint32_t translate(int segment_number, uint32_t offset) const {
        if (segment_number < 0 || segment_number >= static_cast<int>(segments.size())) {
            throw std::runtime_error("Invalid segment number");
        }
        
        const Segment& seg = segments[segment_number];
        return seg.getPhysicalAddress(offset);
    }
    
    // Verifica se uma operação de escrita é permitida
    bool canWrite(int segment_number) const {
        if (segment_number < 0 || segment_number >= static_cast<int>(segments.size())) {
            return false;
        }
        return !segments[segment_number].read_only && segments[segment_number].present;
    }
    
    // Obtém informações de um segmento
    const Segment& getSegment(int segment_number) const {
        if (segment_number < 0 || segment_number >= static_cast<int>(segments.size())) {
            throw std::runtime_error("Invalid segment number");
        }
        return segments[segment_number];
    }
    
    // Retorna o número de segmentos
    size_t getSegmentCount() const {
        return segments.size();
    }
    
    int getProcessId() const {
        return process_id;
    }
};

// Classe para gerenciar endereçamento segmentado
class SegmentedAddressing {
public:
    // Extrai número do segmento e offset de um endereço linear
    // Formato: bits 31-30 = segmento (2 bits = 4 segmentos)
    //          bits 29-0  = offset (30 bits)
    static void decodeAddress(uint32_t linear_address, int& segment_number, uint32_t& offset) {
        segment_number = (linear_address >> 30) & 0x3;  // 2 bits mais significativos
        offset = linear_address & 0x3FFFFFFF;           // 30 bits menos significativos
    }
    
    // Cria um endereço linear a partir de segmento e offset
    static uint32_t encodeAddress(int segment_number, uint32_t offset) {
        return ((segment_number & 0x3) << 30) | (offset & 0x3FFFFFFF);
    }
    
    // Documentação do mapeamento de endereços
    static std::string getAddressingDocumentation() {
        return R"(
=== MODELO DE ENDEREÇAMENTO SEGMENTADO (TANENBAUM) ===

1. ESTRUTURA DO ENDEREÇO LÓGICO (32 bits):
   ┌────────────┬──────────────────────────────────┐
   │ Segmento   │         Offset                   │
   │  (2 bits)  │        (30 bits)                 │
   └────────────┴──────────────────────────────────┘
   Bits: 31-30     29-0

2. SEGMENTOS DEFINIDOS:
   - Segmento 0 (00): CODE   - Código do programa (Read-Only)
   - Segmento 1 (01): DATA   - Dados estáticos
   - Segmento 2 (10): STACK  - Pilha de execução
   - Segmento 3 (11): HEAP   - Memória dinâmica

3. TRADUÇÃO DE ENDEREÇOS:
   Endereço Lógico → Tabela de Segmentos → Endereço Físico
   
   a) Extrair número do segmento (bits 31-30)
   b) Extrair offset (bits 29-0)
   c) Verificar se offset < limite do segmento
   d) Calcular: endereço_físico = base_segmento + offset

4. PROTEÇÃO:
   - Verificação de limites (bound checking)
   - Segmentos CODE são read-only
   - Segmentation fault se offset > limite

5. EXEMPLO:
   Endereço Lógico: 0x40001000 (binário: 01 00000000000000000001000000000000)
   - Segmento: 1 (DATA)
   - Offset: 0x00001000
   - Se DATA.base = 0x2000 e DATA.limit = 0x4000:
     Endereço Físico = 0x2000 + 0x1000 = 0x3000
)";
    }
};

#endif // SEGMENT_TABLE_HPP

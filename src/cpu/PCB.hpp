#ifndef PCB_HPP
#define PCB_HPP
/*
  PCB.hpp
  Definição do bloco de controle de processo (PCB) usado pelo simulador da CPU.
  Centraliza: identificação do processo, prioridade, quantum, pesos de memória e
  contadores de instrumentação de pipeline/memória.
*/
#include <string>
#include <atomic>
#include <cstdint>
#include <vector>
#include <chrono>
#include "memory/cache.hpp"
#include "REGISTER_BANK.hpp" // necessidade de objeto completo dentro do PCB


// Estados possíveis do processo (simplificado)
enum class State {
    Ready,
    Running,
    Blocked,
    Finished
};

struct MemWeights {
    uint64_t cache = 1;   // custo por acesso à memória cache
    uint64_t primary = 5; // custo por acesso à memória primária
    uint64_t secondary = 10; // custo por acesso à memória secundária
};

struct PCB {
    int pid = 0;
    std::string name;
    int quantum = 5; // Valor padrão para quantum (reduzido para demonstrar preempção)
    int priority = 0;
    size_t base_address = 0; // Endereço base do processo na memória

    State state = State::Ready;
    hw::REGISTER_BANK regBank;
    int instruction_count = 0; // Contador de instruções executadas

    std::vector<std::string> execution_log; // Log de execução

    // Campos auxiliares para detectar estagnação (loop infinito)
    int stagnation_counter = 0;    // conta quantas vezes o processo voltou pronto sem avançar
    int last_instruction_count = 0; // última contagem de instruções observada

    // Contadores de acesso à memória
    std::atomic<uint64_t> primary_mem_accesses{0};
    std::atomic<uint64_t> secondary_mem_accesses{0};
    std::atomic<uint64_t> memory_cycles{0};
    std::atomic<uint64_t> mem_accesses_total{0};
    std::atomic<uint64_t> extra_cycles{0};
    std::atomic<uint64_t> cache_mem_accesses{0};

    // Instrumentação detalhada
    std::atomic<uint64_t> pipeline_cycles{0};
    std::atomic<uint64_t> stage_invocations{0};
    std::atomic<uint64_t> mem_reads{0};
    std::atomic<uint64_t> mem_writes{0};

    // Novos contadores
    std::atomic<uint64_t> cache_hits{0};
    std::atomic<uint64_t> cache_misses{0};
    std::atomic<uint64_t> io_cycles{1};

    // Métricas de tempo para escalonamento
    std::chrono::time_point<std::chrono::high_resolution_clock> arrival_time;
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
    std::chrono::time_point<std::chrono::high_resolution_clock> finish_time;
    uint64_t wait_time_ms = 0;        // Tempo de espera
    uint64_t turnaround_time_ms = 0;  // Tempo de retorno (turnaround)
    uint64_t response_time_ms = 0;    // Tempo de resposta
    bool first_run = true;             // Indica se é a primeira execução

    // Rastreamento de utilização de memória ao longo do tempo
    struct MemorySnapshot {
        std::chrono::time_point<std::chrono::high_resolution_clock> timestamp;
        uint64_t cache_usage;      // Bytes em uso na cache
        uint64_t ram_usage;        // Bytes em uso na RAM
        uint64_t total_accesses;   // Total de acessos até o momento
        double cache_hit_rate;     // Taxa de hit da cache no momento
    };
    std::vector<MemorySnapshot> memory_usage_timeline;

    MemWeights memWeights;

    PCB() : state(State::Ready), base_address(0), quantum(10), instruction_count(0) {}
};

// Contabilizar cache
inline void contabiliza_cache(PCB &pcb, bool hit) {
    if (hit) {
        pcb.cache_hits++;
    } else {
        pcb.cache_misses++;
    }
}

#endif // PCB_HPP
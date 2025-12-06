#ifndef MEMORY_MANAGER_HPP
#define MEMORY_MANAGER_HPP

#include <memory>
#include <stdexcept>
#include "MAIN_MEMORY.hpp"
#include "SECONDARY_MEMORY.hpp"
#include "cache.hpp" // Incluir a cache
#include "cachePolicy.hpp" // Incluir para ReplacementPolicy enum
#include "../cpu/PCB.hpp" // Incluir o PCB para as métricas

const size_t MAIN_MEMORY_SIZE = 1024;

class MemoryManager {
public:
    MemoryManager(size_t mainMemorySize, size_t secondaryMemorySize);
    ~MemoryManager();  // Destrutor para limpar cache antes de destruir memórias

    // Métodos unificados agora recebem o PCB para as métricas
    uint32_t read(uint32_t address, PCB& process);
    void write(uint32_t address, uint32_t data, PCB& process);
    
    // Método para resetar a cache (útil entre execuções de diferentes escalonadores)
    void resetCache();
    
    // Simula cache pollution (invalidação parcial) durante context switch
    void simulateContextSwitch();
    
    // Simula context switch SEM invalidar cache (para single-core sem preempção)
    void simulateContextSwitchLight();
    
    // Função auxiliar para o write-back da cache
    void writeToFile(uint32_t address, uint32_t data);
    
    // Métodos para configurar e obter política de cache
    void setCachePolicy(ReplacementPolicy policy);
    ReplacementPolicy getCachePolicy() const;

private:
    std::unique_ptr<MAIN_MEMORY> mainMemory;
    std::unique_ptr<SECONDARY_MEMORY> secondaryMemory;
    std::unique_ptr<Cache> L1_cache; // Adiciona a Cache L1

    size_t mainMemoryLimit;
};

#endif // MEMORY_MANAGER_HPP
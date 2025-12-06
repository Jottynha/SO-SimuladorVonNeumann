#include "MemoryManager.hpp"
#include "cachePolicy.hpp"

MemoryManager::MemoryManager(size_t mainMemorySize, size_t secondaryMemorySize) {
    mainMemory = std::make_unique<MAIN_MEMORY>(mainMemorySize);
    secondaryMemory = std::make_unique<SECONDARY_MEMORY>(secondaryMemorySize);
    // Inicializa com FIFO por padrão (pode ser mudado para LRU)
    L1_cache = std::make_unique<Cache>(ReplacementPolicy::FIFO);
    mainMemoryLimit = mainMemorySize;
}

MemoryManager::~MemoryManager() {
    // CRÍTICO: Limpar cache ANTES de destruir as memórias
    // para evitar write-back em memórias já destruídas
    if (L1_cache) {
        L1_cache->invalidate();
        L1_cache.reset();  // Destruir cache explicitamente
    }
    // Agora podemos destruir as memórias com segurança
    mainMemory.reset();
    secondaryMemory.reset();
}

uint32_t MemoryManager::read(uint32_t address, PCB& process) {
    process.mem_accesses_total.fetch_add(1);
    process.mem_reads.fetch_add(1);

    // 1. Tenta ler da Cache
    size_t cache_data = L1_cache->get(address);
    if (cache_data != CACHE_MISS) {
        process.cache_mem_accesses.fetch_add(1);
        process.memory_cycles.fetch_add(process.memWeights.cache);

        contabiliza_cache(process, true);  // HIT
        return cache_data;
    }

    // 2. Cache Miss: busca na memória correta
    contabiliza_cache(process, false); // MISS

    uint32_t data_from_mem;
    if (address < mainMemoryLimit) {
        process.primary_mem_accesses.fetch_add(1);
        process.memory_cycles.fetch_add(process.memWeights.primary);
        data_from_mem = mainMemory->ReadMem(address);
    } else {
        process.secondary_mem_accesses.fetch_add(1);
        process.memory_cycles.fetch_add(process.memWeights.secondary);
        uint32_t secondaryAddress = address - mainMemoryLimit;
        data_from_mem = secondaryMemory->ReadMem(secondaryAddress);
    }

    // 3. Após a busca, armazena o dado na cache
    L1_cache->put(address, data_from_mem, this);

    return data_from_mem;
}

void MemoryManager::write(uint32_t address, uint32_t data, PCB& process) {
    process.mem_accesses_total.fetch_add(1);
    process.mem_writes.fetch_add(1);

    size_t cache_data = L1_cache->get(address);

    if (cache_data == CACHE_MISS) {
        contabiliza_cache(process, false); // MISS
        read(address, process); // Write-allocate: busca e coloca na cache
    } else {
        contabiliza_cache(process, true);  // HIT
    }

    // Agora que o dado está na cache, atualiza e marca como "dirty"
    L1_cache->update(address, data);
    process.cache_mem_accesses.fetch_add(1);
    process.memory_cycles.fetch_add(process.memWeights.cache);
}

// Função chamada pela cache para escrever dados "sujos" de volta na memória
void MemoryManager::writeToFile(uint32_t address, uint32_t data) {
    if (address < mainMemoryLimit) {
        mainMemory->WriteMem(address, data);
    } else {
        uint32_t secondaryAddress = address - mainMemoryLimit;
        secondaryMemory->WriteMem(secondaryAddress, data);
    }
}

void MemoryManager::resetCache() {
    L1_cache->reset();
}

void MemoryManager::simulateContextSwitch() {
    // Durante um context switch, parte da cache é invalidada (cache pollution)
    // Context switches causam:
    // - FCFS: menos switches (menos pollution) -> melhor cache
    // - SJN: switches médios -> cache média
    // - Priority/RR: mais switches (mais pollution) -> pior cache
    
    // IMPORTANTE: Não invalida TODA a cache (muito agressivo)
    // Em sistemas reais, apenas parte da cache é poluída:
    // 1. TLB é esvaziada (não simulado)
    // 2. Linhas são gradualmente substituídas pelo novo processo
    // 3. Cache de instruções vs dados tem comportamento diferente
    
    // Invalidação PARCIAL (30% da cache) - mais realista
    // Isso permite que:
    // - Escalonadores com menos switches mantenham mais cache quente
    // - Multi-core tenha vantagem por menos contenção
    L1_cache->invalidatePartial(0.3f);  // Invalida 30% da cache
}

void MemoryManager::simulateContextSwitchLight() {
    // Context switch sem preempção (ex: FCFS puro)
    // Quase não polui a cache, apenas marca algumas entradas como menos recentes
    L1_cache->invalidatePartial(0.1f);  // Invalida apenas 10% da cache
}

void MemoryManager::setCachePolicy(ReplacementPolicy policy) {
    if (L1_cache) {
        L1_cache->setPolicy(policy);
    }
}

ReplacementPolicy MemoryManager::getCachePolicy() const {
    return L1_cache ? L1_cache->getPolicy() : ReplacementPolicy::FIFO;
}
#include "cache.hpp"
#include "cachePolicy.hpp"
#include "MemoryManager.hpp" // Necessário para a lógica de write-back

Cache::Cache(ReplacementPolicy p) : policy(p) {
    this->capacity = CACHE_CAPACITY;
    this->cacheMap.reserve(CACHE_CAPACITY);
    this->cache_misses = 0;
    this->cache_hits = 0;
}

Cache::~Cache() {
    this->cacheMap.clear();
}

size_t Cache::get(size_t address) {
    if (cacheMap.count(address) > 0 && cacheMap[address].isValid) {
        cache_hits++;
        
        // Se estamos usando LRU, atualizar a ordem de acesso
        if (policy == ReplacementPolicy::LRU) {
            CachePolicy cachepolicy(policy);
            cachepolicy.updateLRU(lru_list, address);
        }
        
        return cacheMap[address].data; // Cache hit
    }

    cache_misses++;
    return CACHE_MISS; // Cache miss
}

void Cache::put(size_t address, size_t data, MemoryManager* memManager) {
    // Se a cache está cheia, precisamos remover um item
    if (cacheMap.size() >= capacity) {
        CachePolicy cachepolicy(policy);
        size_t addr_to_remove = -1;
        
        // Escolher política de substituição
        if (policy == ReplacementPolicy::FIFO) {
            addr_to_remove = cachepolicy.getAddressToReplaceFIFO(fifo_queue);
        } else if (policy == ReplacementPolicy::LRU) {
            addr_to_remove = cachepolicy.getAddressToReplaceLRU(lru_list);
        }

        if (addr_to_remove != static_cast<size_t>(-1)) {
            CacheEntry& entry_to_remove = cacheMap[addr_to_remove];

            // Lógica de WRITE-BACK: se o bloco a ser removido estiver sujo...
            if (entry_to_remove.isDirty) {
                memManager->writeToFile(addr_to_remove, entry_to_remove.data);
            }
            // Remove da cache
            cacheMap.erase(addr_to_remove);
        }
    }

    // Adiciona o novo item na cache
    CacheEntry new_entry;
    new_entry.data = data;
    new_entry.isValid = true;
    new_entry.isDirty = false; // Começa como "limpo"

    cacheMap[address] = new_entry;
    
    // Adiciona na estrutura de controle apropriada
    if (policy == ReplacementPolicy::FIFO) {
        fifo_queue.push(address);
    } else if (policy == ReplacementPolicy::LRU) {
        CachePolicy cachepolicy(policy);
        cachepolicy.updateLRU(lru_list, address);
    }
}

void Cache::update(size_t address, size_t data) {
    // Se o item não está na cache, primeiro o colocamos lá
    if (cacheMap.find(address) == cacheMap.end()) {
        // Para a simplicidade, assumimos que o `put` deve ser chamado pelo `MemoryManager`
        // em um cache miss de escrita. Aqui, focamos em atualizar.
        // Em um sistema real, aqui ocorreria um "write-allocate".
        // Por ora, vamos apenas atualizar se existir.
        return;
    }
    
    cacheMap[address].data = data;
    cacheMap[address].isDirty = true; // Marca como sujo
    cacheMap[address].isValid = true;
}

void Cache::invalidate() {
    for (auto &c : cacheMap) {
        c.second.isValid = false;
    }
    // Limpar as estruturas de controle
    std::queue<size_t> empty_queue;
    fifo_queue.swap(empty_queue);
    lru_list.clear();
}

void Cache::invalidatePartial(float percentage) {
    // Invalida apenas uma porcentagem da cache (cache pollution parcial)
    // Mais realista que invalidar tudo durante context switch
    
    if (percentage <= 0.0f) return;
    if (percentage >= 1.0f) {
        invalidate();  // Se >= 100%, invalida tudo
        return;
    }
    
    int entries_to_invalidate = static_cast<int>(cacheMap.size() * percentage);
    int count = 0;
    
    // Invalida as primeiras N entradas (simula que o novo processo sobrescreve parte da cache)
    for (auto &entry : cacheMap) {
        if (count >= entries_to_invalidate) break;
        entry.second.isValid = false;
        count++;
    }
    
    // Não limpamos as estruturas de controle completamente
    // Apenas removemos parte dos elementos das filas
    if (policy == ReplacementPolicy::FIFO) {
        std::queue<size_t> new_queue;
        int remaining = fifo_queue.size() - entries_to_invalidate;
        while (remaining > 0 && !fifo_queue.empty()) {
            new_queue.push(fifo_queue.front());
            fifo_queue.pop();
            remaining--;
        }
        fifo_queue = new_queue;
    } else if (policy == ReplacementPolicy::LRU) {
        while (entries_to_invalidate > 0 && !lru_list.empty()) {
            lru_list.pop_front();  // Remove os menos recentemente usados
            entries_to_invalidate--;
        }
    }
}

void Cache::reset() {
    // Limpa completamente a cache (dados + estatísticas)
    cacheMap.clear();
    std::queue<size_t> empty_queue;
    fifo_queue.swap(empty_queue);
    lru_list.clear();
    cache_hits = 0;
    cache_misses = 0;
}

std::vector<std::pair<size_t, size_t>> Cache::dirtyData() {
    std::vector<std::pair<size_t, size_t>> dirty_data;
    for (const auto &c : cacheMap) {
        if (c.second.isDirty) {
            dirty_data.emplace_back(c.first, c.second.data);
        }
    }
    return dirty_data;
}

int Cache::get_misses(){
       // Retorna o número de cache misses
    return cache_misses;
}
int Cache::get_hits(){
       // Retorna o número de cache hits
    return cache_hits;
}
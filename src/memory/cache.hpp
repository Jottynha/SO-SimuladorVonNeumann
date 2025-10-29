#ifndef CACHE_HPP
#define CACHE_HPP

#include <cstdint>
#include <cstddef> 
#include <unordered_map>
#include <vector>
#include <queue>
#include <list>

#define CACHE_CAPACITY 16
#define CACHE_MISS UINT32_MAX

struct CacheEntry {
    size_t data;
    bool isValid;
    bool isDirty;
};

// Forward declaration
class MemoryManager;
enum class ReplacementPolicy;

class Cache {
private:
    std::unordered_map<size_t, CacheEntry> cacheMap;
    std::queue<size_t> fifo_queue; // Fila para controlar a ordem de entrada (FIFO)
    std::list<size_t> lru_list;    // Lista para controlar acessos (LRU)
    ReplacementPolicy policy;      // Política de substituição atual
    size_t capacity;
    int cache_misses;
    int cache_hits;

public:
    Cache(ReplacementPolicy p = static_cast<ReplacementPolicy>(0)); // FIFO por padrão
    ~Cache();
    int get_misses();
    int get_hits();
    size_t get(size_t address);
    // O método put agora precisa interagir com o MemoryManager para o write-back
    void put(size_t address, size_t data, MemoryManager* memManager);
    void update(size_t address, size_t data);
    void invalidate();
    void reset(); // Reseta completamente a cache (dados + estatísticas)
    std::vector<std::pair<size_t, size_t>> dirtyData(); // Mantido para possíveis outras lógicas
    
    // Métodos para trocar política em tempo de execução
    void setPolicy(ReplacementPolicy p) { policy = p; }
    ReplacementPolicy getPolicy() const { return policy; }
};

#endif
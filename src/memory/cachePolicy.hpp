#ifndef CACHE_POLICY_HPP
#define CACHE_POLICY_HPP

#include "cache.hpp"
#include <queue>
#include <map>
#include <list>

// Enum para definir a política de substituição
enum class ReplacementPolicy {
    FIFO,  // First In, First Out
    LRU    // Least Recently Used
};

class CachePolicy {
private:
    ReplacementPolicy policy;

public:
    CachePolicy(ReplacementPolicy p = ReplacementPolicy::FIFO);
    ~CachePolicy();

    // Retorna o endereço a ser substituído com base na política FIFO
    size_t getAddressToReplaceFIFO(std::queue<size_t>& fifo_queue);
    
    // Retorna o endereço a ser substituído com base na política LRU
    size_t getAddressToReplaceLRU(std::list<size_t>& lru_list);
    
    // Atualiza o acesso LRU (move para o final da lista)
    void updateLRU(std::list<size_t>& lru_list, size_t address);
    
    // Getter para a política atual
    ReplacementPolicy getPolicy() const { return policy; }
    void setPolicy(ReplacementPolicy p) { policy = p; }
};

#endif
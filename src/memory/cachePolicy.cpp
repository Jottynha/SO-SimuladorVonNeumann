#include "cachePolicy.hpp"

CachePolicy::CachePolicy(ReplacementPolicy p) : policy(p) {}

CachePolicy::~CachePolicy() {}

// Implementação da política FIFO
size_t CachePolicy::getAddressToReplaceFIFO(std::queue<size_t>& fifo_queue) {
    if (fifo_queue.empty()) {
        return -1; // Retorna -1 se não houver nada para remover
    }

    // Pega o primeiro endereço que entrou na fila
    size_t address_to_remove = fifo_queue.front();
    // Remove da fila
    fifo_queue.pop();

    return address_to_remove;
}

// Implementação da política LRU (Least Recently Used)
size_t CachePolicy::getAddressToReplaceLRU(std::list<size_t>& lru_list) {
    if (lru_list.empty()) {
        return -1; // Retorna -1 se não houver nada para remover
    }

    // Remove o primeiro da lista (menos recentemente usado)
    size_t address_to_remove = lru_list.front();
    lru_list.pop_front();

    return address_to_remove;
}

// Atualiza o acesso LRU - move endereço para o final (mais recente)
void CachePolicy::updateLRU(std::list<size_t>& lru_list, size_t address) {
    // Remove o endereço se já existir na lista
    lru_list.remove(address);
    // Adiciona no final (mais recentemente usado)
    lru_list.push_back(address);
}
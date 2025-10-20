#include "Scheduler.hpp"

// --- FCFS Policy ---
void FCFS_Policy::add(PCB* process) {
    ready_queue.push_back(process);
}

PCB* FCFS_Policy::get_next() {
    if (ready_queue.empty()) {
        return nullptr;
    }
    PCB* next_process = ready_queue.front();
    ready_queue.pop_front();
    return next_process;
}

bool FCFS_Policy::is_empty() const {
    return ready_queue.empty();
}

// --- SJN Policy ---
void SJN_Policy::add(PCB* process) {
    ready_queue.push_back(process);
    // Ordena a fila com base no número de instruções (menor primeiro)
    std::sort(ready_queue.begin(), ready_queue.end(), [](const PCB* a, const PCB* b) {
        return a->instruction_count < b->instruction_count;
    });
}

PCB* SJN_Policy::get_next() {
    if (ready_queue.empty()) {
        return nullptr;
    }
    PCB* next_process = ready_queue.front();
    ready_queue.erase(ready_queue.begin());
    return next_process;
}

bool SJN_Policy::is_empty() const {
    return ready_queue.empty();
}

// --- Priority Policy ---
void Priority_Policy::add(PCB* process) {
    ready_queue.push_back(process);
    // Ordena a fila com base na prioridade (menor valor = maior prioridade)
    std::sort(ready_queue.begin(), ready_queue.end(), [](const PCB* a, const PCB* b) {
        return a->priority < b->priority;
    });
}

PCB* Priority_Policy::get_next() {
    if (ready_queue.empty()) {
        return nullptr;
    }
    PCB* next_process = ready_queue.front();
    ready_queue.erase(ready_queue.begin());
    return next_process;
}

bool Priority_Policy::is_empty() const {
    return ready_queue.empty();
}

// --- Round Robin ---
void RoundRobin_Policy::add(PCB* process) {
    ready_queue.push_back(process);
}

PCB* RoundRobin_Policy::get_next() {
    if (ready_queue.empty()) {
        return nullptr;
    }
    // Round Robin: pega o primeiro da fila
    PCB* process = ready_queue.front();
    ready_queue.pop_front();
    return process;
}

bool RoundRobin_Policy::is_empty() const {
    return ready_queue.empty();
}

// --- Scheduler ---
Scheduler::Scheduler(SchedulerType type) {
    switch (type) {
        case SchedulerType::FCFS:
            policy = std::make_unique<FCFS_Policy>();
            break;
        case SchedulerType::SJN:
            policy = std::make_unique<SJN_Policy>();
            break;
        case SchedulerType::Priority:
            policy = std::make_unique<Priority_Policy>();
            break;
        case SchedulerType::RoundRobin:
            policy = std::make_unique<RoundRobin_Policy>();
            break;
    }
}

void Scheduler::add_process(PCB* process) {
    policy->add(process);
}

PCB* Scheduler::get_next_process() {
    return policy->get_next();
}

bool Scheduler::is_empty() const {
    return policy->is_empty();
}

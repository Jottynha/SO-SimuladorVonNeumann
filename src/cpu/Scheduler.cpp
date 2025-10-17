#include "Scheduler.hpp"
#include <thread>
#include <chrono>
#include "CONTROL_UNIT.hpp"
#include "memory/MemoryManager.hpp"
#include "IO/IOManager.hpp"

extern MemoryManager memManager;
extern IOManager ioManager;
extern void print_metrics(const PCB& pcb);

// --- FCFS_Policy ---
void FCFS_Policy::add_process(PCB* process) {
    ready_queue.push_back(process);
}

PCB* FCFS_Policy::get_next_process() {
    if (is_empty()) return nullptr;
    PCB* next = ready_queue.front();
    ready_queue.pop_front();
    return next;
}

bool FCFS_Policy::is_empty() const {
    return ready_queue.empty();
}

// --- SJN_Policy ---
void SJN_Policy::sort_processes() {
    // Ordena a lista de processos com base no número de instruções (menor primeiro)
    std::sort(process_list.begin(), process_list.end(), [](const PCB* a, const PCB* b) {
        return a->instruction_count < b->instruction_count;
    });
}

void SJN_Policy::add_process(PCB* process) {
    process_list.push_back(process);
    sort_processes(); // Reordena sempre que um novo processo é adicionado
}

PCB* SJN_Policy::get_next_process() {
    if (is_empty()) return nullptr;
    PCB* next = process_list.front();
    process_list.erase(process_list.begin());
    return next;
}

bool SJN_Policy::is_empty() const {
    return process_list.empty();
}

// --- Priority_Policy ---
void Priority_Policy::sort_queue() {
    // Ordena a fila de prontos com base na prioridade (menor valor = maior prioridade)
    std::sort(ready_queue.begin(), ready_queue.end(), [](const PCB* a, const PCB* b) {
        return a->priority < b->priority;
    });
}

void Priority_Policy::add_process(PCB* process) {
    ready_queue.push_back(process);
    sort_queue(); // Reordena a fila sempre que um novo processo é adicionado
}

PCB* Priority_Policy::get_next_process() {
    if (is_empty()) return nullptr;
    PCB* next = ready_queue.front();
    ready_queue.erase(ready_queue.begin());
    return next;
}

bool Priority_Policy::is_empty() const {
    return ready_queue.empty();
}

// --- RR_Policy ---
void RR_Policy::add_process(PCB* process) {
    ready_queue.push_back(process);
}

PCB* RR_Policy::get_next_process() {
    if (is_empty()) return nullptr;
    PCB* next = ready_queue.front();
    ready_queue.pop_front();
    return next;
}

bool RR_Policy::is_empty() const {
    return ready_queue.empty();
}

// --- Scheduler ---
Scheduler::Scheduler(SchedulingPolicyType policy_type, std::vector<std::unique_ptr<PCB>>& process_list_owner) {
    switch (policy_type) {
        case SchedulingPolicyType::FCFS:
            policy = std::make_unique<FCFS_Policy>();
            break;
        case SchedulingPolicyType::SJN:
            policy = std::make_unique<SJN_Policy>();
            break;
        case SchedulingPolicyType::RR:
            policy = std::make_unique<RR_Policy>();
            break;
        case SchedulingPolicyType::Priority:
            policy = std::make_unique<Priority_Policy>();
            break;
    }

    total_processes = process_list_owner.size();
    finished_processes = 0;
    for (const auto& p : process_list_owner) {
        policy->add_process(p.get());
    }
}

void Scheduler::add_to_ready_queue(PCB* process) {
    policy->add_process(process);
}

PCB* Scheduler::get_next_process() {
    return policy->get_next_process();
}

void Scheduler::add_to_blocked_list(PCB* process) {
    blocked_list.push_back(process);
}

bool Scheduler::has_blocked_processes() const {
    return !blocked_list.empty();
}

void Scheduler::check_blocked_processes() {
    for (auto it = blocked_list.begin(); it != blocked_list.end(); ) {
        if ((*it)->state == State::Ready) {
            std::cout << "[Scheduler] Processo " << (*it)->pid << " desbloqueado e movido para a fila de prontos.\n";
            add_to_ready_queue(*it);
            it = blocked_list.erase(it);
        } else {
            ++it;
        }
    }
}

bool Scheduler::has_finished() const {
    return finished_processes >= total_processes;
}

void Scheduler::increment_finished_processes() {
    finished_processes++;
}

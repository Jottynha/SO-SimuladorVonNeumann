#ifndef SCHEDULER_HPP
#define SCHEDULER_HPP

#include <deque>
#include <vector>
#include <memory>
#include <iostream>
#include <algorithm>
#include "cpu/PCB.hpp"

// Enum para identificar as políticas de escalonamento
enum class SchedulingPolicyType {
    FCFS,
    SJN,
    RR,
    Priority // Adicionando a nova política de Prioridade
};

// Interface para as políticas de escalonamento
class SchedulingPolicy {
public:
    virtual ~SchedulingPolicy() = default;
    virtual void add_process(PCB* process) = 0;
    virtual PCB* get_next_process() = 0;
    virtual bool is_empty() const = 0;
    virtual void print_type() const = 0;
    virtual void print_type_to_stream(std::ostream& os) const = 0;
    virtual std::string get_type_as_string() const = 0;
};

// Implementação do FCFS (First-Come, First-Served)
class FCFS_Policy : public SchedulingPolicy {
private:
    std::deque<PCB*> ready_queue;
public:
    void add_process(PCB* process) override;
    PCB* get_next_process() override;
    bool is_empty() const override;
    void print_type() const override { std::cout << "FCFS"; }
    void print_type_to_stream(std::ostream& os) const override { os << "FCFS"; }
    std::string get_type_as_string() const override { return "FCFS"; }
};

// Implementação do SJN (Shortest Job Next) - Não Preemptivo
class SJN_Policy : public SchedulingPolicy {
private:
    std::vector<PCB*> process_list;
    void sort_processes();
public:
    void add_process(PCB* process) override;
    PCB* get_next_process() override;
    bool is_empty() const override;
    void print_type() const override { std::cout << "SJN"; }
    void print_type_to_stream(std::ostream& os) const override { os << "SJN"; }
    std::string get_type_as_string() const override { return "SJN"; }
};

// Nova política de escalonamento por prioridade
class Priority_Policy : public SchedulingPolicy {
private:
    std::vector<PCB*> ready_queue;
    void sort_queue();
public:
    void add_process(PCB* process) override;
    PCB* get_next_process() override;
    bool is_empty() const override;
    void print_type() const override { std::cout << "Priority"; }
    void print_type_to_stream(std::ostream& os) const override { os << "Priority"; }
    std::string get_type_as_string() const override { return "Priority"; }
};

// Implementação do Round Robin (RR)
class RR_Policy : public SchedulingPolicy {
private:
    std::deque<PCB*> ready_queue;
public:
    void add_process(PCB* process) override;
    PCB* get_next_process() override;
    bool is_empty() const override;
    void print_type() const override { std::cout << "Round Robin"; }
    void print_type_to_stream(std::ostream& os) const override { os << "Round Robin"; }
    std::string get_type_as_string() const override { return "Round Robin"; }
};


// Classe principal do Escalonador
class Scheduler {
private:
    std::unique_ptr<SchedulingPolicy> policy;
    std::vector<PCB*> blocked_list;
    int total_processes = 0;
    int finished_processes = 0;

public:
    Scheduler(SchedulingPolicyType policy_type, std::vector<std::unique_ptr<PCB>>& process_list);
    void run();
    void add_to_ready_queue(PCB* process);
    PCB* get_next_process();
    void add_to_blocked_list(PCB* process);
    void check_blocked_processes();
    bool has_finished() const;
    bool has_blocked_processes() const; // Declaração da nova função
    void increment_finished_processes();
    SchedulingPolicy* get_policy() const { return policy.get(); }
};

#endif // SCHEDULER_HPP

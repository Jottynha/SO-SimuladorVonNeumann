#ifndef SCHEDULER_HPP
#define SCHEDULER_HPP

#include <vector>
#include <deque>
#include <memory>
#include <algorithm>
#include "PCB.hpp"

// Enum para os tipos de escalonadores
enum class SchedulerType {
    FCFS,
    SJN,
    Priority
};

// Interface para as políticas de escalonamento
class SchedulingPolicy {
public:
    virtual ~SchedulingPolicy() = default;
    virtual void add(PCB* process) = 0;
    virtual PCB* get_next() = 0;
    virtual bool is_empty() const = 0;
};

// Implementação do FCFS
class FCFS_Policy : public SchedulingPolicy {
private:
    std::deque<PCB*> ready_queue;
public:
    void add(PCB* process) override;
    PCB* get_next() override;
    bool is_empty() const override;
};

// Implementação do SJN
class SJN_Policy : public SchedulingPolicy {
private:
    std::vector<PCB*> ready_queue;
public:
    void add(PCB* process) override;
    PCB* get_next() override;
    bool is_empty() const override;
};

// Implementação do Priority
class Priority_Policy : public SchedulingPolicy {
private:
    std::vector<PCB*> ready_queue;
public:
    void add(PCB* process) override;
    PCB* get_next() override;
    bool is_empty() const override;
};

// Classe principal do Escalonador
class Scheduler {
private:
    std::unique_ptr<SchedulingPolicy> policy;
public:
    Scheduler(SchedulerType type);
    void add_process(PCB* process);
    PCB* get_next_process();
    bool is_empty() const;
};

#endif // SCHEDULER_HPP

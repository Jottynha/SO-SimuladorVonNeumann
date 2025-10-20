#ifndef MULTICORE_HPP
#define MULTICORE_HPP

#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include "PCB.hpp"
#include "Scheduler.hpp"
#include "../memory/MemoryManager.hpp"
#include "../IO/IOManager.hpp"

// Estrutura para representar um núcleo de processamento
class Core {
private:
    int core_id;
    std::atomic<bool> is_busy{false};
    std::atomic<uint64_t> instructions_executed{0};
    std::atomic<uint64_t> idle_cycles{0};
    std::atomic<uint64_t> busy_cycles{0};
    PCB* current_process = nullptr;

public:
    Core(int id) : core_id(id) {}
    
    int get_id() const { return core_id; }
    bool is_running() const { return is_busy.load(); }
    void set_busy(bool busy) { is_busy.store(busy); }
    
    PCB* get_current_process() { return current_process; }
    void set_current_process(PCB* process) { current_process = process; }
    
    uint64_t get_instructions_executed() const { return instructions_executed.load(); }
    void increment_instructions(uint64_t count = 1) { instructions_executed.fetch_add(count); }
    
    uint64_t get_idle_cycles() const { return idle_cycles.load(); }
    void increment_idle_cycles(uint64_t count = 1) { idle_cycles.fetch_add(count); }
    
    uint64_t get_busy_cycles() const { return busy_cycles.load(); }
    void increment_busy_cycles(uint64_t count = 1) { busy_cycles.fetch_add(count); }
    
    double get_utilization() const {
        uint64_t total = busy_cycles.load() + idle_cycles.load();
        return (total > 0) ? (100.0 * busy_cycles.load() / total) : 0.0;
    }
};

// Estrutura para métricas multicore
struct MulticoreMetrics {
    int num_cores = 1;
    uint64_t total_execution_time_ms = 0;
    double avg_cpu_utilization = 0.0;
    std::vector<double> per_core_utilization;
    double throughput = 0.0;
    double speedup = 0.0; // Speedup em relação ao single-core
};

#endif // MULTICORE_HPP

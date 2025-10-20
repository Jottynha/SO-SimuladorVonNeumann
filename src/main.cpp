#include <iostream>
#include <vector>
#include <deque>
#include <memory>
#include <thread>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <ctime>
#include <sys/stat.h>
#include <sys/types.h>


#include "cpu/PCB.hpp"
#include "cpu/pcb_loader.hpp"
#include "cpu/CONTROL_UNIT.hpp"
#include "memory/MemoryManager.hpp"
#include "parser_json/parser_json.hpp"
#include "IO/IOManager.hpp"
#include "cpu/Scheduler.hpp"

// Função auxiliar para criar diretório
inline void create_output_directory() {
    mkdir("output", 0755);
}

// Estrutura para armazenar métricas agregadas
struct SchedulerMetrics {
    std::string name = "";
    double execution_time_ms = 0.0;
    uint64_t total_pipeline_cycles = 0;
    uint64_t total_memory_accesses = 0;
    uint64_t total_cache_hits = 0;
    uint64_t total_cache_misses = 0;
    double cache_hit_rate = 0.0;
    int processes_finished = 0;
    int context_switches = 0;
    
    // Métricas adicionais
    uint64_t total_memory_reads = 0;
    uint64_t total_memory_writes = 0;
    uint64_t total_cache_accesses = 0;
    uint64_t total_primary_mem_accesses = 0;
    uint64_t total_secondary_mem_accesses = 0;
    uint64_t total_memory_cycles = 0;
    double avg_memory_cycles_per_access = 0.0;
    double throughput = 0.0; // processos/segundo
    
    // Métricas de escalonamento (Requisitos do PDF)
    double avg_wait_time_ms = 0.0;          // Tempo médio de espera
    double avg_turnaround_time_ms = 0.0;    // Tempo médio de retorno
    double avg_response_time_ms = 0.0;      // Tempo médio de resposta
    double avg_cpu_utilization = 0.0;       // Utilização média da CPU
    double efficiency = 0.0;                 // Eficiência global
    
    // Multicore
    int num_cores = 1;
    std::vector<double> per_core_utilization;
};

// Função para imprimir as métricas de um processo
void print_metrics(const PCB& pcb, std::ofstream& outFile) {
    outFile << "\n--- METRICAS FINAIS DO PROCESSO " << pcb.pid << " ---\n";
    outFile << "Nome do Processo:       " << pcb.name << "\n";
    outFile << "Estado Final:           " << (pcb.state == State::Finished ? "Finished" : "Incomplete") << "\n";
    outFile << "Prioridade:             " << pcb.priority << "\n";
    outFile << "Quantum:                " << pcb.quantum << "\n";
    
    // Métricas de Tempo
    outFile << "\n--- METRICAS DE TEMPO ---\n";
    outFile << "Tempo de Espera:        " << pcb.wait_time_ms << " ms\n";
    outFile << "Tempo de Resposta:      " << pcb.response_time_ms << " ms\n";
    outFile << "Tempo de Retorno:       " << pcb.turnaround_time_ms << " ms\n";
    
    // Métricas de CPU e Memória
    outFile << "\n--- METRICAS DE CPU E MEMORIA ---\n";
    outFile << "Ciclos de Pipeline:     " << pcb.pipeline_cycles.load() << "\n";
    outFile << "Total de Acessos a Mem: " << pcb.mem_accesses_total.load() << "\n";
    outFile << "  - Leituras:             " << pcb.mem_reads.load() << "\n";
    outFile << "  - Escritas:             " << pcb.mem_writes.load() << "\n";
    outFile << "Acessos a Cache L1:     " << pcb.cache_mem_accesses.load() << "\n";
    outFile << "Acessos a Mem Principal:" << pcb.primary_mem_accesses.load() << "\n";
    outFile << "Acessos a Mem Secundaria:" << pcb.secondary_mem_accesses.load() << "\n";
    outFile << "Ciclos Totais de Memoria: " << pcb.memory_cycles.load() << "\n";
    outFile << "Cache Hits:             " << pcb.cache_hits.load() << "\n";
    outFile << "Cache Misses:           " << pcb.cache_misses.load() << "\n";
    outFile << "Ciclos de IO:             " << pcb.io_cycles.load() << "\n";
    
    outFile << "\n--- LOG DE EXECUCAO ---\n";
    for (const auto& log_entry : pcb.execution_log) {
        outFile << log_entry << "\n";
    }
    
    outFile << "------------------------------------------\n";
}


// Função auxiliar para carregar processos
std::vector<std::unique_ptr<PCB>> load_processes(MemoryManager& memManager) {
    std::vector<std::unique_ptr<PCB>> process_list;
    
    // Processo 1: Quick
    auto p1 = std::make_unique<PCB>();
    if (load_pcb_from_json("processes/process_quick.json", *p1)) {
        loadJsonProgram("tasks/tasks_quick.json", memManager, *p1, 0);
        process_list.push_back(std::move(p1));
    }

    // Processo 2: Short
    auto p2 = std::make_unique<PCB>();
    if (load_pcb_from_json("processes/process_short.json", *p2)) {
        loadJsonProgram("tasks/tasks_short.json", memManager, *p2, 1024);
        process_list.push_back(std::move(p2));
    }

    // Processo 3: Medium
    auto p3 = std::make_unique<PCB>();
    if (load_pcb_from_json("processes/process_medium.json", *p3)) {
        loadJsonProgram("tasks/tasks_medium.json", memManager, *p3, 2048);
        process_list.push_back(std::move(p3));
    }

    // Processo 4: Long
    auto p4 = std::make_unique<PCB>();
    if (load_pcb_from_json("processes/process_long.json", *p4)) {
        loadJsonProgram("tasks/tasks_long.json", memManager, *p4, 3072);
        process_list.push_back(std::move(p4));
    }

    // Processo 5: CPU-Bound
    auto p5 = std::make_unique<PCB>();
    if (load_pcb_from_json("processes/process_cpu_bound.json", *p5)) {
        loadJsonProgram("tasks/tasks_cpu_bound.json", memManager, *p5, 4096);
        process_list.push_back(std::move(p5));
    }

    // Processo 6: IO-Bound
    auto p6 = std::make_unique<PCB>();
    if (load_pcb_from_json("processes/process_io_bound.json", *p6)) {
        loadJsonProgram("tasks/tasks_io_bound.json", memManager, *p6, 5120);
        process_list.push_back(std::move(p6));
    }

    // Processo 7: Memory-Intensive
    auto p7 = std::make_unique<PCB>();
    if (load_pcb_from_json("processes/process_memory_intensive.json", *p7)) {
        loadJsonProgram("tasks/tasks_memory_intensive.json", memManager, *p7, 6144);
        process_list.push_back(std::move(p7));
    }

    // Processo 8: Balanced
    auto p8 = std::make_unique<PCB>();
    if (load_pcb_from_json("processes/process_balanced.json", *p8)) {
        loadJsonProgram("tasks/tasks_balanced.json", memManager, *p8, 7168);
        process_list.push_back(std::move(p8));
    }
    
    // Registrar tempo de chegada de todos os processos
    auto arrival = std::chrono::high_resolution_clock::now();
    for (auto& proc : process_list) {
        proc->arrival_time = arrival;
    }
    
    return process_list;
}

// Função para executar um escalonador e retornar suas métricas
SchedulerMetrics run_scheduler(SchedulerType scheduler_type, const std::string& scheduler_name, bool save_logs = false) {
    SchedulerMetrics metrics;
    metrics.name = scheduler_name;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    MemoryManager memManager(8192, 16384);
    // Reset cache para garantir execução limpa entre escalonadores
    memManager.resetCache();
    
    IOManager ioManager;
    Scheduler scheduler(scheduler_type);
    
    auto process_list = load_processes(memManager);
    
    for (const auto& process : process_list) {
        scheduler.add_process(process.get());
    }

    int total_processes = process_list.size();
    int finished_processes = 0;
    std::vector<PCB*> blocked_list;

    create_output_directory();
    std::ofstream results_file;
    if (save_logs) {
        results_file.open("output/resultados_" + scheduler_name + ".dat");
    }

    int max_iterations = 10000;
    int iteration_count = 0;
    
    while (finished_processes < total_processes && iteration_count < max_iterations) {
        iteration_count++;
        
        for (auto it = blocked_list.begin(); it != blocked_list.end(); ) {
            if ((*it)->state == State::Ready) {
                scheduler.add_process(*it);
                it = blocked_list.erase(it);
            } else {
                ++it;
            }
        }

        PCB* current_process = scheduler.get_next_process();

        if (!current_process) {
            if (blocked_list.empty() && scheduler.is_empty()) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        // Registrar tempo de início na primeira execução (tempo de resposta)
        if (current_process->first_run) {
            current_process->start_time = std::chrono::high_resolution_clock::now();
            current_process->first_run = false;
        }

        current_process->state = State::Running;

        std::vector<std::unique_ptr<IORequest>> io_requests;
        bool print_lock = true;

        int before_instr = current_process->instruction_count;
        size_t before_log = current_process->execution_log.size();

        Core(memManager, *current_process, &io_requests, print_lock);

        if (current_process->state == State::Blocked) {
            ioManager.registerProcessWaitingForIO(current_process);
            blocked_list.push_back(current_process);
        } else if (current_process->state == State::Finished) {
            // Registrar tempo de término
            current_process->finish_time = std::chrono::high_resolution_clock::now();
            
            // Calcular métricas de tempo
            auto wait_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                current_process->start_time - current_process->arrival_time);
            current_process->wait_time_ms = wait_duration.count();
            
            auto turnaround_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                current_process->finish_time - current_process->arrival_time);
            current_process->turnaround_time_ms = turnaround_duration.count();
            
            auto response_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                current_process->start_time - current_process->arrival_time);
            current_process->response_time_ms = response_duration.count();
            
            if (save_logs) {
                print_metrics(*current_process, results_file);
            }
            
            // Acumular métricas
            metrics.total_pipeline_cycles += current_process->pipeline_cycles.load();
            metrics.total_memory_accesses += current_process->mem_accesses_total.load();
            metrics.total_cache_hits += current_process->cache_hits.load();
            metrics.total_cache_misses += current_process->cache_misses.load();
            metrics.total_memory_reads += current_process->mem_reads.load();
            metrics.total_memory_writes += current_process->mem_writes.load();
            metrics.total_cache_accesses += current_process->cache_mem_accesses.load();
            metrics.total_primary_mem_accesses += current_process->primary_mem_accesses.load();
            metrics.total_secondary_mem_accesses += current_process->secondary_mem_accesses.load();
            metrics.total_memory_cycles += current_process->memory_cycles.load();
            
            // Acumular métricas de tempo
            metrics.avg_wait_time_ms += current_process->wait_time_ms;
            metrics.avg_turnaround_time_ms += current_process->turnaround_time_ms;
            metrics.avg_response_time_ms += current_process->response_time_ms;
            
            finished_processes++;
        } else {
            bool progressed = (current_process->instruction_count > before_instr) || 
                            (current_process->execution_log.size() > before_log);
            if (progressed) {
                current_process->stagnation_counter = 0;
            } else {
                current_process->stagnation_counter++;
                if (current_process->stagnation_counter >= 5) {
                    if (save_logs) {
                        print_metrics(*current_process, results_file);
                    }
                    // Registrar tempo de término para processos estagnados
                    current_process->finish_time = std::chrono::high_resolution_clock::now();
                    
                    // Calcular métricas mesmo para processos estagnados
                    if (!current_process->first_run) {
                        auto wait_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                            current_process->start_time - current_process->arrival_time);
                        current_process->wait_time_ms = wait_duration.count();
                        
                        auto turnaround_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                            current_process->finish_time - current_process->arrival_time);
                        current_process->turnaround_time_ms = turnaround_duration.count();
                        
                        auto response_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                            current_process->start_time - current_process->arrival_time);
                        current_process->response_time_ms = response_duration.count();
                        
                        // Acumular métricas de tempo
                        metrics.avg_wait_time_ms += current_process->wait_time_ms;
                        metrics.avg_turnaround_time_ms += current_process->turnaround_time_ms;
                        metrics.avg_response_time_ms += current_process->response_time_ms;
                    }
                    
                    current_process->state = State::Finished;
                    finished_processes++;
                    continue;
                }
            }
            current_process->state = State::Ready;
            
            // Simular cache pollution durante context switch
            // Quanto mais processos, mais a cache é "poluída"
            memManager.simulateContextSwitch();
            scheduler.increment_context_switch();
            
            scheduler.add_process(current_process);
        }
    }

    if (save_logs && results_file.is_open()) {
        results_file.close();
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    metrics.execution_time_ms = duration.count() / 1000.0;
    metrics.processes_finished = finished_processes;
    metrics.context_switches = scheduler.get_context_switch_count();
    
    // Calcular métricas derivadas
    uint64_t total_cache_accesses = metrics.total_cache_hits + metrics.total_cache_misses;
    metrics.cache_hit_rate = (total_cache_accesses > 0) ? 
        (100.0 * metrics.total_cache_hits / total_cache_accesses) : 0.0;
    
    metrics.avg_memory_cycles_per_access = (metrics.total_memory_accesses > 0) ?
        (static_cast<double>(metrics.total_memory_cycles) / metrics.total_memory_accesses) : 0.0;
    
    // Throughput: processos finalizados por segundo
    metrics.throughput = (metrics.execution_time_ms > 0) ?
        (finished_processes * 1000.0 / metrics.execution_time_ms) : 0.0;
    
    // Calcular médias das métricas de escalonamento
    if (finished_processes > 0) {
        metrics.avg_wait_time_ms /= finished_processes;
        metrics.avg_turnaround_time_ms /= finished_processes;
        metrics.avg_response_time_ms /= finished_processes;
        
        // Utilização da CPU: tempo de CPU sobre tempo total
        // Aproximação: ciclos de pipeline / (execution_time_ms * clock_frequency_estimate)
        // Para simplificar, usamos a proporção de ciclos ativos
        uint64_t total_cycles = metrics.total_pipeline_cycles + (finished_processes * 100); // overhead estimado
        metrics.avg_cpu_utilization = (total_cycles > 0) ? 
            (100.0 * metrics.total_pipeline_cycles / total_cycles) : 0.0;
        
        // Eficiência: throughput / contextos de troca (quanto maior, melhor)
        metrics.efficiency = (metrics.context_switches > 0) ?
            (metrics.throughput / metrics.context_switches) : metrics.throughput;
    }
    
    return metrics;
}

// Função para executar escalonador multicore
SchedulerMetrics run_multicore_scheduler(int num_cores, SchedulerType scheduler_type, 
                                         const std::string& scheduler_name, bool save_logs = false) {
    SchedulerMetrics metrics;
    metrics.name = scheduler_name;
    metrics.num_cores = num_cores;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Gerenciadores compartilhados
    MemoryManager memManager(8192, 16384);
    memManager.resetCache();
    IOManager ioManager;
    Scheduler scheduler(scheduler_type);
    
    // Carregar processos
    auto process_list = load_processes(memManager);
    
    for (const auto& process : process_list) {
        scheduler.add_process(process.get());
    }

    int total_processes = process_list.size();
    std::atomic<int> finished_processes{0};
    std::vector<PCB*> blocked_list;
    std::mutex blocked_mutex;
    std::mutex scheduler_mutex;
    std::mutex metrics_mutex;
    
    create_output_directory();
    std::ofstream results_file;
    if (save_logs) {
        results_file.open("output/resultados_" + scheduler_name + "_multicore.dat");
    }
    
    // Estrutura para métricas por núcleo
    struct CoreMetrics {
        std::atomic<uint64_t> busy_cycles{0};
        std::atomic<uint64_t> idle_cycles{0};
    };
    std::vector<CoreMetrics> core_metrics(num_cores);
    
    // Flag para controlar execução
    std::atomic<bool> should_stop{false};
    
    // Função executada por cada núcleo
    auto core_function = [&](int core_id) {
        while (!should_stop.load() && finished_processes.load() < total_processes) {
            // Verificar processos bloqueados
            {
                std::lock_guard<std::mutex> lock(blocked_mutex);
                for (auto it = blocked_list.begin(); it != blocked_list.end(); ) {
                    if ((*it)->state == State::Ready) {
                        std::lock_guard<std::mutex> sched_lock(scheduler_mutex);
                        scheduler.add_process(*it);
                        it = blocked_list.erase(it);
                    } else {
                        ++it;
                    }
                }
            }
            
            // Obter próximo processo
            PCB* current_process = nullptr;
            {
                std::lock_guard<std::mutex> sched_lock(scheduler_mutex);
                current_process = scheduler.get_next_process();
            }
            
            if (!current_process) {
                core_metrics[core_id].idle_cycles.fetch_add(10);
                std::this_thread::sleep_for(std::chrono::microseconds(100));
                continue;
            }
            
            // Registrar tempo de início na primeira execução
            if (current_process->first_run) {
                current_process->start_time = std::chrono::high_resolution_clock::now();
                current_process->first_run = false;
            }
            
            current_process->state = State::Running;
            core_metrics[core_id].busy_cycles.fetch_add(1);
            
            std::vector<std::unique_ptr<IORequest>> io_requests;
            bool print_lock = true;
            
            int before_instr = current_process->instruction_count;
            size_t before_log = current_process->execution_log.size();
            
            // Executar processo
            Core(memManager, *current_process, &io_requests, print_lock);
            
            // Processar resultado
            if (current_process->state == State::Blocked) {
                std::lock_guard<std::mutex> lock(blocked_mutex);
                ioManager.registerProcessWaitingForIO(current_process);
                blocked_list.push_back(current_process);
            } else if (current_process->state == State::Finished) {
                // Registrar tempo de término
                current_process->finish_time = std::chrono::high_resolution_clock::now();
                
                // Calcular métricas de tempo
                auto wait_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                    current_process->start_time - current_process->arrival_time);
                current_process->wait_time_ms = wait_duration.count();
                
                auto turnaround_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                    current_process->finish_time - current_process->arrival_time);
                current_process->turnaround_time_ms = turnaround_duration.count();
                
                auto response_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                    current_process->start_time - current_process->arrival_time);
                current_process->response_time_ms = response_duration.count();
                
                // Acumular métricas (thread-safe)
                {
                    std::lock_guard<std::mutex> lock(metrics_mutex);
                    if (save_logs) {
                        print_metrics(*current_process, results_file);
                    }
                    
                    metrics.total_pipeline_cycles += current_process->pipeline_cycles.load();
                    metrics.total_memory_accesses += current_process->mem_accesses_total.load();
                    metrics.total_cache_hits += current_process->cache_hits.load();
                    metrics.total_cache_misses += current_process->cache_misses.load();
                    metrics.total_memory_reads += current_process->mem_reads.load();
                    metrics.total_memory_writes += current_process->mem_writes.load();
                    metrics.total_cache_accesses += current_process->cache_mem_accesses.load();
                    metrics.total_primary_mem_accesses += current_process->primary_mem_accesses.load();
                    metrics.total_secondary_mem_accesses += current_process->secondary_mem_accesses.load();
                    metrics.total_memory_cycles += current_process->memory_cycles.load();
                    
                    metrics.avg_wait_time_ms += current_process->wait_time_ms;
                    metrics.avg_turnaround_time_ms += current_process->turnaround_time_ms;
                    metrics.avg_response_time_ms += current_process->response_time_ms;
                }
                
                finished_processes.fetch_add(1);
            } else {
                bool progressed = (current_process->instruction_count > before_instr) || 
                                (current_process->execution_log.size() > before_log);
                if (progressed) {
                    current_process->stagnation_counter = 0;
                } else {
                    current_process->stagnation_counter++;
                    if (current_process->stagnation_counter >= 5) {
                        // Processo estagnado
                        current_process->finish_time = std::chrono::high_resolution_clock::now();
                        
                        if (!current_process->first_run) {
                            auto wait_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                                current_process->start_time - current_process->arrival_time);
                            current_process->wait_time_ms = wait_duration.count();
                            
                            auto turnaround_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                                current_process->finish_time - current_process->arrival_time);
                            current_process->turnaround_time_ms = turnaround_duration.count();
                            
                            auto response_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                                current_process->start_time - current_process->arrival_time);
                            current_process->response_time_ms = response_duration.count();
                            
                            std::lock_guard<std::mutex> lock(metrics_mutex);
                            metrics.avg_wait_time_ms += current_process->wait_time_ms;
                            metrics.avg_turnaround_time_ms += current_process->turnaround_time_ms;
                            metrics.avg_response_time_ms += current_process->response_time_ms;
                        }
                        
                        {
                            std::lock_guard<std::mutex> lock(metrics_mutex);
                            if (save_logs) {
                                print_metrics(*current_process, results_file);
                            }
                        }
                        
                        current_process->state = State::Finished;
                        finished_processes.fetch_add(1);
                        continue;
                    }
                }
                
                current_process->state = State::Ready;
                memManager.simulateContextSwitch();
                
                {
                    std::lock_guard<std::mutex> lock(scheduler_mutex);
                    scheduler.increment_context_switch();
                    scheduler.add_process(current_process);
                }
            }
        }
    };
    
    // Criar e iniciar threads para cada núcleo
    std::vector<std::thread> threads;
    for (int i = 0; i < num_cores; i++) {
        threads.emplace_back(core_function, i);
    }
    
    // Aguardar conclusão de todos os threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    if (save_logs && results_file.is_open()) {
        results_file.close();
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    metrics.execution_time_ms = duration.count() / 1000.0;
    metrics.processes_finished = finished_processes.load();
    metrics.context_switches = scheduler.get_context_switch_count();
    
    // Calcular métricas derivadas
    uint64_t total_cache_accesses = metrics.total_cache_hits + metrics.total_cache_misses;
    metrics.cache_hit_rate = (total_cache_accesses > 0) ? 
        (100.0 * metrics.total_cache_hits / total_cache_accesses) : 0.0;
    
    metrics.avg_memory_cycles_per_access = (metrics.total_memory_accesses > 0) ?
        (static_cast<double>(metrics.total_memory_cycles) / metrics.total_memory_accesses) : 0.0;
    
    metrics.throughput = (metrics.execution_time_ms > 0) ?
        (finished_processes.load() * 1000.0 / metrics.execution_time_ms) : 0.0;
    
    // Calcular médias das métricas de escalonamento
    int procs_finished = finished_processes.load();
    if (procs_finished > 0) {
        metrics.avg_wait_time_ms /= procs_finished;
        metrics.avg_turnaround_time_ms /= procs_finished;
        metrics.avg_response_time_ms /= procs_finished;
        
        uint64_t total_cycles = metrics.total_pipeline_cycles + (procs_finished * 100);
        metrics.avg_cpu_utilization = (total_cycles > 0) ? 
            (100.0 * metrics.total_pipeline_cycles / total_cycles) : 0.0;
        
        metrics.efficiency = (metrics.context_switches > 0) ?
            (metrics.throughput / metrics.context_switches) : metrics.throughput;
    }
    
    // Calcular utilização por núcleo
    for (int i = 0; i < num_cores; i++) {
        uint64_t busy = core_metrics[i].busy_cycles.load();
        uint64_t idle = core_metrics[i].idle_cycles.load();
        uint64_t total = busy + idle;
        double util = (total > 0) ? (100.0 * busy / total) : 0.0;
        metrics.per_core_utilization.push_back(util);
    }
    
    return metrics;
}

// Função para salvar tabela detalhada em arquivo
void save_detailed_comparison(const std::vector<SchedulerMetrics>& all_metrics, const std::string& filename) {
    std::ofstream outFile(filename);
    if (!outFile.is_open()) {
        std::cerr << "Erro ao criar arquivo " << filename << "\n";
        return;
    }
    
    // Verificar se é multicore
    bool is_multicore = !all_metrics.empty() && all_metrics[0].num_cores > 1;
    
    outFile << std::string(150, '=') << "\n";
    outFile << "                    RELATÓRIO COMPARATIVO DETALHADO DE ESCALONADORES";
    if (is_multicore) {
        outFile << " (MULTICORE - " << all_metrics[0].num_cores << " cores)";
    }
    outFile << "\n";
    
    // Obter timestamp atual
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);
    outFile << "                    Data: " << std::put_time(&tm, "%d/%m/%Y %H:%M:%S") << "\n";
    
    outFile << std::string(150, '=') << "\n\n";
    
    // Tabela de Desempenho Geral
    outFile << "=== DESEMPENHO GERAL ===\n\n";
    outFile << std::left << std::setw(15) << "Escalonador"
            << std::right << std::setw(18) << "Tempo Exec(ms)"
            << std::setw(15) << "Throughput"
            << std::setw(15) << "Processos"
            << std::setw(15) << "Ctx Switch"
            << std::setw(18) << "Ciclos CPU"
            << "\n";
    outFile << std::string(100, '-') << "\n";
    
    for (const auto& m : all_metrics) {
        outFile << std::left << std::setw(15) << m.name
                << std::right << std::fixed << std::setprecision(3)
                << std::setw(18) << m.execution_time_ms
                << std::setprecision(2)
                << std::setw(15) << m.throughput
                << std::setw(15) << m.processes_finished
                << std::setw(15) << m.context_switches
                << std::setw(18) << m.total_pipeline_cycles
                << "\n";
    }
    
    // Tabela de Métricas de Escalonamento
    outFile << "\n\n=== MÉTRICAS DE ESCALONAMENTO ===\n\n";
    outFile << std::left << std::setw(15) << "Escalonador"
            << std::right << std::setw(18) << "Tempo Esp(ms)"
            << std::setw(18) << "Tempo Ret(ms)"
            << std::setw(18) << "Tempo Resp(ms)"
            << std::setw(18) << "Util CPU(%)"
            << std::setw(15) << "Eficiência"
            << "\n";
    outFile << std::string(105, '-') << "\n";
    
    for (const auto& m : all_metrics) {
        outFile << std::left << std::setw(15) << m.name
                << std::right << std::fixed << std::setprecision(2)
                << std::setw(18) << m.avg_wait_time_ms
                << std::setw(18) << m.avg_turnaround_time_ms
                << std::setw(18) << m.avg_response_time_ms
                << std::setprecision(1)
                << std::setw(18) << m.avg_cpu_utilization
                << std::setprecision(3)
                << std::setw(15) << m.efficiency
                << "\n";
    }
    
    // Tabela de Métricas de Memória
    outFile << "\n\n=== MÉTRICAS DE MEMÓRIA ===\n\n";
    outFile << std::left << std::setw(15) << "Escalonador"
            << std::right << std::setw(18) << "Acessos Total"
            << std::setw(15) << "Leituras"
            << std::setw(15) << "Escritas"
            << std::setw(18) << "Cache Hits"
            << std::setw(18) << "Cache Misses"
            << "\n";
    outFile << std::string(100, '-') << "\n";
    
    for (const auto& m : all_metrics) {
        outFile << std::left << std::setw(15) << m.name
                << std::right << std::setw(18) << m.total_memory_accesses
                << std::setw(15) << m.total_memory_reads
                << std::setw(15) << m.total_memory_writes
                << std::setw(18) << m.total_cache_hits
                << std::setw(18) << m.total_cache_misses
                << "\n";
    }
    
    // Tabela de Eficiência
    outFile << "\n\n=== EFICIÊNCIA DE CACHE E MEMÓRIA ===\n\n";
    outFile << std::left << std::setw(15) << "Escalonador"
            << std::right << std::setw(18) << "Taxa Cache(%)"
            << std::setw(20) << "Ciclos Mem/Acesso"
            << std::setw(18) << "Mem Principal"
            << std::setw(18) << "Mem Secundária"
            << "\n";
    outFile << std::string(100, '-') << "\n";
    
    for (const auto& m : all_metrics) {
        outFile << std::left << std::setw(15) << m.name
                << std::right << std::fixed << std::setprecision(2)
                << std::setw(18) << m.cache_hit_rate
                << std::setprecision(3)
                << std::setw(20) << m.avg_memory_cycles_per_access
                << std::setw(18) << m.total_primary_mem_accesses
                << std::setw(18) << m.total_secondary_mem_accesses
                << "\n";
    }
    
    // Análise Comparativa
    outFile << "\n\n=== ANÁLISE COMPARATIVA ===\n\n";
    
    auto fastest = std::min_element(all_metrics.begin(), all_metrics.end(),
        [](const auto& a, const auto& b) { return a.execution_time_ms < b.execution_time_ms; });
    auto best_cache = std::max_element(all_metrics.begin(), all_metrics.end(),
        [](const auto& a, const auto& b) { return a.cache_hit_rate < b.cache_hit_rate; });
    auto best_throughput = std::max_element(all_metrics.begin(), all_metrics.end(),
        [](const auto& a, const auto& b) { return a.throughput < b.throughput; });
    auto fewest_switches = std::min_element(all_metrics.begin(), all_metrics.end(),
        [](const auto& a, const auto& b) { return a.context_switches < b.context_switches; });
    auto best_response = std::min_element(all_metrics.begin(), all_metrics.end(),
        [](const auto& a, const auto& b) { return a.avg_response_time_ms < b.avg_response_time_ms; });
    auto best_wait = std::min_element(all_metrics.begin(), all_metrics.end(),
        [](const auto& a, const auto& b) { return a.avg_wait_time_ms < b.avg_wait_time_ms; });
    
    outFile << "🏆 Mais Rápido:           " << fastest->name 
            << " (" << fastest->execution_time_ms << " ms)\n";
    outFile << "💾 Melhor Taxa Cache:     " << best_cache->name 
            << " (" << best_cache->cache_hit_rate << "%)\n";
    outFile << "⚡ Maior Throughput:       " << best_throughput->name 
            << " (" << best_throughput->throughput << " proc/s)\n";
    outFile << "🔄 Menos Ctx Switches:    " << fewest_switches->name 
            << " (" << fewest_switches->context_switches << " switches)\n";
    outFile << "⏱️  Melhor Tempo Resposta: " << best_response->name 
            << " (" << best_response->avg_response_time_ms << " ms)\n";
    outFile << "⌛ Melhor Tempo Espera:   " << best_wait->name 
            << " (" << best_wait->avg_wait_time_ms << " ms)\n";
    
    // Se multicore, adicionar métricas por núcleo
    if (is_multicore) {
        outFile << "\n\n=== UTILIZAÇÃO POR NÚCLEO ===\n\n";
        for (const auto& m : all_metrics) {
            outFile << m.name << ":\n";
            for (size_t core = 0; core < m.per_core_utilization.size(); core++) {
                outFile << "  Core " << core << ": " 
                        << std::fixed << std::setprecision(2) 
                        << m.per_core_utilization[core] << "%\n";
            }
            outFile << "\n";
        }
    }
    
    outFile << "\n" << std::string(150, '=') << "\n";
    outFile.close();
}

// Função para imprimir tabela comparativa
void print_comparison_table(const std::vector<SchedulerMetrics>& all_metrics) {
    std::cout << "\n" << std::string(160, '=') << "\n";
    std::cout << "                    TABELA COMPARATIVA DE ESCALONADORES";
    
    // Verificar se é multicore
    bool is_multicore = !all_metrics.empty() && all_metrics[0].num_cores > 1;
    if (is_multicore) {
        std::cout << " (MULTICORE - " << all_metrics[0].num_cores << " cores)";
    }
    std::cout << "\n";
    std::cout << std::string(160, '=') << "\n\n";
    
    // Tabela 1: Desempenho Geral
    std::cout << "=== DESEMPENHO GERAL ===\n";
    std::cout << std::left << std::setw(12) << "Escalonador"
              << std::right << std::setw(13) << "Tempo(ms)"
              << std::setw(12) << "Throughput"
              << std::setw(10) << "Procs"
              << std::setw(10) << "CtxSwch"
              << std::setw(13) << "Ciclos CPU"
              << std::setw(12) << "Eficiência";
    if (is_multicore) {
        std::cout << std::setw(10) << "Cores";
    }
    std::cout << "\n";
    std::cout << std::string(is_multicore ? 110 : 100, '-') << "\n";
    
    for (const auto& metrics : all_metrics) {
        std::cout << std::left << std::setw(12) << metrics.name
                  << std::right << std::fixed << std::setprecision(2)
                  << std::setw(13) << metrics.execution_time_ms
                  << std::setprecision(1)
                  << std::setw(12) << metrics.throughput
                  << std::setw(10) << metrics.processes_finished
                  << std::setw(10) << metrics.context_switches
                  << std::setw(13) << metrics.total_pipeline_cycles
                  << std::setprecision(3)
                  << std::setw(12) << metrics.efficiency;
        if (is_multicore) {
            std::cout << std::setw(10) << metrics.num_cores;
        }
        std::cout << "\n";
    }
    
    // Tabela 2: Métricas de Escalonamento
    std::cout << "\n=== METRICAS DE ESCALONAMENTO ===\n";
    std::cout << std::left << std::setw(12) << "Escalonador"
              << std::right << std::setw(15) << "Tempo Esp(ms)"
              << std::setw(15) << "Tempo Ret(ms)"
              << std::setw(15) << "Tempo Resp(ms)"
              << std::setw(18) << "Util CPU(%)"
              << "\n";
    std::cout << std::string(100, '-') << "\n";
    
    for (const auto& metrics : all_metrics) {
        std::cout << std::left << std::setw(12) << metrics.name
                  << std::right << std::fixed << std::setprecision(2)
                  << std::setw(15) << metrics.avg_wait_time_ms
                  << std::setw(15) << metrics.avg_turnaround_time_ms
                  << std::setw(15) << metrics.avg_response_time_ms
                  << std::setprecision(1)
                  << std::setw(18) << metrics.avg_cpu_utilization
                  << "\n";
    }
    
    // Tabela 3: Métricas de Cache
    std::cout << "\n=== METRICAS DE CACHE E MEMORIA ===\n";
    std::cout << std::left << std::setw(12) << "Escalonador"
              << std::right << std::setw(13) << "Acess Mem"
              << std::setw(15) << "Cache Hit(%)"
              << std::setw(15) << "Ciclos/Acess"
              << "\n";
    std::cout << std::string(100, '-') << "\n";
    
    for (const auto& metrics : all_metrics) {
        std::cout << std::left << std::setw(12) << metrics.name
                  << std::right << std::setw(13) << metrics.total_memory_accesses
                  << std::fixed << std::setprecision(2)
                  << std::setw(15) << metrics.cache_hit_rate
                  << std::setprecision(2)
                  << std::setw(15) << metrics.avg_memory_cycles_per_access
                  << "\n";
    }
    
    std::cout << std::string(160, '=') << "\n";
    
    // Análise
    auto fastest = std::min_element(all_metrics.begin(), all_metrics.end(),
        [](const SchedulerMetrics& a, const SchedulerMetrics& b) {
            return a.execution_time_ms < b.execution_time_ms;
        });
    
    auto most_efficient_cache = std::max_element(all_metrics.begin(), all_metrics.end(),
        [](const SchedulerMetrics& a, const SchedulerMetrics& b) {
            return a.cache_hit_rate < b.cache_hit_rate;
        });
    
    auto fewest_switches = std::min_element(all_metrics.begin(), all_metrics.end(),
        [](const SchedulerMetrics& a, const SchedulerMetrics& b) {
            return a.context_switches < b.context_switches;
        });
    
    auto best_response = std::min_element(all_metrics.begin(), all_metrics.end(),
        [](const SchedulerMetrics& a, const SchedulerMetrics& b) {
            return a.avg_response_time_ms < b.avg_response_time_ms;
        });
    
    std::cout << "\n📊 ANÁLISE:\n";
    std::cout << "  🏆 Mais Rápido:           " << fastest->name 
              << " (" << fastest->execution_time_ms << " ms)\n";
    std::cout << "  💾 Melhor Taxa Cache:     " << most_efficient_cache->name 
              << " (" << most_efficient_cache->cache_hit_rate << "%)\n";
    std::cout << "  🔄 Menos Context Switches: " << fewest_switches->name 
              << " (" << fewest_switches->context_switches << " switches)\n";
    std::cout << "  ⚡ Melhor Tempo Resposta:  " << best_response->name 
              << " (" << best_response->avg_response_time_ms << " ms)\n";
    std::cout << std::string(120, '=') << "\n\n";
}


int main() {
    // 0. Configuração Inicial - Número de Cores
    std::cout << "=== SIMULADOR DE ARQUITETURA MULTICORE VON NEUMANN ===\n\n";
    std::cout << "Digite o número de cores (1-8): ";
    int num_cores = 1;
    std::cin >> num_cores;
    if (num_cores < 1 || num_cores > 8) {
        std::cerr << "Número inválido. Usando 1 core (single-core).\n";
        num_cores = 1;
    }
    std::cout << "Configuração: " << num_cores << " core(s)\n\n";
    
    // 1. Escolha do Algoritmo de Escalonamento
    std::cout << "Escolha o algoritmo de escalonamento:\n";
    std::cout << "1. FCFS (First-Come, First-Served)\n";
    std::cout << "2. SJN (Shortest Job Next)\n";
    std::cout << "3. Priority\n";
    std::cout << "4. Round Robin (RR)\n";
    std::cout << "5. Executar TODOS e Comparar\n";
    std::cout << "Digite sua escolha (1-5): ";
    int choice;
    std::cin >> choice;

    if (choice == 5) {
        // Executar todos e comparar
        std::cout << "\n🔄 Executando todos os escalonadores";
        if (num_cores > 1) {
            std::cout << " com " << num_cores << " cores (MULTICORE)";
        }
        std::cout << "...\n\n";
        
        std::vector<SchedulerMetrics> all_metrics;
        
        // Escolher função apropriada baseada no número de cores
        auto run_func = [&](SchedulerType type, const std::string& name) -> SchedulerMetrics {
            if (num_cores > 1) {
                return run_multicore_scheduler(num_cores, type, name, true);
            } else {
                return run_scheduler(type, name, true);
            }
        };
        
        std::cout << "⏳ Executando FCFS..." << std::flush;
        all_metrics.push_back(run_func(SchedulerType::FCFS, "FCFS"));
        std::cout << " ✅\n";
        
        std::cout << "🔄 Resetando cache para próximo teste..." << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << " ✅\n";
        
        std::cout << "⏳ Executando SJN..." << std::flush;
        all_metrics.push_back(run_func(SchedulerType::SJN, "SJN"));
        std::cout << " ✅\n";
        
        std::cout << "🔄 Resetando cache para próximo teste..." << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << " ✅\n";
        
        std::cout << "⏳ Executando Priority..." << std::flush;
        all_metrics.push_back(run_func(SchedulerType::Priority, "Priority"));
        std::cout << " ✅\n";
        
        std::cout << "🔄 Resetando cache para próximo teste..." << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << " ✅\n";
        
        std::cout << "⏳ Executando Round Robin..." << std::flush;
        all_metrics.push_back(run_func(SchedulerType::RoundRobin, "RoundRobin"));
        std::cout << " ✅\n";
        
        print_comparison_table(all_metrics);
        
        // Salvar tabela detalhada em arquivo
        std::string filename = (num_cores > 1) ? 
            "output/comparacao_escalonadores_multicore_" + std::to_string(num_cores) + "cores.txt" :
            "output/comparacao_escalonadores.txt";
        save_detailed_comparison(all_metrics, filename);
        
        std::cout << "📁 Logs detalhados salvos em:\n";
        std::cout << "   - output/resultados_FCFS" << (num_cores > 1 ? "_multicore" : "") << ".dat\n";
        std::cout << "   - output/resultados_SJN" << (num_cores > 1 ? "_multicore" : "") << ".dat\n";
        std::cout << "   - output/resultados_Priority" << (num_cores > 1 ? "_multicore" : "") << ".dat\n";
        std::cout << "   - output/resultados_RoundRobin" << (num_cores > 1 ? "_multicore" : "") << ".dat\n";
        std::cout << "   - " << filename << " (Tabela Comparativa Completa)\n\n";
        
        // Se multicore, mostrar métricas por núcleo
        if (num_cores > 1) {
            std::cout << "\n=== UTILIZAÇÃO POR NÚCLEO ===\n";
            for (size_t i = 0; i < all_metrics.size(); i++) {
                std::cout << "\n" << all_metrics[i].name << ":\n";
                for (size_t core = 0; core < all_metrics[i].per_core_utilization.size(); core++) {
                    std::cout << "  Core " << core << ": " 
                              << std::fixed << std::setprecision(2) 
                              << all_metrics[i].per_core_utilization[core] << "%\n";
                }
            }
            std::cout << "\n";
        }
        
        return 0;
    }

    SchedulerType scheduler_type;
    switch (choice) {
        case 1: scheduler_type = SchedulerType::FCFS; break;
        case 2: scheduler_type = SchedulerType::SJN; break;
        case 3: scheduler_type = SchedulerType::Priority; break;
        case 4: scheduler_type = SchedulerType::RoundRobin; break;
        default:
            std::cerr << "Escolha inválida. Usando FCFS por padrão.\n";
            scheduler_type = SchedulerType::FCFS;
            break;
    }

    // 2. Inicialização dos Módulos Principais
    std::cout << "Inicializando o simulador...\n";
    MemoryManager memManager(8192, 16384); // Aumentando a memória
    IOManager ioManager;
    Scheduler scheduler(scheduler_type);

    // 3. Carregamento dos Processos
    std::vector<std::unique_ptr<PCB>> process_list;
    
    // Processo 1: Quick
    auto p1 = std::make_unique<PCB>();
    if (load_pcb_from_json("processes/process_quick.json", *p1)) {
        std::cout << "Carregando programa 'tasks_quick.json' para o processo " << p1->pid << "...\n";
        loadJsonProgram("tasks/tasks_quick.json", memManager, *p1, 0);
        process_list.push_back(std::move(p1));
    } else {
        std::cerr << "Erro ao carregar 'process_quick.json'.\n";
    }

    // Processo 2: Short
    auto p2 = std::make_unique<PCB>();
    if (load_pcb_from_json("processes/process_short.json", *p2)) {
        std::cout << "Carregando programa 'tasks_short.json' para o processo " << p2->pid << "...\n";
        loadJsonProgram("tasks/tasks_short.json", memManager, *p2, 1024);
        process_list.push_back(std::move(p2));
    } else {
        std::cerr << "Erro ao carregar 'process_short.json'.\n";
    }

    // Processo 3: Medium
    auto p3 = std::make_unique<PCB>();
    if (load_pcb_from_json("processes/process_medium.json", *p3)) {
        std::cout << "Carregando programa 'tasks_medium.json' para o processo " << p3->pid << "...\n";
        loadJsonProgram("tasks/tasks_medium.json", memManager, *p3, 2048);
        process_list.push_back(std::move(p3));
    } else {
        std::cerr << "Erro ao carregar 'process_medium.json'.\n";
    }

    // Processo 4: Long
    auto p4 = std::make_unique<PCB>();
    if (load_pcb_from_json("processes/process_long.json", *p4)) {
        std::cout << "Carregando programa 'tasks_long.json' para o processo " << p4->pid << "...\n";
        loadJsonProgram("tasks/tasks_long.json", memManager, *p4, 3072);
        process_list.push_back(std::move(p4));
    } else {
        std::cerr << "Erro ao carregar 'process_long.json'.\n";
    }

    // Processo 5: CPU-Bound
    auto p5 = std::make_unique<PCB>();
    if (load_pcb_from_json("processes/process_cpu_bound.json", *p5)) {
        std::cout << "Carregando programa 'tasks_cpu_bound.json' para o processo " << p5->pid << "...\n";
        loadJsonProgram("tasks/tasks_cpu_bound.json", memManager, *p5, 4096);
        process_list.push_back(std::move(p5));
    } else {
        std::cerr << "Erro ao carregar 'process_cpu_bound.json'.\n";
    }

    // Processo 6: IO-Bound
    auto p6 = std::make_unique<PCB>();
    if (load_pcb_from_json("processes/process_io_bound.json", *p6)) {
        std::cout << "Carregando programa 'tasks_io_bound.json' para o processo " << p6->pid << "...\n";
        loadJsonProgram("tasks/tasks_io_bound.json", memManager, *p6, 5120);
        process_list.push_back(std::move(p6));
    } else {
        std::cerr << "Erro ao carregar 'process_io_bound.json'.\n";
    }

    // Processo 7: Memory-Intensive
    auto p7 = std::make_unique<PCB>();
    if (load_pcb_from_json("processes/process_memory_intensive.json", *p7)) {
        std::cout << "Carregando programa 'tasks_memory_intensive.json' para o processo " << p7->pid << "...\n";
        loadJsonProgram("tasks/tasks_memory_intensive.json", memManager, *p7, 6144);
        process_list.push_back(std::move(p7));
    } else {
        std::cerr << "Erro ao carregar 'process_memory_intensive.json'.\n";
    }

    // Processo 8: Balanced
    auto p8 = std::make_unique<PCB>();
    if (load_pcb_from_json("processes/process_balanced.json", *p8)) {
        std::cout << "Carregando programa 'tasks_balanced.json' para o processo " << p8->pid << "...\n";
        loadJsonProgram("tasks/tasks_balanced.json", memManager, *p8, 7168);
        process_list.push_back(std::move(p8));
    } else {
        std::cerr << "Erro ao carregar 'process_balanced.json'.\n";
    }

    // Adiciona os processos ao escalonador
    for (const auto& process : process_list) {
        scheduler.add_process(process.get());
    }

    int total_processes = process_list.size();
    int finished_processes = 0;
    std::vector<PCB*> blocked_list;

    // Abre o arquivo de resultados uma vez
    create_output_directory();
    std::ofstream results_file("output/resultados.dat");

    // 4. Loop Principal do Escalonador
    int max_iterations = 10000; // Proteção contra loop infinito
    int iteration_count = 0;
    
    while (finished_processes < total_processes && iteration_count < max_iterations) {
        iteration_count++;
        // Desbloqueia processos que terminaram I/O
        for (auto it = blocked_list.begin(); it != blocked_list.end(); ) {
            if ((*it)->state == State::Ready) {
                std::cout << "[Scheduler] Processo " << (*it)->pid << " desbloqueado.\n";
                scheduler.add_process(*it);
                it = blocked_list.erase(it);
            } else {
                ++it;
            }
        }

        PCB* current_process = scheduler.get_next_process();

        if (!current_process) {
            if (blocked_list.empty() && scheduler.is_empty()) {
                break; // Todos os processos terminaram
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        std::cout << "\n[Scheduler] Executando processo " << current_process->pid << " (Prioridade: " << current_process->priority << ").\n";
        current_process->state = State::Running;

        std::vector<std::unique_ptr<IORequest>> io_requests;
        bool print_lock = true;

        // Marcar estado antes da execução para detectar progresso
        int before_instr = current_process->instruction_count;
        size_t before_log = current_process->execution_log.size();

        Core(memManager, *current_process, &io_requests, print_lock);

        // Avalia o estado do processo após a execução
        if (current_process->state == State::Blocked) {
            std::cout << "[Scheduler] Processo " << current_process->pid << " bloqueado por I/O.\n";
            ioManager.registerProcessWaitingForIO(current_process);
            blocked_list.push_back(current_process);
        } else if (current_process->state == State::Finished) {
            std::cout << "[Scheduler] Processo " << current_process->pid << " finalizado.\n";
            print_metrics(*current_process, results_file);
            finished_processes++;
        } else { // Ready or other states
            std::cout << "[Scheduler] Processo " << current_process->pid << " volta para a fila.\n";
            // Detecta progresso: instruções executadas ou novos logs
            bool progressed = (current_process->instruction_count > before_instr) || (current_process->execution_log.size() > before_log);
            if (progressed) {
                current_process->stagnation_counter = 0;
            } else {
                current_process->stagnation_counter++;
                if (current_process->stagnation_counter >= 5) {
                    std::cout << "[Scheduler] Processo " << current_process->pid << " considerado estagnado. Finalizando para evitar loop.\n";
                    current_process->state = State::Finished;
                    print_metrics(*current_process, results_file);
                    finished_processes++;
                    continue; // pula re-inserção na fila
                }
            }
            current_process->state = State::Ready;
            scheduler.add_process(current_process);
        }
    }

    std::cout << "\nSimulação finalizada. Resultados salvos em 'output/resultados.dat'.\n";
    results_file.close();
    
    return 0;
}
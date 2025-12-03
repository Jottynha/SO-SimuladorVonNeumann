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
#include <string>
#include <algorithm>
#include <map>


#include "cpu/PCB.hpp"
#include "cpu/pcb_loader.hpp"
#include "cpu/CONTROL_UNIT.hpp"
#include "memory/MemoryManager.hpp"
#include "memory/SegmentTable.hpp"
#include "memory/MemoryUsageTracker.hpp"
#include "parser_json/parser_json.hpp"
#include "IO/IOManager.hpp"
#include "cpu/Scheduler.hpp"

// ==================== ESTRUTURAS PARA LINHA DE COMANDO ====================

struct CommandLineConfig {
    std::string config_dir = "processes";
    std::string tasks_dir = "tasks";
    std::string output_dir = "output";
    int cores = 1;
    std::string replacement_policy = "FIFO";  // FIFO ou LRU
    std::string scheduler = "FCFS";            // FCFS, SJN, Priority, RR
    int quantum = 5;
    bool use_threads = true;                  // Se true, usa multi-threading quando cores > 1
    bool interactive_mode = true;             // Se true, usa menu interativo
    bool help = false;
};

// Função para exibir ajuda
void print_help(const char* program_name) {
    std::cout << "Uso: " << program_name << " [opções]\n\n";
    std::cout << "Opções:\n";
    std::cout << "  --config <dir>       Diretório dos arquivos PCB (padrão: processes)\n";
    std::cout << "  --tasks <dir>        Diretório dos arquivos de tarefas (padrão: tasks)\n";
    std::cout << "  --output <dir>       Diretório de saída (padrão: output)\n";
    std::cout << "  --cores <n>          Número de cores 1-8 (padrão: 1)\n";
    std::cout << "  --no-threads         Desabilita multi-threading (usa sequencial mesmo com múltiplos cores)\n";
    std::cout << "  --replacement <pol>  Política de substituição: FIFO ou LRU (padrão: FIFO)\n";
    std::cout << "  --scheduler <alg>    Algoritmo: FCFS, SJN, Priority, RR (padrão: FCFS)\n";
    std::cout << "  --quantum <n>        Quantum para Round Robin (padrão: 5)\n";
    std::cout << "  --help, -h           Mostra esta ajuda\n\n";
    std::cout << "Exemplos:\n";
    std::cout << "  " << program_name << "\n";
    std::cout << "    (modo interativo)\n\n";
    std::cout << "  " << program_name << " --cores 4 --scheduler RR --quantum 10\n";
    std::cout << "    (4 cores com threads, Round Robin, quantum 10)\n\n";
    std::cout << "  " << program_name << " --cores 4 --no-threads --scheduler FCFS\n";
    std::cout << "    (4 cores SEM threads - execução sequencial)\n\n";
    std::cout << "  " << program_name << " --config processes/ --tasks tasks/ \\\n";
    std::cout << "    --cores 4 --replacement LRU --scheduler RR --quantum 5 \\\n";
    std::cout << "    --output output/\n";
    std::cout << "    (configuração completa via linha de comando)\n\n";
}

// Parser de argumentos da linha de comando
CommandLineConfig parse_arguments(int argc, char* argv[]) {
    CommandLineConfig config;
    
    // Se não há argumentos, usa modo interativo
    if (argc == 1) {
        return config;
    }
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            config.help = true;
            return config;
        }
        else if (arg == "--no-threads") {
            config.use_threads = false;
            config.interactive_mode = false;
        }
        else if (arg == "--config" && i + 1 < argc) {
            config.config_dir = argv[++i];
            config.interactive_mode = false;
        }
        else if (arg == "--tasks" && i + 1 < argc) {
            config.tasks_dir = argv[++i];
            config.interactive_mode = false;
        }
        else if (arg == "--output" && i + 1 < argc) {
            config.output_dir = argv[++i];
            config.interactive_mode = false;
        }
        else if (arg == "--cores" && i + 1 < argc) {
            config.cores = std::stoi(argv[++i]);
            config.interactive_mode = false;
        }
        else if (arg == "--replacement" && i + 1 < argc) {
            config.replacement_policy = argv[++i];
            std::transform(config.replacement_policy.begin(), 
                         config.replacement_policy.end(), 
                         config.replacement_policy.begin(), ::toupper);
            config.interactive_mode = false;
        }
        else if (arg == "--scheduler" && i + 1 < argc) {
            config.scheduler = argv[++i];
            std::transform(config.scheduler.begin(), 
                         config.scheduler.end(), 
                         config.scheduler.begin(), ::toupper);
            config.interactive_mode = false;
        }
        else if (arg == "--quantum" && i + 1 < argc) {
            config.quantum = std::stoi(argv[++i]);
            config.interactive_mode = false;
        }
        else {
            std::cerr << "Argumento desconhecido: " << arg << "\n";
            std::cerr << "Use --help para ver a lista de opções.\n";
            exit(1);
        }
    }
    
    return config;
}

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

// Função para imprimir as métricas de um processo (SIMPLIFICADA)
void print_metrics(const PCB& pcb, std::ofstream& outFile) {
    outFile << "\n=== PROCESSO " << pcb.pid << ": " << pcb.name << " ===\n";
    outFile << "Estado: " << (pcb.state == State::Finished ? "Finalizado" : "Incompleto") << "\n";
    outFile << "Prioridade: " << pcb.priority << " | Quantum: " << pcb.quantum << "\n";
    
    // Métricas de Tempo (Escalonamento)
    outFile << "\n[TEMPOS]\n";
    outFile << "  Espera:   " << std::setw(6) << pcb.wait_time_ms << " ms\n";
    outFile << "  Resposta: " << std::setw(6) << pcb.response_time_ms << " ms\n";
    outFile << "  Retorno:  " << std::setw(6) << pcb.turnaround_time_ms << " ms\n";
    
    // Métricas de CPU
    outFile << "\n[CPU]\n";
    outFile << "  Pipeline Cycles: " << pcb.pipeline_cycles.load() << "\n";
    outFile << "  IO Cycles:       " << pcb.io_cycles.load() << "\n";
    
    // Métricas de Memória (ESSENCIAL)
    outFile << "\n[MEMÓRIA]\n";
    outFile << "  Total Acessos:    " << pcb.mem_accesses_total.load() << "\n";
    outFile << "    - Leituras:     " << pcb.mem_reads.load() << "\n";
    outFile << "    - Escritas:     " << pcb.mem_writes.load() << "\n";
    outFile << "  Cache L1:         " << pcb.cache_mem_accesses.load() << "\n";
    outFile << "  RAM (Principal):  " << pcb.primary_mem_accesses.load() << "\n";
    outFile << "  Disco (Secund.):  " << pcb.secondary_mem_accesses.load() << "\n";
    outFile << "  Ciclos Memória:   " << pcb.memory_cycles.load() << "\n";
    
    // Cache Performance (CRUCIAL para análise)
    uint64_t cache_hits = pcb.cache_hits.load();
    uint64_t cache_misses = pcb.cache_misses.load();
    uint64_t total_cache = cache_hits + cache_misses;
    double hit_rate = (total_cache > 0) ? (100.0 * cache_hits / total_cache) : 0.0;
    
    outFile << "\n[CACHE PERFORMANCE]\n";
    outFile << "  Hits:   " << cache_hits << "\n";
    outFile << "  Misses: " << cache_misses << "\n";
    outFile << "  Taxa:   " << std::fixed << std::setprecision(2) << hit_rate << "%\n";
    
    outFile << "\n" << std::string(60, '-') << "\n";
}


// Função auxiliar para carregar processos
std::vector<std::unique_ptr<PCB>> load_processes(MemoryManager& memManager, 
                                                  const std::string& config_dir = "processes",
                                                  const std::string& tasks_dir = "tasks") {
    std::vector<std::unique_ptr<PCB>> process_list;
    
    std::cout << "\n[LOAD_PROCESSES] Iniciando carregamento de processos...\n";
    std::cout << "   Config Dir: " << config_dir << "\n";
    std::cout << "   Tasks Dir:  " << tasks_dir << "\n\n";
    
    int loaded_count = 0;
    
    // Processo 1: Quick
    std::cout << "   [1/9] Carregando Quick Process... ";
    auto p1 = std::make_unique<PCB>();
    if (load_pcb_from_json(config_dir + "/process_quick.json", *p1)) {
        loadJsonProgram(tasks_dir + "/tasks_quick.json", memManager, *p1, 0);
        process_list.push_back(std::move(p1));
        std::cout << " PID: " << process_list.back()->pid << "\n";
        loaded_count++;
    } else {
        std::cout << "❌ Falhou\n";
    }

    // Processo 2: Short
    std::cout << "   [2/9] Carregando Short Process... ";
    auto p2 = std::make_unique<PCB>();
    if (load_pcb_from_json(config_dir + "/process_short.json", *p2)) {
        loadJsonProgram(tasks_dir + "/tasks_short.json", memManager, *p2, 1024);
        process_list.push_back(std::move(p2));
        std::cout << " PID: " << process_list.back()->pid << "\n";
        loaded_count++;
    } else {
        std::cout << "❌ Falhou\n";
    }

    // Processo 3: Medium
    std::cout << "   [3/9] Carregando Medium Process... ";
    auto p3 = std::make_unique<PCB>();
    if (load_pcb_from_json(config_dir + "/process_medium.json", *p3)) {
        loadJsonProgram(tasks_dir + "/tasks_medium.json", memManager, *p3, 2048);
        process_list.push_back(std::move(p3));
        std::cout << " PID: " << process_list.back()->pid << "\n";
        loaded_count++;
    } else {
        std::cout << "❌ Falhou\n";
    }

    // Processo 4: Long
    std::cout << "   [4/9] Carregando Long Process... ";
    auto p4 = std::make_unique<PCB>();
    if (load_pcb_from_json(config_dir + "/process_long.json", *p4)) {
        loadJsonProgram(tasks_dir + "/tasks_long.json", memManager, *p4, 3072);
        process_list.push_back(std::move(p4));
        std::cout << " PID: " << process_list.back()->pid << "\n";
        loaded_count++;
    } else {
        std::cout << "❌ Falhou\n";
    }

    // Processo 5: CPU-Bound
    std::cout << "   [5/9] Carregando CPU-Bound Process... ";
    auto p5 = std::make_unique<PCB>();
    if (load_pcb_from_json(config_dir + "/process_cpu_bound.json", *p5)) {
        loadJsonProgram(tasks_dir + "/tasks_cpu_bound.json", memManager, *p5, 4096);
        process_list.push_back(std::move(p5));
        std::cout << " PID: " << process_list.back()->pid << "\n";
        loaded_count++;
    } else {
        std::cout << "❌ Falhou\n";
    }

    // Processo 6: IO-Bound
    std::cout << "   [6/9] Carregando IO-Bound Process... ";
    auto p6 = std::make_unique<PCB>();
    if (load_pcb_from_json(config_dir + "/process_io_bound.json", *p6)) {
        loadJsonProgram(tasks_dir + "/tasks_io_bound.json", memManager, *p6, 5120);
        process_list.push_back(std::move(p6));
        std::cout << " PID: " << process_list.back()->pid << "\n";
        loaded_count++;
    } else {
        std::cout << "❌ Falhou\n";
    }

    // Processo 7: Memory-Intensive
    std::cout << "   [7/9] Carregando Memory-Intensive Process... ";
    auto p7 = std::make_unique<PCB>();
    if (load_pcb_from_json(config_dir + "/process_memory_intensive.json", *p7)) {
        loadJsonProgram(tasks_dir + "/tasks_memory_intensive.json", memManager, *p7, 6144);
        process_list.push_back(std::move(p7));
        std::cout << " PID: " << process_list.back()->pid << "\n";
        loaded_count++;
    } else {
        std::cout << "❌ Falhou\n";
    }

    // Processo 8: Balanced
    std::cout << "   [8/9] Carregando Balanced Process... ";
    auto p8 = std::make_unique<PCB>();
    if (load_pcb_from_json(config_dir + "/process_balanced.json", *p8)) {
        loadJsonProgram(tasks_dir + "/tasks_balanced.json", memManager, *p8, 7168);
        process_list.push_back(std::move(p8));
        std::cout << " PID: " << process_list.back()->pid << "\n";
        loaded_count++;
    } else {
        std::cout << "❌ Falhou\n";
    }
    
    // Processo 9: Loop-Heavy (para demonstrar preempção)
    std::cout << "   [9/9] Carregando Loop-Heavy Process... ";
    auto p9 = std::make_unique<PCB>();
    bool loaded = load_pcb_from_json(config_dir + "/process_loop_heavy.json", *p9);
    if (loaded) {
        loadJsonProgram(tasks_dir + "/tasks_loop_heavy.json", memManager, *p9, 8192);
        process_list.push_back(std::move(p9));
        std::cout << " PID: " << process_list.back()->pid << "\n";
        loaded_count++;
    } else {
        std::cout << "❌ Falhou\n";
    }
    
    std::cout << "\n   📦 Total: " << loaded_count << "/9 processos carregados com sucesso!\n";
    
    // Registrar tempo de chegada de todos os processos
    auto arrival = std::chrono::high_resolution_clock::now();
    for (auto& proc : process_list) {
        proc->arrival_time = arrival;
    }
    
    return process_list;
}

// Função para executar um escalonador e retornar suas métricas
SchedulerMetrics run_scheduler(SchedulerType scheduler_type, const std::string& scheduler_name, 
                               bool save_logs = false,
                               const std::string& config_dir = "processes",
                               const std::string& tasks_dir = "tasks",
                               const std::string& output_dir = "output") {
    SchedulerMetrics metrics;
    metrics.name = scheduler_name;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    MemoryManager memManager(8192, 16384);
    // Reset cache para garantir execução limpa entre escalonadores
    memManager.resetCache();
    
    IOManager ioManager;
    Scheduler scheduler(scheduler_type);
    
    auto process_list = load_processes(memManager, config_dir, tasks_dir);
    
    for (const auto& process : process_list) {
        scheduler.add_process(process.get());
    }

    int total_processes = process_list.size();
    int finished_processes = 0;
    std::vector<PCB*> blocked_list;

    mkdir(output_dir.c_str(), 0755);
    std::ofstream results_file;
    if (save_logs) {
        results_file.open(output_dir + "/resultados_" + scheduler_name + ".dat");
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
                                         const std::string& scheduler_name, bool save_logs = false,
                                         const std::string& config_dir = "processes",
                                         const std::string& tasks_dir = "tasks",
                                         const std::string& output_dir = "output") {
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
    auto process_list = load_processes(memManager, config_dir, tasks_dir);
    
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
        results_file.open(output_dir + "/resultados_" + scheduler_name + "_multicore.dat");
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
    
    outFile << "Mais Rápido:           " << fastest->name 
            << " (" << fastest->execution_time_ms << " ms)\n";
    outFile << " Melhor Taxa Cache:     " << best_cache->name 
            << " (" << best_cache->cache_hit_rate << "%)\n";
    outFile << " Maior Throughput:       " << best_throughput->name 
            << " (" << best_throughput->throughput << " proc/s)\n";
    outFile << " Menos Ctx Switches:    " << fewest_switches->name 
            << " (" << fewest_switches->context_switches << " switches)\n";
    outFile << "  Melhor Tempo Resposta: " << best_response->name 
            << " (" << best_response->avg_response_time_ms << " ms)\n";
    outFile << " Melhor Tempo Espera:   " << best_wait->name 
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
    
    std::cout << "\nANÁLISE:\n";
    std::cout << "  Mais Rápido:           " << fastest->name 
              << " (" << fastest->execution_time_ms << " ms)\n";
    std::cout << "   Melhor Taxa Cache:     " << most_efficient_cache->name 
              << " (" << most_efficient_cache->cache_hit_rate << "%)\n";
    std::cout << "   Menos Context Switches: " << fewest_switches->name 
              << " (" << fewest_switches->context_switches << " switches)\n";
    std::cout << "   Melhor Tempo Resposta:  " << best_response->name 
              << " (" << best_response->avg_response_time_ms << " ms)\n";
    std::cout << std::string(120, '=') << "\n\n";
}


int main(int argc, char* argv[]) {
    // Parser de argumentos da linha de comando
    CommandLineConfig config = parse_arguments(argc, argv);
    
    // Mostrar ajuda se solicitado
    if (config.help) {
        print_help(argv[0]);
        return 0;
    }
    
    std::cout << "=== SIMULADOR DE ARQUITETURA MULTICORE VON NEUMANN ===\n\n";
    
    int num_cores = config.cores;
    int choice = 0;
    SchedulerType scheduler_type;
    
    // ==== MODO INTERATIVO ====
    if (config.interactive_mode) {
        // 0. Configuração Inicial - Número de Cores
        std::cout << "Digite o número de cores (1-8): ";
        std::cin >> num_cores;
        if (num_cores < 1 || num_cores > 8) {
            std::cerr << "Número inválido. Usando 1 core (single-core).\n";
            num_cores = 1;
        }
        std::cout << "Configuração: " << num_cores << " core(s)\n";
        
        // 0.1 Perguntar sobre threading se múltiplos cores
        if (num_cores > 0) {
            char use_threading;
            std::cout << "Usar multi-threading? (s/n, padrão: s): ";
            std::cin >> use_threading;
            if (use_threading == 'n' || use_threading == 'N') {
                config.use_threads = false;
                std::cout << "Threading: DESABILITADO (execução sequencial)\n";
            } else {
                config.use_threads = true;
                std::cout << "Threading: HABILITADO (execução paralela)\n";
            }
        }
        std::cout << "\n";
        
        // 1. Escolha do Algoritmo de Escalonamento
        std::cout << "Escolha o algoritmo de escalonamento:\n";
        std::cout << "1. FCFS (First-Come, First-Served)\n";
        std::cout << "2. SJN (Shortest Job Next)\n";
        std::cout << "3. Priority\n";
        std::cout << "4. Round Robin (RR)\n";
        std::cout << "5. Executar TODOS e Comparar\n";
        std::cout << "Digite sua escolha (1-5): ";
        std::cin >> choice;
    }
    // ==== MODO LINHA DE COMANDO ====
    else {
        // Validar número de cores
        if (num_cores < 1 || num_cores > 8) {
            std::cerr << "Número de cores inválido: " << num_cores << " (deve ser 1-8)\n";
            return 1;
        }
        
        // Converter scheduler string para tipo
        std::map<std::string, SchedulerType> scheduler_map = {
            {"FCFS", SchedulerType::FCFS},
            {"SJN", SchedulerType::SJN},
            {"PRIORITY", SchedulerType::Priority},
            {"RR", SchedulerType::RoundRobin}
        };
        
        if (scheduler_map.find(config.scheduler) == scheduler_map.end()) {
            std::cerr << "Escalonador inválido: " << config.scheduler << "\n";
            std::cerr << "   Use: FCFS, SJN, Priority ou RR\n";
            return 1;
        }
        
        // Validar política de substituição
        if (config.replacement_policy != "FIFO" && config.replacement_policy != "LRU") {
            std::cerr << "Política de substituição inválida: " << config.replacement_policy << "\n";
            std::cerr << "   Use: FIFO ou LRU\n";
            return 1;
        }
        
        scheduler_type = scheduler_map[config.scheduler];
        
        // Criar diretório de saída se não existir
        mkdir(config.output_dir.c_str(), 0755);
        
        std::cout << " Configuração:\n";
        std::cout << "   Cores:        " << num_cores << "\n";
        std::cout << "   Threading:    " << (config.use_threads && num_cores > 1 ? "✓ Habilitado" : "✗ Desabilitado") << "\n";
        std::cout << "   Escalonador:  " << config.scheduler << "\n";
        std::cout << "   Quantum:      " << config.quantum << " ciclos\n";
        std::cout << "   Substituição: " << config.replacement_policy << "\n";
        std::cout << "   Config Dir:   " << config.config_dir << "\n";
        std::cout << "   Tasks Dir:    " << config.tasks_dir << "\n";
        std::cout << "   Output Dir:   " << config.output_dir << "\n\n";
        
        // Executar escalonador diretamente via CLI
        std::cout << "Executando " << config.scheduler;
        if (num_cores > 1) {
            std::cout << " (" << num_cores << " cores, " 
                      << (config.use_threads ? "multi-thread" : "sequencial") << ")";
        }
        std::cout << "...\n\n";
        
        SchedulerMetrics metrics;
        // Decidir entre multi-thread ou sequencial
        if (num_cores > 1 && config.use_threads) {
            metrics = run_multicore_scheduler(num_cores, scheduler_type, config.scheduler, true,
                                             config.config_dir, config.tasks_dir, config.output_dir);
        } else {
            // Execução sequencial (mesmo com múltiplos cores logicamente)
            metrics = run_scheduler(scheduler_type, config.scheduler, true,
                                   config.config_dir, config.tasks_dir, config.output_dir);
            metrics.num_cores = num_cores; // Registrar número de cores configurados
        }
        
        std::cout << "\nExecução concluída!\n";
        std::cout << "Tempo: " << metrics.execution_time_ms << " ms\n";
        std::cout << "Processos finalizados: " << metrics.processes_finished << "\n";
        std::cout << "Context switches: " << metrics.context_switches << "\n";
        std::cout << "Cache hit rate: " << std::fixed << std::setprecision(2) 
                  << metrics.cache_hit_rate << "%\n\n";
        
        std::string mode_suffix = (num_cores > 1 && config.use_threads) ? "_multicore" : "";
        std::string output_file = config.output_dir + "/resultados_" + config.scheduler + mode_suffix + ".dat";
        std::cout << "Resultados salvos em: " << output_file << "\n";
        
        return 0;
    }

    if (choice == 5) {
        // Executar todos e comparar
        std::cout << "\nExecutando todos os escalonadores";
        if (num_cores > 1) {
            std::cout << " com " << num_cores << " cores";
            std::cout << " (" << (config.use_threads ? "multi-thread" : "sequencial") << ")";
        }
        std::cout << "...\n\n";
        
        std::vector<SchedulerMetrics> all_metrics;
        
        // Escolher função apropriada baseada no número de cores e threading
        auto run_func = [&](SchedulerType type, const std::string& name) -> SchedulerMetrics {
            if (num_cores > 1 && config.use_threads) {
                return run_multicore_scheduler(num_cores, type, name, true);
            } else {
                auto metrics = run_scheduler(type, name, true);
                metrics.num_cores = num_cores;
                return metrics;
            }
        };
        
        std::cout << "Executando FCFS..." << std::flush;
        all_metrics.push_back(run_func(SchedulerType::FCFS, "FCFS"));
        
        
        std::cout << "Resetando cache para próximo teste..." << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        
        std::cout << "Executando SJN..." << std::flush;
        all_metrics.push_back(run_func(SchedulerType::SJN, "SJN"));
        
        
        std::cout << "Resetando cache para próximo teste..." << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        
        std::cout << "Executando Priority..." << std::flush;
        all_metrics.push_back(run_func(SchedulerType::Priority, "Priority"));
        
        
        std::cout << "Resetando cache para próximo teste..." << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        
        std::cout << "Executando Round Robin..." << std::flush;
        all_metrics.push_back(run_func(SchedulerType::RoundRobin, "RoundRobin"));
        
        
        print_comparison_table(all_metrics);
        
        // Salvar tabela detalhada em arquivo
        std::string mode_str = (num_cores > 1 && config.use_threads) ? "_multicore_" : "_";
        std::string filename = "output/comparacao_escalonadores" + mode_str;
        if (num_cores > 1) {
            filename += std::to_string(num_cores) + "cores";
            if (!config.use_threads) {
                filename += "_sequencial";
            }
        }
        filename += ".txt";
        
        save_detailed_comparison(all_metrics, filename);
        
        std::cout << "\nLogs detalhados salvos em:\n";
        std::string suffix = (num_cores > 1 && config.use_threads) ? "_multicore" : "";
        std::cout << "   - output/resultados_FCFS" << suffix << ".dat\n";
        std::cout << "   - output/resultados_SJN" << suffix << ".dat\n";
        std::cout << "   - output/resultados_Priority" << suffix << ".dat\n";
        std::cout << "   - output/resultados_RoundRobin" << suffix << ".dat\n";
        std::cout << "   - " << filename << " (Tabela Comparativa Completa)\n\n";
        
        // Se multicore, mostrar métricas por núcleo
        if (num_cores > 1 && config.use_threads) {
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

    // Modo interativo - executar escalonador escolhido
    std::string scheduler_name;
    switch (choice) {
        case 1: 
            scheduler_type = SchedulerType::FCFS; 
            scheduler_name = "FCFS";
            break;
        case 2: 
            scheduler_type = SchedulerType::SJN; 
            scheduler_name = "SJN";
            break;
        case 3: 
            scheduler_type = SchedulerType::Priority; 
            scheduler_name = "Priority";
            break;
        case 4: 
            scheduler_type = SchedulerType::RoundRobin; 
            scheduler_name = "RoundRobin";
            break;
        default:
            std::cerr << "Escolha inválida. Usando FCFS por padrão.\n";
            scheduler_type = SchedulerType::FCFS;
            scheduler_name = "FCFS";
            break;
    }
    
    // Executar usando as mesmas funções do modo CLI para consistência
    std::cout << "\nExecutando " << scheduler_name;
    if (num_cores > 1) {
        std::cout << " (" << num_cores << " cores, " 
                  << (config.use_threads ? "multi-thread" : "sequencial") << ")";
    }
    std::cout << "...\n\n";
    
    SchedulerMetrics metrics;
    // Decidir entre multi-thread ou sequencial
    if (num_cores > 1 && config.use_threads) {
        metrics = run_multicore_scheduler(num_cores, scheduler_type, scheduler_name, true);
    } else {
        // Execução sequencial (mesmo com múltiplos cores logicamente)
        metrics = run_scheduler(scheduler_type, scheduler_name, true);
        metrics.num_cores = num_cores; // Registrar número de cores configurados
    }
    
    std::cout << "\nSimulação concluída!\n";
    std::cout << "Tempo de execução: " << metrics.execution_time_ms << " ms\n";
    std::cout << "Processos finalizados: " << metrics.processes_finished << "\n";
    std::cout << "Context switches: " << metrics.context_switches << "\n";
    std::cout << "Cache hit rate: " << std::fixed << std::setprecision(2) 
              << metrics.cache_hit_rate << "%\n\n";
    
    std::string mode_suffix = (num_cores > 1 && config.use_threads) ? "_multicore" : "";
    std::string output_file = "output/resultados_" + scheduler_name + mode_suffix + ".dat";
    std::cout << "Resultados salvos em: " << output_file << "\n";
    
    return 0;
}
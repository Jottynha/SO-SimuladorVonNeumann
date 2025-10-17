#include <iostream>
#include <vector>
#include <deque>
#include <memory>
#include <thread>
#include <chrono>
#include <filesystem>
#include <fstream>


#include "cpu/PCB.hpp"
#include "cpu/pcb_loader.hpp"
#include "cpu/CONTROL_UNIT.hpp"
#include "memory/MemoryManager.hpp"
#include "parser_json/parser_json.hpp"
#include "IO/IOManager.hpp"
#include "cpu/Scheduler.hpp"

// Função para imprimir as métricas de um processo
void print_metrics(const PCB& pcb, std::ofstream& outFile) {
    outFile << "\n--- METRICAS FINAIS DO PROCESSO " << pcb.pid << " ---\n";
    outFile << "Nome do Processo:       " << pcb.name << "\n";
    outFile << "Estado Final:           " << (pcb.state == State::Finished ? "Finished" : "Incomplete") << "\n";
    outFile << "Prioridade:             " << pcb.priority << "\n";
    outFile << "Quantum:                " << pcb.quantum << "\n";
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


int main() {
    // 1. Escolha do Algoritmo de Escalonamento
    std::cout << "Escolha o algoritmo de escalonamento:\n";
    std::cout << "1. FCFS (First-Come, First-Served)\n";
    std::cout << "2. SJN (Shortest Job Next)\n";
    std::cout << "3. Priority\n";
    std::cout << "Digite sua escolha (1-3): ";
    int choice;
    std::cin >> choice;

    SchedulerType scheduler_type;
    switch (choice) {
        case 1: scheduler_type = SchedulerType::FCFS; break;
        case 2: scheduler_type = SchedulerType::SJN; break;
        case 3: scheduler_type = SchedulerType::Priority; break;
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
    
    // Processo 1: Short
    auto p1 = std::make_unique<PCB>();
    if (load_pcb_from_json("process_short.json", *p1)) {
        std::cout << "Carregando programa 'tasks_short.json' para o processo " << p1->pid << "...\n";
        loadJsonProgram("tasks_short.json", memManager, *p1, 0);
        process_list.push_back(std::move(p1));
    } else {
        std::cerr << "Erro ao carregar 'process_short.json'.\n";
    }

    // Processo 2: Long
    auto p2 = std::make_unique<PCB>();
    if (load_pcb_from_json("process_long.json", *p2)) {
        std::cout << "Carregando programa 'tasks_long.json' para o processo " << p2->pid << "...\n";
        loadJsonProgram("tasks_long.json", memManager, *p2, 2048);
        process_list.push_back(std::move(p2));
    } else {
        std::cerr << "Erro ao carregar 'process_long.json'.\n";
    }

    // Processo 3: CPU-Bound
    auto p3 = std::make_unique<PCB>();
    if (load_pcb_from_json("process_cpu_bound.json", *p3)) {
        std::cout << "Carregando programa 'tasks_cpu_bound.json' para o processo " << p3->pid << "...\n";
        loadJsonProgram("tasks_cpu_bound.json", memManager, *p3, 4096);
        process_list.push_back(std::move(p3));
    } else {
        std::cerr << "Erro ao carregar 'process_cpu_bound.json'.\n";
    }

    // Processo 4: IO-Bound
    auto p4 = std::make_unique<PCB>();
    if (load_pcb_from_json("process_io_bound.json", *p4)) {
        std::cout << "Carregando programa 'tasks_io_bound.json' para o processo " << p4->pid << "...\n";
        loadJsonProgram("tasks_io_bound.json", memManager, *p4, 6144);
        process_list.push_back(std::move(p4));
    } else {
        std::cerr << "Erro ao carregar 'process_io_bound.json'.\n";
    }

    // Adiciona os processos ao escalonador
    for (const auto& process : process_list) {
        scheduler.add_process(process.get());
    }

    int total_processes = process_list.size();
    int finished_processes = 0;
    std::vector<PCB*> blocked_list;

    // Abre o arquivo de resultados uma vez
    std::filesystem::create_directory("output");
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
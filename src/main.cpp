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

// Arquivo de saída global para consolidar os resultados
std::ofstream global_output_file;

// Função para imprimir as métricas de um processo
void print_metrics(const PCB& pcb, SchedulingPolicyType policy) {
    global_output_file << "\n--- METRICAS FINAIS DO PROCESSO " << pcb.pid << " (Escalonador: ";
    switch (policy) {
        case SchedulingPolicyType::FCFS: global_output_file << "FCFS"; break;
        case SchedulingPolicyType::SJN: global_output_file << "SJN"; break;
        case SchedulingPolicyType::RR: global_output_file << "RR"; break;
        case SchedulingPolicyType::Priority: global_output_file << "Priority"; break;
    }
    global_output_file << ") ---\n";
    global_output_file << "Nome do Processo:       " << pcb.name << "\n";
    global_output_file << "Estado Final:           " << (pcb.state == State::Finished ? "Finished" : "Incomplete") << "\n";
    global_output_file << "Ciclos de Pipeline:     " << pcb.pipeline_cycles.load() << "\n";
    global_output_file << "Total de Acessos a Mem: " << pcb.mem_accesses_total.load() << "\n";
    global_output_file << "  - Leituras:             " << pcb.mem_reads.load() << "\n";
    global_output_file << "  - Escritas:             " << pcb.mem_writes.load() << "\n";
    global_output_file << "Acessos a Cache L1:     " << pcb.cache_mem_accesses.load() << "\n";
    global_output_file << "Acessos a Mem Principal:" << pcb.primary_mem_accesses.load() << "\n";
    global_output_file << "Acessos a Mem Secundaria:" << pcb.secondary_mem_accesses.load() << "\n";
    global_output_file << "Ciclos Totais de Memoria: " << pcb.memory_cycles.load() << "\n";
    global_output_file << "Ciclos de IO:           " << pcb.io_cycles.load() << "\n";
    global_output_file << "------------------------------------------\n";
    global_output_file << "Log de Execucao:\n";
    for (const auto& log_entry : pcb.execution_log) {
        global_output_file << "  - " << log_entry << "\n";
    }
    global_output_file << "------------------------------------------\n\n";
}

void run_simulation(SchedulingPolicyType selected_policy) {
    // 1. Inicialização dos Módulos Principais
    MemoryManager memManager(10240, 8192); // Aumenta a memória principal
    IOManager ioManager;

    std::cout << "Inicializando o simulador...\n";

    // 2. Carregamento dos Processos
    std::vector<std::unique_ptr<PCB>> process_list;

    // Carrega um processo a partir de um arquivo JSON
    auto p1 = std::make_unique<PCB>();
    if (load_pcb_from_json("process_short.json", *p1)) {
        std::cout << "Carregando programa '" << p1->program << "' para o processo " << p1->pid << "...\n";
        p1->base_address = 0; // Define o endereço base
        p1->priority = 10; // Prioridade alta
        loadJsonProgram(p1->program, memManager, *p1, p1->base_address);
        process_list.push_back(std::move(p1));
    } else {
        std::cerr << "Erro ao carregar 'process_short.json'.\n";
        return;
    }

    auto p2 = std::make_unique<PCB>();
    if (load_pcb_from_json("process_long.json", *p2)) {
        std::cout << "Carregando programa '" << p2->program << "' para o processo " << p2->pid << "...\n";
        p2->base_address = 1024; // Define o endereço base
        p2->priority = 20; // Prioridade baixa
        loadJsonProgram(p2->program, memManager, *p2, p2->base_address);
        process_list.push_back(std::move(p2));
    } else {
        std::cerr << "Erro ao carregar 'process_long.json'.\n";
        return;
    }

    // Carrega o processo CPU-Bound
    auto p3 = std::make_unique<PCB>();
    if (load_pcb_from_json("process_cpu_bound.json", *p3)) {
        std::cout << "Carregando programa '" << p3->program << "' para o processo " << p3->pid << "...\n";
        p3->base_address = 2048; // Endereço base para o novo processo
        p3->priority = 15; // Prioridade média
        loadJsonProgram(p3->program, memManager, *p3, p3->base_address);
        process_list.push_back(std::move(p3));
    } else {
        std::cerr << "Erro ao carregar 'process_cpu_bound.json'.\n";
        return;
    }

    // Carrega o processo IO-Bound
    auto p4 = std::make_unique<PCB>();
    if (load_pcb_from_json("process_io_bound.json", *p4)) {
        std::cout << "Carregando programa '" << p4->program << "' para o processo " << p4->pid << "...\n";
        p4->base_address = 3072; // Endereço base para o novo processo
        p4->priority = 5; // Prioridade mais alta
        loadJsonProgram(p4->program, memManager, *p4, p4->base_address);
        process_list.push_back(std::move(p4));
    } else {
        std::cerr << "Erro ao carregar 'process_io_bound.json'.\n";
        return;
    }

    // A política de escalonamento agora é passada como argumento e não é mais sobrescrita aqui.

    // 3. Criação e Loop Principal do Escalonador
    Scheduler scheduler(selected_policy, process_list);
    
    global_output_file << "\n======================================================\n";
    global_output_file << "INICIANDO SIMULACAO COM ESCALONADOR: ";
    scheduler.get_policy()->print_type_to_stream(global_output_file);
    global_output_file << "\n======================================================\n\n";

    while (!scheduler.has_finished()) {
        scheduler.check_blocked_processes();
        PCB* current_process = scheduler.get_next_process();

        if (current_process == nullptr) {
            if (scheduler.has_finished()) break;
            // Se não há processos prontos, mas ainda há processos bloqueados ou IO em andamento,
            // a simulação deve continuar.
            if (ioManager.is_busy() || scheduler.has_blocked_processes()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1)); // Pequena pausa para não sobrecarregar a CPU
                continue;
            } else {
                // Se não há processos prontos, mas ainda há processos bloqueados,
                // simula a passagem do tempo no IOManager para que eles possam ser desbloqueados.
                if (scheduler.has_blocked_processes()) {
                    ioManager.advance_time(50); // Avança o tempo em 50ms
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                } else {
                     // Se não há processos prontos nem bloqueados, mas o escalonador não terminou,
                     // pode ser um estado de transição. Apenas espere um pouco.
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
                continue;
            }
        }

        current_process->state = State::Running;
        std::vector<std::unique_ptr<IORequest>> io_requests;
        bool print_lock = false;

        Core(memManager, *current_process, &io_requests, print_lock);

        switch (current_process->state) {
            case State::Blocked:
                ioManager.registerProcessWaitingForIO(current_process);
                scheduler.add_to_blocked_list(current_process);
                break;
            case State::Finished:
                print_metrics(*current_process, selected_policy);
                scheduler.increment_finished_processes();
                break;
            case State::Running: // O processo usou seu quantum, mas não terminou
                if (selected_policy == SchedulingPolicyType::RR || selected_policy == SchedulingPolicyType::Priority) {
                    current_process->state = State::Ready;
                    scheduler.add_to_ready_queue(current_process);
                } else {
                    // Para FCFS e SJN, se saiu do Core como Running, algo está errado,
                    // mas por segurança, vamos considerar finalizado para evitar loops.
                    current_process->state = State::Finished;
                    print_metrics(*current_process, selected_policy);
                    scheduler.increment_finished_processes();
                }
                break;
            default:
                 // Se o estado for Ready (ou outro inesperado), e for RR,
                 // devolve para a fila para evitar perdê-lo.
                 if (selected_policy == SchedulingPolicyType::RR || selected_policy == SchedulingPolicyType::Priority) {
                    current_process->state = State::Ready;
                    scheduler.add_to_ready_queue(current_process);
                }
                break;
        }
    }
    global_output_file << "FIM DA SIMULACAO (" << scheduler.get_policy()->get_type_as_string() << ")\n";
}

int main() {
    int choice;
    SchedulingPolicyType selected_policy;

    std::cout << "Escolha o algoritmo de escalonamento:\n";
    std::cout << "1. FCFS (First-Come, First-Served)\n";
    std::cout << "2. SJN (Shortest Job Next)\n";
    std::cout << "3. Priority\n";
    std::cout << "Digite sua escolha (1-3): ";
    std::cin >> choice;

    switch (choice) {
        case 1:
            selected_policy = SchedulingPolicyType::FCFS;
            break;
        case 2:
            selected_policy = SchedulingPolicyType::SJN;
            break;
        case 3:
            selected_policy = SchedulingPolicyType::Priority;
            break;
        default:
            std::cerr << "Escolha inválida. Usando FCFS por padrão.\n";
            selected_policy = SchedulingPolicyType::FCFS;
            break;
    }

    std::filesystem::create_directory("output");
    global_output_file.open("output/resultados.dat");

    if (!global_output_file.is_open()) {
        std::cerr << "Erro ao abrir o arquivo de saída 'output/resultados.dat'.\n";
        return 1;
    }

    run_simulation(selected_policy);

    std::cout << "\nSimulação finalizada. Resultados salvos em 'output/resultados.dat'.\n";
    global_output_file.close();
    
    return 0;
}
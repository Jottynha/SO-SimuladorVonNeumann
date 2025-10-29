#ifndef MEMORY_USAGE_TRACKER_HPP
#define MEMORY_USAGE_TRACKER_HPP

#include <vector>
#include <string>
#include <fstream>
#include <iomanip>
#include <chrono>
#include "../cpu/PCB.hpp"

class MemoryUsageTracker {
public:
    // Registra um snapshot da utilização de memória
    static void recordSnapshot(PCB& process, uint64_t cache_usage, uint64_t ram_usage) {
        PCB::MemorySnapshot snapshot;
        snapshot.timestamp = std::chrono::high_resolution_clock::now();
        snapshot.cache_usage = cache_usage;
        snapshot.ram_usage = ram_usage;
        snapshot.total_accesses = process.mem_accesses_total.load();
        
        uint64_t total_cache_ops = process.cache_hits.load() + process.cache_misses.load();
        snapshot.cache_hit_rate = (total_cache_ops > 0) ? 
            (100.0 * process.cache_hits.load() / total_cache_ops) : 0.0;
        
        process.memory_usage_timeline.push_back(snapshot);
    }
    
    // Gera relatório de utilização de memória ao longo do tempo
    static void generateReport(const PCB& process, const std::string& filename) {
        std::ofstream outFile(filename);
        if (!outFile.is_open()) {
            return;
        }
        
        outFile << "========================================\n";
        outFile << "  RELATORIO DE UTILIZACAO DE MEMORIA  \n";
        outFile << "  Processo: " << process.name << " (PID: " << process.pid << ")\n";
        outFile << "========================================\n\n";
        
        if (process.memory_usage_timeline.empty()) {
            outFile << "Nenhum snapshot de memoria registrado.\n";
            outFile.close();
            return;
        }
        
        // Cabeçalho da tabela
        outFile << std::left << std::setw(12) << "Tempo(ms)"
                << std::right << std::setw(15) << "Cache(bytes)"
                << std::setw(15) << "RAM(bytes)"
                << std::setw(15) << "Total Acess"
                << std::setw(18) << "Cache Hit(%)"
                << "\n";
        outFile << std::string(75, '-') << "\n";
        
        // Dados
        auto start_time = process.arrival_time;
        for (const auto& snapshot : process.memory_usage_timeline) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                snapshot.timestamp - start_time);
            
            outFile << std::left << std::setw(12) << elapsed.count()
                    << std::right << std::setw(15) << snapshot.cache_usage
                    << std::setw(15) << snapshot.ram_usage
                    << std::setw(15) << snapshot.total_accesses
                    << std::fixed << std::setprecision(2)
                    << std::setw(18) << snapshot.cache_hit_rate
                    << "\n";
        }
        
        outFile << "\n";
        outFile << "=== ESTATISTICAS FINAIS ===\n";
        outFile << "Total de snapshots: " << process.memory_usage_timeline.size() << "\n";
        outFile << "Memoria cache maxima: " << getMaxCacheUsage(process) << " bytes\n";
        outFile << "Memoria RAM maxima: " << getMaxRAMUsage(process) << " bytes\n";
        outFile << "Taxa de cache final: " << std::fixed << std::setprecision(2) 
                << process.memory_usage_timeline.back().cache_hit_rate << "%\n";
        
        outFile.close();
    }
    
    // Gera relatório agregado para múltiplos processos
    static void generateAggregatedReport(const std::vector<PCB*>& processes, 
                                        const std::string& filename) {
        std::ofstream outFile(filename);
        if (!outFile.is_open()) {
            return;
        }
        
        outFile << "================================================================\n";
        outFile << "     RELATORIO AGREGADO DE UTILIZACAO DE MEMORIA (SISTEMA)    \n";
        outFile << "================================================================\n\n";
        
        outFile << std::left << std::setw(8) << "PID"
                << std::setw(20) << "Nome"
                << std::right << std::setw(15) << "Snapshots"
                << std::setw(18) << "Cache Max(B)"
                << std::setw(18) << "RAM Max(B)"
                << std::setw(18) << "Hit Rate Final"
                << "\n";
        outFile << std::string(95, '-') << "\n";
        
        for (const auto* proc : processes) {
            if (proc->memory_usage_timeline.empty()) continue;
            
            outFile << std::left << std::setw(8) << proc->pid
                    << std::setw(20) << proc->name
                    << std::right << std::setw(15) << proc->memory_usage_timeline.size()
                    << std::setw(18) << getMaxCacheUsage(*proc)
                    << std::setw(18) << getMaxRAMUsage(*proc)
                    << std::fixed << std::setprecision(2)
                    << std::setw(18) << proc->memory_usage_timeline.back().cache_hit_rate << "%"
                    << "\n";
        }
        
        outFile << "\n=== RESUMO GERAL ===\n";
        outFile << "Total de processos monitorados: " << processes.size() << "\n";
        outFile << "Memoria cache total do sistema: 16 blocos (configurado)\n";
        outFile << "Memoria RAM total do sistema: 8192 bytes (configurado)\n";
        
        outFile.close();
    }

private:
    static uint64_t getMaxCacheUsage(const PCB& process) {
        uint64_t max_usage = 0;
        for (const auto& snapshot : process.memory_usage_timeline) {
            if (snapshot.cache_usage > max_usage) {
                max_usage = snapshot.cache_usage;
            }
        }
        return max_usage;
    }
    
    static uint64_t getMaxRAMUsage(const PCB& process) {
        uint64_t max_usage = 0;
        for (const auto& snapshot : process.memory_usage_timeline) {
            if (snapshot.ram_usage > max_usage) {
                max_usage = snapshot.ram_usage;
            }
        }
        return max_usage;
    }
};

#endif // MEMORY_USAGE_TRACKER_HPP

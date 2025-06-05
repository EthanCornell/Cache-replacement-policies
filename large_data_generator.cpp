/*
 * -----------------------------------------------------------------------------
 *  Large-Scale Test Data Generator for Cache Replacement Policies
 * -----------------------------------------------------------------------------
 *
 *  Generates various workload patterns with 20M+ page references to stress
 *  test cache replacement algorithms under realistic conditions.
 *
 *  Compile: g++ -std=c++20 -O3 -o data_generator large_data_generator.cpp
 *  Usage: ./data_generator [workload_type] [output_file] [size_mb]
 *
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <chrono>
#include <string>
#include <unordered_set>
#include <cmath>
#include <algorithm>
#include <iomanip>

class LargeDataGenerator {
private:
    std::mt19937 rng;
    std::ofstream outFile;
    int processId = 1;

public:
    LargeDataGenerator() : rng(std::chrono::steady_clock::now().time_since_epoch().count()) {}

    void generateWorkload(const std::string& type, const std::string& filename, int sizeMB) {
        int numReferences = sizeMB * 1024 * 1024 / 8; // ~8 bytes per reference line
        
        outFile.open(filename);
        if (!outFile.is_open()) {
            std::cerr << "Error: Cannot create output file " << filename << std::endl;
            return;
        }

        std::cout << "Generating " << type << " workload...\n";
        std::cout << "Target size: " << sizeMB << "MB (~" << numReferences << " references)\n";
        std::cout << "Output file: " << filename << "\n";

        auto startTime = std::chrono::high_resolution_clock::now();

        if (type == "sequential") {
            generateSequentialWorkload(numReferences);
        } else if (type == "random") {
            generateRandomWorkload(numReferences);
        } else if (type == "locality") {
            generateLocalityWorkload(numReferences);
        } else if (type == "temporal") {
            generateTemporalWorkload(numReferences);
        } else if (type == "mixed") {
            generateMixedWorkload(numReferences);
        } else if (type == "adversarial") {
            generateAdversarialWorkload(numReferences);
        } else if (type == "realistic") {
            generateRealisticWorkload(numReferences);
        } else if (type == "stress") {
            generateStressTestWorkload(numReferences);
        } else {
            std::cerr << "Unknown workload type: " << type << std::endl;
            outFile.close();
            return;
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double>(endTime - startTime);

        outFile.close();

        // Get file size
        std::ifstream file(filename, std::ios::binary | std::ios::ate);
        double fileSizeMB = file.tellg() / (1024.0 * 1024.0);
        file.close();

        std::cout << "✓ Generated successfully!\n";
        std::cout << "  File size: " << std::fixed << std::setprecision(2) << fileSizeMB << " MB\n";
        std::cout << "  Generation time: " << std::fixed << std::setprecision(2) << duration.count() << " seconds\n";
        std::cout << "  References: " << numReferences << "\n\n";
    }

private:
    void generateSequentialWorkload(int numReferences) {
        std::cout << "Pattern: Sequential access with wraparound\n";
        
        const int maxPage = 10000;
        int currentPage = 0;
        
        for (int i = 0; i < numReferences; ++i) {
            outFile << processId << " " << currentPage << "\n";
            currentPage = (currentPage + 1) % maxPage;
            
            if (i % 1000000 == 0 && i > 0) {
                std::cout << "  Generated " << i / 1000000 << "M references...\n";
            }
        }
    }

    void generateRandomWorkload(int numReferences) {
        std::cout << "Pattern: Uniform random access\n";
        
        const int maxPage = 50000;
        std::uniform_int_distribution<> pageDist(0, maxPage - 1);
        
        for (int i = 0; i < numReferences; ++i) {
            int page = pageDist(rng);
            outFile << processId << " " << page << "\n";
            
            if (i % 1000000 == 0 && i > 0) {
                std::cout << "  Generated " << i / 1000000 << "M references...\n";
            }
        }
    }

    void generateLocalityWorkload(int numReferences) {
        std::cout << "Pattern: Spatial and temporal locality (80/20 rule)\n";
        
        const int totalPages = 100000;
        const int hotSetSize = totalPages / 5; // 20% of pages are "hot"
        
        std::uniform_real_distribution<> accessDist(0.0, 1.0);
        std::uniform_int_distribution<> hotPageDist(0, hotSetSize - 1);
        std::uniform_int_distribution<> coldPageDist(hotSetSize, totalPages - 1);
        
        for (int i = 0; i < numReferences; ++i) {
            int page;
            if (accessDist(rng) < 0.8) {
                // 80% of accesses go to 20% of pages (hot set)
                page = hotPageDist(rng);
            } else {
                // 20% of accesses go to 80% of pages (cold set)
                page = coldPageDist(rng);
            }
            
            outFile << processId << " " << page << "\n";
            
            if (i % 1000000 == 0 && i > 0) {
                std::cout << "  Generated " << i / 1000000 << "M references...\n";
            }
        }
    }

    void generateTemporalWorkload(int numReferences) {
        std::cout << "Pattern: Temporal locality with working set changes\n";
        
        const int totalPages = 80000;
        const int workingSetSize = 1000;
        const int phaseLength = 100000; // Change working set every 100k references
        
        std::uniform_int_distribution<> pageSelectDist(0, totalPages - workingSetSize);
        
        for (int i = 0; i < numReferences; ++i) {
            // Change working set periodically
            int phase = i / phaseLength;
            int workingSetStart = (phase * 1000) % (totalPages - workingSetSize);
            
            // 90% references within current working set
            std::uniform_real_distribution<> accessTypeDist(0.0, 1.0);
            int page;
            
            if (accessTypeDist(rng) < 0.9) {
                // Access within working set
                std::uniform_int_distribution<> workingSetDist(
                    workingSetStart, workingSetStart + workingSetSize - 1);
                page = workingSetDist(rng);
            } else {
                // Random access outside working set
                std::uniform_int_distribution<> outsideDist(0, totalPages - 1);
                do {
                    page = outsideDist(rng);
                } while (page >= workingSetStart && page < workingSetStart + workingSetSize);
            }
            
            outFile << processId << " " << page << "\n";
            
            if (i % 1000000 == 0 && i > 0) {
                std::cout << "  Generated " << i / 1000000 << "M references...\n";
            }
        }
    }

    void generateMixedWorkload(int numReferences) {
        std::cout << "Pattern: Mixed sequential, random, and locality patterns\n";
        
        const int totalPages = 60000;
        int currentSeqPage = 0;
        
        std::uniform_real_distribution<> patternDist(0.0, 1.0);
        std::uniform_int_distribution<> randomPageDist(0, totalPages - 1);
        std::uniform_int_distribution<> localityPageDist(0, totalPages / 10);
        
        for (int i = 0; i < numReferences; ++i) {
            int page;
            double pattern = patternDist(rng);
            
            if (pattern < 0.4) {
                // 40% sequential access
                page = currentSeqPage;
                currentSeqPage = (currentSeqPage + 1) % totalPages;
            } else if (pattern < 0.7) {
                // 30% locality access (first 10% of pages)
                page = localityPageDist(rng);
            } else {
                // 30% random access
                page = randomPageDist(rng);
            }
            
            outFile << processId << " " << page << "\n";
            
            if (i % 1000000 == 0 && i > 0) {
                std::cout << "  Generated " << i / 1000000 << "M references...\n";
            }
        }
    }

    void generateAdversarialWorkload(int numReferences) {
        std::cout << "Pattern: Adversarial for specific algorithms\n";
        
        // This pattern is designed to be bad for LRU and FIFO
        const int cacheSize = 1000; // Assume typical cache size
        const int patternSize = cacheSize + 1;
        
        for (int i = 0; i < numReferences; ++i) {
            // Cyclic pattern that's larger than typical cache size
            int page = i % patternSize;
            outFile << processId << " " << page << "\n";
            
            if (i % 1000000 == 0 && i > 0) {
                std::cout << "  Generated " << i / 1000000 << "M references...\n";
            }
        }
    }

    void generateRealisticWorkload(int numReferences) {
        std::cout << "Pattern: Realistic multi-process simulation\n";
        
        const int numProcesses = 8;
        const int pagesPerProcess = 10000;
        
        std::uniform_int_distribution<> processDist(1, numProcesses);
        std::uniform_int_distribution<> pageBaseDist(0, pagesPerProcess - 1);
        std::normal_distribution<> localityDist(0.0, 100.0);
        
        for (int i = 0; i < numReferences; ++i) {
            int proc = processDist(rng);
            int baseAddr = proc * pagesPerProcess;
            
            // Add some locality around the base address
            int offset = static_cast<int>(std::abs(localityDist(rng))) % pagesPerProcess;
            int page = baseAddr + offset;
            
            outFile << proc << " " << page << "\n";
            
            if (i % 1000000 == 0 && i > 0) {
                std::cout << "  Generated " << i / 1000000 << "M references...\n";
            }
        }
    }

    void generateStressTestWorkload(int numReferences) {
        std::cout << "Pattern: Stress test with all patterns combined\n";
        
        const int totalPages = 200000;
        const int segmentSize = numReferences / 10; // 10 different segments
        
        for (int segment = 0; segment < 10; ++segment) {
            std::cout << "  Generating segment " << (segment + 1) << "/10...\n";
            
            for (int i = 0; i < segmentSize; ++i) {
                int globalIndex = segment * segmentSize + i;
                int page;
                
                switch (segment % 5) {
                    case 0: // Sequential
                        page = (globalIndex % 5000);
                        break;
                    case 1: // Random
                        page = std::uniform_int_distribution<>(0, totalPages - 1)(rng);
                        break;
                    case 2: // Locality
                        page = std::uniform_int_distribution<>(0, totalPages / 20)(rng);
                        break;
                    case 3: // Adversarial
                        page = globalIndex % 1001;
                        break;
                    case 4: // Mixed
                        if (i % 3 == 0) page = i % 1000;
                        else if (i % 3 == 1) page = std::uniform_int_distribution<>(1000, 5000)(rng);
                        else page = std::uniform_int_distribution<>(0, totalPages - 1)(rng);
                        break;
                }
                
                outFile << processId << " " << page << "\n";
            }
        }
    }
};

void printUsage(const char* programName) {
    std::cout << "Large-Scale Cache Test Data Generator\n";
    std::cout << "=====================================\n\n";
    std::cout << "Usage: " << programName << " <workload_type> <output_file> <size_mb>\n\n";
    std::cout << "Workload Types:\n";
    std::cout << "  sequential   - Sequential page access with wraparound\n";
    std::cout << "  random       - Uniform random page access\n";
    std::cout << "  locality     - 80/20 locality pattern (realistic)\n";
    std::cout << "  temporal     - Temporal locality with working set changes\n";
    std::cout << "  mixed        - Mix of sequential, random, and locality\n";
    std::cout << "  adversarial  - Worst-case pattern for most algorithms\n";
    std::cout << "  realistic    - Multi-process realistic simulation\n";
    std::cout << "  stress       - Comprehensive stress test (all patterns)\n\n";
    std::cout << "Parameters:\n";
    std::cout << "  size_mb      - Target file size in megabytes (20+ recommended)\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << programName << " locality workload_20m.txt 20\n";
    std::cout << "  " << programName << " stress stress_test_50m.txt 50\n";
    std::cout << "  " << programName << " realistic real_world_100m.txt 100\n\n";
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        printUsage(argv[0]);
        return 1;
    }

    std::string workloadType = argv[1];
    std::string outputFile = argv[2];
    int sizeMB = std::stoi(argv[3]);

    if (sizeMB < 1) {
        std::cerr << "Error: Size must be at least 1 MB\n";
        return 1;
    }

    std::vector<std::string> validTypes = {
        "sequential", "random", "locality", "temporal", 
        "mixed", "adversarial", "realistic", "stress"
    };

    if (std::find(validTypes.begin(), validTypes.end(), workloadType) == validTypes.end()) {
        std::cerr << "Error: Invalid workload type '" << workloadType << "'\n";
        printUsage(argv[0]);
        return 1;
    }

    std::cout << "Large-Scale Cache Replacement Test Data Generator\n";
    std::cout << "=================================================\n\n";

    LargeDataGenerator generator;
    generator.generateWorkload(workloadType, outputFile, sizeMB);

    std::cout << "Next steps:\n";
    std::cout << "1. Test with your cache simulator:\n";
    std::cout << "   ./cache_simulator " << outputFile << " a 64 0 0\n";
    std::cout << "2. Compare algorithm performance under heavy load\n";
    std::cout << "3. Monitor memory usage and execution time\n\n";

    return 0;
}
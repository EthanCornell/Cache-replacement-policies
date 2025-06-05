/*
 * Fixed Memory-Efficient Large Scale Cache Testing
 * 
 * This version works with the existing CacheReplacementSimulator
 * by creating temporary files instead of trying to modify the class.
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <chrono>
#include <iomanip>
#include <string>
#include <cstdlib>
#include <functional>

class WorkingMemoryTester {
private:
    std::mt19937 rng;

public:
    WorkingMemoryTester() : rng(std::chrono::steady_clock::now().time_since_epoch().count()) {}

    // Generate different workload patterns
    int generateLocalityPattern(int index) {
        // 80/20 rule: 80% of accesses to 20% of pages
        // Hot set: pages 0-999, Cold set: pages 1000-9999
        std::uniform_real_distribution<> dist(0.0, 1.0);
        if (dist(rng) < 0.8) {
            // Hot set access (20% of total pages, 80% of accesses)
            return std::uniform_int_distribution<>(0, 999)(rng);
        } else {
            // Cold set access (80% of total pages, 20% of accesses)
            return std::uniform_int_distribution<>(1000, 9999)(rng);
        }
    }

    int generateSequentialPattern(int index) {
        return index % 10000;  // Sequential with wraparound
    }

    int generateRandomPattern(int index) {
        return std::uniform_int_distribution<>(0, 19999)(rng);  // Uniform random
    }

    int generateAdversarialPattern(int index, int cacheSize = 64) {
        // Cyclic pattern designed to cause maximum cache misses
        return index % (cacheSize + 1);
    }

    int generateTemporalPattern(int index) {
        // Working set changes every 100k references
        int phase = index / 100000;
        int workingSetStart = (phase * 500) % 8000;
        
        std::uniform_real_distribution<> dist(0.0, 1.0);
        if (dist(rng) < 0.9) {
            // 90% within current working set (500 pages)
            return workingSetStart + (index % 500);
        } else {
            // 10% outside working set
            return std::uniform_int_distribution<>(0, 15000)(rng);
        }
    }

    void createTestFile(const std::string& filename, int numReferences, 
                       std::function<int(int)> patternGenerator,
                       const std::string& patternName) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot create file: " + filename);
        }

        std::cout << "Generating " << numReferences/1000000 << "M " << patternName 
                  << " references... ";
        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < numReferences; ++i) {
            int page = patternGenerator(i);
            file << "1 " << page << "\n";

            if (i % 2000000 == 0 && i > 0) {
                std::cout << i/1000000 << "M ";
                std::cout.flush();
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double>(end - start);
        std::cout << "Done (" << std::fixed << std::setprecision(2) 
                  << duration.count() << "s)\n";
        
        file.close();
    }

    void runAlgorithmTest(const std::string& dataFile, const std::string& algorithmName, 
                         char algorithmCode, int numFrames, bool showDetails = true) {
        if (showDetails) {
            std::cout << "  Testing " << algorithmName << " (" << numFrames << " frames)... ";
        }

        std::string command = "./cache_simulator \"" + dataFile + "\" " + 
                             std::string(1, algorithmCode) + " " + std::to_string(numFrames) + 
                             " 0 0 2>/dev/null";

        auto start = std::chrono::high_resolution_clock::now();
        int result = system(command.c_str());
        auto end = std::chrono::high_resolution_clock::now();

        auto duration = std::chrono::duration<double>(end - start);

        if (showDetails) {
            if (result == 0) {
                std::cout << "✓ (" << std::fixed << std::setprecision(3) 
                          << duration.count() << "s)\n";
            } else {
                std::cout << "✗ Failed\n";
            }
        }
    }

    void runVerboseTest(const std::string& dataFile, const std::string& algorithmName,
                       char algorithmCode, int numFrames) {
        std::cout << "\n" << algorithmName << " with " << numFrames << " frames:\n";
        std::cout << std::string(50, '-') << "\n";
        
        std::string command = "./cache_simulator \"" + dataFile + "\" " + 
                             std::string(1, algorithmCode) + " " + std::to_string(numFrames) + 
                             " 0 0";
        system(command.c_str());
    }

    void performanceTest() {
        std::cout << "\n=== Performance Test (10M References) ===\n";
        
        const std::string testFile = "temp_perf_test.txt";
        const int numReferences = 10000000;
        
        // Generate locality workload
        auto localityPattern = [this](int i) { return generateLocalityPattern(i); };
        createTestFile(testFile, numReferences, localityPattern, "locality");
        
        std::cout << "\nTesting algorithms with different cache sizes:\n";
        std::vector<int> sizes = {32, 64, 128, 256};
        std::vector<std::pair<std::string, char>> algorithms = {
            {"LRU", 'L'}, {"FIFO", 'F'}, {"CLOCK", 'C'}, {"RANDOM", 'R'}
        };
        
        for (int size : sizes) {
            std::cout << "\nCache size: " << size << " frames\n";
            for (const auto& [name, code] : algorithms) {
                runAlgorithmTest(testFile, name, code, size);
            }
        }
        
        std::cout << "\nDetailed results for 64 frames:\n";
        runVerboseTest(testFile, "LRU", 'L', 64);
        
        std::remove(testFile.c_str());
        std::cout << "\nPerformance test completed!\n";
    }

    void massiveTest() {
        std::cout << "\n=== Massive Scale Test (20M References) ===\n";
        
        const int numReferences = 20000000;
        const int cacheSize = 64;
        
        std::vector<std::tuple<std::string, std::function<int(int)>, std::string>> patterns = {
            {"locality", [this](int i) { return generateLocalityPattern(i); }, "Locality (80/20)"},
            {"sequential", [this](int i) { return generateSequentialPattern(i); }, "Sequential"},
            {"random", [this](int i) { return generateRandomPattern(i); }, "Random"},
            {"adversarial", [this](int i) { return generateAdversarialPattern(i, cacheSize); }, "Adversarial"}
        };
        
        std::vector<std::pair<std::string, char>> algorithms = {
            {"LRU", 'L'}, {"FIFO", 'F'}, {"CLOCK", 'C'}, {"RANDOM", 'R'}
        };

        for (const auto& [patternKey, patternGen, patternDescription] : patterns) {
            std::cout << "\n" << patternDescription << " Pattern:\n";
            std::cout << std::string(40, '-') << "\n";
            
            std::string testFile = "temp_massive_" + patternKey + ".txt";
            createTestFile(testFile, numReferences, patternGen, patternKey);
            
            // Test key algorithms with verbose output for first one
            bool first = true;
            for (const auto& [algoName, algoCode] : algorithms) {
                if (first) {
                    runVerboseTest(testFile, algoName, algoCode, cacheSize);
                    first = false;
                } else {
                    runAlgorithmTest(testFile, algoName, algoCode, cacheSize);
                }
            }
            
            std::remove(testFile.c_str());
        }
        
        std::cout << "\nMassive scale test completed!\n";
    }

    void memoryStressTest() {
        std::cout << "\n=== Memory Stress Test (Large Caches) ===\n";
        
        const int numReferences = 1000000;  // 1M references
        const std::string testFile = "temp_memory_test.txt";
        
        // Generate random workload
        auto randomPattern = [this](int i) { return generateRandomPattern(i); };
        createTestFile(testFile, numReferences, randomPattern, "random");
        
        std::cout << "\nTesting large cache sizes:\n";
        std::vector<int> largeSizes = {1000, 5000, 10000};
        
        for (int size : largeSizes) {
            runAlgorithmTest(testFile, "LRU", 'L', size);
        }
        
        std::remove(testFile.c_str());
        std::cout << "\nMemory stress test completed!\n";
    }

    void quickDemo() {
        std::cout << "\n=== Quick Demo (5M References) ===\n";
        
        const int numReferences = 5000000;
        const std::string testFile = "temp_demo.txt";
        
        // Generate locality workload
        auto localityPattern = [this](int i) { return generateLocalityPattern(i); };
        createTestFile(testFile, numReferences, localityPattern, "locality");
        
        std::cout << "\nRunning all algorithms comparison (32 frames):\n";
        runVerboseTest(testFile, "All Algorithms", 'a', 32);
        
        std::remove(testFile.c_str());
        std::cout << "\nQuick demo completed!\n";
    }

    void patternAnalysis() {
        std::cout << "\n=== Pattern Analysis (100k samples each) ===\n";
        
        const int numSamples = 100000;
        
        std::vector<std::tuple<std::string, std::function<int(int)>, std::string>> patterns = {
            {"locality", [this](int i) { return generateLocalityPattern(i); }, "Locality (80/20)"},
            {"sequential", [this](int i) { return generateSequentialPattern(i); }, "Sequential"},
            {"random", [this](int i) { return generateRandomPattern(i); }, "Random"},
            {"temporal", [this](int i) { return generateTemporalPattern(i); }, "Temporal"}
        };
        
        for (const auto& [patternKey, patternGen, patternDescription] : patterns) {
            std::cout << "\n" << patternDescription << " Pattern Analysis:\n";
            std::cout << std::string(30, '-') << "\n";
            
            std::string testFile = "temp_analysis_" + patternKey + ".txt";
            createTestFile(testFile, numSamples, patternGen, patternKey);
            
            // Quick test with multiple cache sizes
            std::vector<int> testSizes = {16, 32, 64, 128};
            for (int size : testSizes) {
                std::cout << "  " << size << " frames: ";
                runAlgorithmTest(testFile, "LRU", 'L', size, false);
            }
            
            std::remove(testFile.c_str());
        }
        
        std::cout << "\nPattern analysis completed!\n";
    }
};

int main(int argc, char* argv[]) {
    std::cout << "Working Memory-Efficient Large Scale Cache Test\n";
    std::cout << "==============================================\n";
    
    // Check if cache_simulator exists
    if (system("test -f ./cache_simulator") != 0) {
        std::cout << "Error: cache_simulator not found.\n";
        std::cout << "Please run 'make' first to build the simulator.\n";
        return 1;
    }
    
    WorkingMemoryTester tester;
    std::string testType = (argc > 1) ? argv[1] : "performance";
    
    try {
        if (testType == "performance") {
            tester.performanceTest();
        } else if (testType == "massive") {
            tester.massiveTest();
        } else if (testType == "memory") {
            tester.memoryStressTest();
        } else if (testType == "demo") {
            tester.quickDemo();
        } else if (testType == "analysis") {
            tester.patternAnalysis();
        } else {
            std::cout << "Usage: " << argv[0] << " [test_type]\n\n";
            std::cout << "Test types:\n";
            std::cout << "  performance - 10M reference performance test\n";
            std::cout << "  massive     - 20M reference massive test\n";
            std::cout << "  memory      - Memory stress test with large caches\n";
            std::cout << "  demo        - Quick 5M reference demo\n";
            std::cout << "  analysis    - Pattern analysis (100k samples)\n";
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
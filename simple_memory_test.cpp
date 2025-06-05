#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <chrono>
#include <iomanip>
#include <string>
#include <cstdlib>

class SimpleTester {
private:
    std::mt19937 rng;
public:
    SimpleTester() : rng(std::chrono::steady_clock::now().time_since_epoch().count()) {}
    
    void generateTestFile(const std::string& filename, int numReferences, const std::string& pattern) {
        std::ofstream file(filename);
        std::cout << "Generating " << numReferences/1000000 << "M " << pattern << " references... ";
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < numReferences; ++i) {
            int page;
            if (pattern == "locality") {
                std::uniform_real_distribution<> dist(0.0, 1.0);
                if (dist(rng) < 0.8) {
                    page = std::uniform_int_distribution<>(0, 9999)(rng);
                } else {
                    page = std::uniform_int_distribution<>(10000, 49999)(rng);
                }
            } else {
                page = std::uniform_int_distribution<>(0, 99999)(rng);
            }
            file << "1 " << page << "\n";
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double>(end - start);
        std::cout << "Done (" << std::fixed << std::setprecision(2) << duration.count() << "s)\n";
        file.close();
    }
    
    void runTest(const std::string& testFile, const std::string& algoName, char code, int frames) {
        std::cout << "  " << algoName << " (" << frames << " frames): ";
        std::string command = "./cache_simulator " + testFile + " " + std::string(1, code) + " " + std::to_string(frames) + " 0 0 2>/dev/null";
        auto start = std::chrono::high_resolution_clock::now();
        int result = system(command.c_str());
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double>(end - start);
        
        if (result == 0) {
            std::cout << "✓ (" << std::fixed << std::setprecision(3) << duration.count() << "s)\n";
        } else {
            std::cout << "✗ Failed\n";
        }
    }
    
    void performanceTest() {
        std::cout << "\n=== Performance Test (10M References) ===\n";
        generateTestFile("temp_perf.txt", 10000000, "locality");
        
        std::vector<int> sizes = {32, 64, 128};
        std::vector<std::pair<std::string, char>> algorithms = {{"LRU", 'L'}, {"FIFO", 'F'}, {"CLOCK", 'C'}};
        
        for (int size : sizes) {
            std::cout << "\nCache size: " << size << " frames\n";
            for (const auto& [name, code] : algorithms) {
                runTest("temp_perf.txt", name, code, size);
            }
        }
        std::remove("temp_perf.txt");
    }
    
    void massiveTest() {
        std::cout << "\n=== Massive Scale Test (20M References) ===\n";
        generateTestFile("temp_massive.txt", 20000000, "locality");
        
        runTest("temp_massive.txt", "LRU", 'L', 64);
        runTest("temp_massive.txt", "FIFO", 'F', 64);
        runTest("temp_massive.txt", "CLOCK", 'C', 64);
        
        std::remove("temp_massive.txt");
    }
};

int main(int argc, char* argv[]) {
    std::cout << "Simple Large-Scale Cache Test\n=============================\n";
    
    if (system("test -f ./cache_simulator") != 0) {
        std::cout << "Error: cache_simulator not found. Run 'make' first.\n";
        return 1;
    }
    
    SimpleTester tester;
    std::string testType = (argc > 1) ? argv[1] : "performance";
    
    if (testType == "performance") {
        tester.performanceTest();
    } else if (testType == "massive") {
        tester.massiveTest();
    } else {
        std::cout << "Usage: " << argv[0] << " [performance|massive]\n";
        return 1;
    }
    return 0;
}

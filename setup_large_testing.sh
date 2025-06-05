#!/bin/bash

# Setup script for large-scale cache testing
echo "Setting up large-scale cache testing..."

# 1. Replace the Makefile
echo "Updating Makefile..."
cat > Makefile << 'EOF'
# Simple Working Makefile for Cache Replacement Simulator
CXX = g++
CXXFLAGS = -std=c++20 -O3 -Wall -Wextra -Wpedantic

MAIN_SRC = main.cpp
IMPL_SRC = CacheReplacementSimulator.cpp
TEST_SRC = test_cache_replacement.cpp
HEADER = CacheReplacementSimulator.hpp

MAIN_TARGET = cache_simulator
TEST_TARGET = test_cache
DATA_GEN_TARGET = data_generator
MEMORY_TEST_TARGET = memory_test

all: $(MAIN_TARGET) $(TEST_TARGET)

$(MAIN_TARGET): $(MAIN_SRC) $(IMPL_SRC) $(HEADER)
	$(CXX) $(CXXFLAGS) -o $(MAIN_TARGET) $(MAIN_SRC) $(IMPL_SRC)

$(TEST_TARGET): $(TEST_SRC) $(IMPL_SRC) $(HEADER)
	$(CXX) $(CXXFLAGS) -o $(TEST_TARGET) $(TEST_SRC) $(IMPL_SRC)

$(DATA_GEN_TARGET): large_data_generator.cpp
	$(CXX) $(CXXFLAGS) -o $(DATA_GEN_TARGET) large_data_generator.cpp

$(MEMORY_TEST_TARGET): simple_memory_test.cpp
	$(CXX) $(CXXFLAGS) -o $(MEMORY_TEST_TARGET) simple_memory_test.cpp

large-data: $(DATA_GEN_TARGET)
	@echo "Data generator ready!"
	@echo "Usage: ./$(DATA_GEN_TARGET) locality test_20m.txt 20"

memory-test: $(MEMORY_TEST_TARGET)
	./$(MEMORY_TEST_TARGET) performance

massive-test: $(MEMORY_TEST_TARGET)
	./$(MEMORY_TEST_TARGET) massive

quick-demo: $(DATA_GEN_TARGET) $(MAIN_TARGET)
	./$(DATA_GEN_TARGET) locality quick_demo.txt 5
	./$(MAIN_TARGET) quick_demo.txt a 32 0 0
	rm -f quick_demo.txt

clean:
	rm -f $(MAIN_TARGET) $(TEST_TARGET) $(DATA_GEN_TARGET) $(MEMORY_TEST_TARGET) *.txt

.PHONY: all large-data memory-test massive-test quick-demo clean
EOF

# 2. Create the simple memory test file
echo "Creating simple memory test..."
cat > simple_memory_test.cpp << 'EOF'
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
EOF

# 3. Build everything
echo "Building tools..."
make clean
make
make large-data
make memory-test

echo ""
echo "✅ Setup completed!"
echo ""
echo "Available commands:"
echo "  make memory-test     # Run 10M reference performance test"  
echo "  make massive-test    # Run 20M reference massive test"
echo "  make large-data      # Build data generator"
echo "  make quick-demo      # Quick 5M reference demo"
echo ""
echo "Manual usage:"
echo "  ./data_generator locality test_20m.txt 20    # Generate 20MB data"
echo "  ./cache_simulator test_20m.txt a 64 0 0      # Test all algorithms"
echo ""
/*
 * -----------------------------------------------------------------------------
 *  Comprehensive Test Suite for Cache Replacement Algorithms
 * -----------------------------------------------------------------------------
 *
 *  This test suite thoroughly tests all cache replacement algorithms with
 *  various scenarios including edge cases, performance tests, and correctness
 *  verification.
 *
 *  Compile with:
 *    g++ -std=c++20 -O2 -Wall -Wextra -o test_cache test_cache_replacement.cpp CacheReplacementSimulator.cpp
 *
 *  Run with:
 *    ./test_cache
 *
 * -----------------------------------------------------------------------------
 */

#include "CacheReplacementSimulator.hpp"
#include <cassert>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <chrono>
#include <iomanip>

class CacheTestSuite {
private:
    int totalTests{0};
    int passedTests{0};
    int failedTests{0};
    
    void reportTest(const std::string& testName, bool passed, const std::string& details = "") {
        totalTests++;
        if (passed) {
            passedTests++;
            std::cout << "[PASS] " << testName << std::endl;
        } else {
            failedTests++;
            std::cout << "[FAIL] " << testName;
            if (!details.empty()) {
                std::cout << " - " << details;
            }
            std::cout << std::endl;
        }
    }

    // Helper function to create test files
    void createTestFile(const std::string& filename, const std::vector<std::pair<int, int>>& references) {
        std::ofstream file(filename);
        for (const auto& ref : references) {
            file << ref.first << " " << ref.second << "\n";
        }
        file.close();
    }

public:
    void runAllTests() {
        std::cout << "=== Cache Replacement Algorithm Test Suite ===" << std::endl;
        std::cout << "Starting comprehensive testing..." << std::endl << std::endl;

        // Basic functionality tests
        testBasicConstruction();
        testConfigurationSettings();
        testFileLoading();
        testAlgorithmSelection();
        
        // Algorithm-specific tests
        testFIFOAlgorithm();
        testLRUAlgorithm();
        testOptimalAlgorithm();
        testClockAlgorithm();
        testRandomAlgorithm();
        testNFUAlgorithm();
        testAgingAlgorithm();
        testMRUAlgorithm();
        testNRUAlgorithm();
        testMFUAlgorithm();
        testLFUAlgorithm();
        testLFRUAlgorithm();
        
        // Edge case tests
        testEdgeCases();
        testStressTests();
        testPerformanceTests();
        
        // Integration tests
        testAllAlgorithmsTogether();
        
        printSummary();
    }

private:
    void testBasicConstruction() {
        std::cout << "--- Testing Basic Construction ---" << std::endl;
        
        try {
            CacheReplacementSimulator simulator;
            reportTest("Basic Constructor", true);
        } catch (const std::exception& e) {
            reportTest("Basic Constructor", false, e.what());
        }
    }

    void testConfigurationSettings() {
        std::cout << "--- Testing Configuration Settings ---" << std::endl;
        
        CacheReplacementSimulator simulator;
        
        // Test valid configurations
        simulator.setConfiguration(4, 100, false, false);
        reportTest("Valid Configuration (4 frames, 100 calls)", simulator.getNumFrames() == 4);
        
        simulator.setConfiguration(1, 50, true, true);
        reportTest("Minimum Configuration (1 frame)", simulator.getNumFrames() == 1);
        
        simulator.setConfiguration(100, 1000, false, true);
        reportTest("Large Configuration (100 frames)", simulator.getNumFrames() == 100);
        
        // Test edge cases
        simulator.setConfiguration(0, 100, false, false);  // Should be clamped to 1
        reportTest("Zero Frames (should clamp to 1)", simulator.getNumFrames() == 1);
        
        simulator.setConfiguration(-5, 100, false, false); // Should be clamped to 1
        reportTest("Negative Frames (should clamp to 1)", simulator.getNumFrames() == 1);
    }

    void testFileLoading() {
        std::cout << "--- Testing File Loading ---" << std::endl;
        
        CacheReplacementSimulator simulator;
        
        // Create test files
        std::vector<std::pair<int, int>> references1 = {{1, 0}, {1, 1}, {1, 2}, {1, 0}, {1, 3}};
        createTestFile("test1.txt", references1);
        
        std::vector<std::pair<int, int>> references2 = {{2, 7}, {2, 0}, {2, 1}, {2, 2}, {2, 0}, {2, 3}, {2, 0}, {2, 4}, {2, 2}, {2, 3}, {2, 0}, {2, 3}, {2, 2}, {2, 1}, {2, 2}, {2, 0}, {2, 1}, {2, 7}, {2, 0}, {2, 1}};
        createTestFile("test2.txt", references2);
        
        bool loaded1 = simulator.loadPageReferences("test1.txt");
        reportTest("Load Small File", loaded1 && simulator.getPageRefs().size() == 5);
        
        bool loaded2 = simulator.loadPageReferences("test2.txt");
        reportTest("Load Larger File", loaded2 && simulator.getPageRefs().size() == 20);
        
        bool loadedInvalid = simulator.loadPageReferences("nonexistent.txt");
        reportTest("Load Nonexistent File", !loadedInvalid);
        
        // Create empty file
        createTestFile("empty.txt", {});
        bool loadedEmpty = simulator.loadPageReferences("empty.txt");
        reportTest("Load Empty File", loadedEmpty && simulator.getPageRefs().size() == 0);
    }

    void testAlgorithmSelection() {
        std::cout << "--- Testing Algorithm Selection ---" << std::endl;
        
        CacheReplacementSimulator simulator;
        
        // Test individual algorithm selection
        char algorithms[] = {'O', 'R', 'F', 'L', 'C', 'N', 'A', 'M', 'n', 'm', 'l', 'f'};
        std::string names[] = {"OPTIMAL", "RANDOM", "FIFO", "LRU", "CLOCK", "NFU", "AGING", "MRU", "NRU", "MFU", "LFU", "LFRU"};
        
        for (int i = 0; i < 12; i++) {
            try {
                simulator.selectAlgorithm(algorithms[i]);
                int selectedCount = 0;
                for (const auto& algo : simulator.getAlgorithms()) {
                    if (algo->selected) selectedCount++;
                }
                reportTest("Select " + names[i], selectedCount == 1);
            } catch (const std::exception& e) {
                reportTest("Select " + names[i], false, e.what());
            }
        }
        
        // Test select all
        simulator.selectAlgorithm('a');
        int allSelectedCount = 0;
        for (const auto& algo : simulator.getAlgorithms()) {
            if (algo->selected) allSelectedCount++;
        }
        reportTest("Select All Algorithms", allSelectedCount == 12);
        
        // Test invalid selection
        try {
            simulator.selectAlgorithm('X');
            reportTest("Invalid Algorithm Selection", false, "Should have thrown exception");
        } catch (const std::exception&) {
            reportTest("Invalid Algorithm Selection", true);
        }
    }

    void testFIFOAlgorithm() {
        std::cout << "--- Testing FIFO Algorithm ---" << std::endl;
        
        // Known FIFO sequence: pages 7,0,1,2,0,3,0,4,2,3,0,3,2,1,2,0,1,7,0,1 with 3 frames
        // Expected: 15 page faults
        std::vector<std::pair<int, int>> fifoTest = {
            {1, 7}, {1, 0}, {1, 1}, {1, 2}, {1, 0}, {1, 3}, {1, 0}, {1, 4}, {1, 2}, {1, 3},
            {1, 0}, {1, 3}, {1, 2}, {1, 1}, {1, 2}, {1, 0}, {1, 1}, {1, 7}, {1, 0}, {1, 1}
        };
        createTestFile("fifo_test.txt", fifoTest);
        
        CacheReplacementSimulator simulator;
        simulator.setConfiguration(3, 20, false, false);
        simulator.loadPageReferences("fifo_test.txt");
        simulator.selectAlgorithm('F');
        simulator.runSimulation();
        
        const auto& algorithms = simulator.getAlgorithms();
        auto fifoAlgo = std::find_if(algorithms.begin(), algorithms.end(),
            [](const auto& algo) { return algo->label == "FIFO"; });
        
        if (fifoAlgo != algorithms.end()) {
            int totalRefs = (*fifoAlgo)->data->hits + (*fifoAlgo)->data->misses;
            reportTest("FIFO Total References", totalRefs == 20);
            reportTest("FIFO Page Faults", (*fifoAlgo)->data->misses == 15);
            reportTest("FIFO Hits", (*fifoAlgo)->data->hits == 5);
        } else {
            reportTest("FIFO Algorithm Found", false);
        }
    }

    void testLRUAlgorithm() {
        std::cout << "--- Testing LRU Algorithm ---" << std::endl;
        
        // Known LRU sequence: same as FIFO test but LRU should perform better
        std::vector<std::pair<int, int>> lruTest = {
            {1, 7}, {1, 0}, {1, 1}, {1, 2}, {1, 0}, {1, 3}, {1, 0}, {1, 4}, {1, 2}, {1, 3},
            {1, 0}, {1, 3}, {1, 2}, {1, 1}, {1, 2}, {1, 0}, {1, 1}, {1, 7}, {1, 0}, {1, 1}
        };
        createTestFile("lru_test.txt", lruTest);
        
        CacheReplacementSimulator simulator;
        simulator.setConfiguration(3, 20, false, false);
        simulator.loadPageReferences("lru_test.txt");
        simulator.selectAlgorithm('L');
        simulator.runSimulation();
        
        const auto& algorithms = simulator.getAlgorithms();
        auto lruAlgo = std::find_if(algorithms.begin(), algorithms.end(),
            [](const auto& algo) { return algo->label == "LRU"; });
        
        if (lruAlgo != algorithms.end()) {
            int totalRefs = (*lruAlgo)->data->hits + (*lruAlgo)->data->misses;
            reportTest("LRU Total References", totalRefs == 20);
            reportTest("LRU Page Faults", (*lruAlgo)->data->misses == 12);  // LRU should have 12 faults
            reportTest("LRU Hits", (*lruAlgo)->data->hits == 8);
        } else {
            reportTest("LRU Algorithm Found", false);
        }
    }

    void testOptimalAlgorithm() {
        std::cout << "--- Testing OPTIMAL Algorithm ---" << std::endl;
        
        // OPTIMAL should perform best - same sequence as above
        std::vector<std::pair<int, int>> optimalTest = {
            {1, 7}, {1, 0}, {1, 1}, {1, 2}, {1, 0}, {1, 3}, {1, 0}, {1, 4}, {1, 2}, {1, 3},
            {1, 0}, {1, 3}, {1, 2}, {1, 1}, {1, 2}, {1, 0}, {1, 1}, {1, 7}, {1, 0}, {1, 1}
        };
        createTestFile("optimal_test.txt", optimalTest);
        
        CacheReplacementSimulator simulator;
        simulator.setConfiguration(3, 20, false, false);
        simulator.loadPageReferences("optimal_test.txt");
        simulator.selectAlgorithm('O');
        simulator.runSimulation();
        
        const auto& algorithms = simulator.getAlgorithms();
        auto optimalAlgo = std::find_if(algorithms.begin(), algorithms.end(),
            [](const auto& algo) { return algo->label == "OPTIMAL"; });
        
        if (optimalAlgo != algorithms.end()) {
            int totalRefs = (*optimalAlgo)->data->hits + (*optimalAlgo)->data->misses;
            reportTest("OPTIMAL Total References", totalRefs == 20);
            reportTest("OPTIMAL Page Faults", (*optimalAlgo)->data->misses == 9);  // OPTIMAL should have 9 faults
            reportTest("OPTIMAL Hits", (*optimalAlgo)->data->hits == 11);
        } else {
            reportTest("OPTIMAL Algorithm Found", false);
        }
    }

    void testClockAlgorithm() {
        std::cout << "--- Testing CLOCK Algorithm ---" << std::endl;
        
        // Simple sequence to test CLOCK's second-chance behavior
        std::vector<std::pair<int, int>> clockTest = {
            {1, 0}, {1, 1}, {1, 2}, {1, 0}, {1, 1}, {1, 3}, {1, 4}, {1, 0}
        };
        createTestFile("clock_test.txt", clockTest);
        
        CacheReplacementSimulator simulator;
        simulator.setConfiguration(3, 8, false, false);
        simulator.loadPageReferences("clock_test.txt");
        simulator.selectAlgorithm('C');
        simulator.runSimulation();
        
        const auto& algorithms = simulator.getAlgorithms();
        auto clockAlgo = std::find_if(algorithms.begin(), algorithms.end(),
            [](const auto& algo) { return algo->label == "CLOCK"; });
        
        if (clockAlgo != algorithms.end()) {
            int totalRefs = (*clockAlgo)->data->hits + (*clockAlgo)->data->misses;
            reportTest("CLOCK Total References", totalRefs == 8);
            reportTest("CLOCK Execution", (*clockAlgo)->data->misses > 0 && (*clockAlgo)->data->hits > 0);
        } else {
            reportTest("CLOCK Algorithm Found", false);
        }
    }

    void testRandomAlgorithm() {
        std::cout << "--- Testing RANDOM Algorithm ---" << std::endl;
        
        std::vector<std::pair<int, int>> randomTest = {
            {1, 0}, {1, 1}, {1, 2}, {1, 3}, {1, 0}, {1, 1}, {1, 2}, {1, 3}
        };
        createTestFile("random_test.txt", randomTest);
        
        CacheReplacementSimulator simulator;
        simulator.setConfiguration(2, 8, false, false);
        simulator.loadPageReferences("random_test.txt");
        simulator.selectAlgorithm('R');
        simulator.runSimulation();
        
        const auto& algorithms = simulator.getAlgorithms();
        auto randomAlgo = std::find_if(algorithms.begin(), algorithms.end(),
            [](const auto& algo) { return algo->label == "RANDOM"; });
        
        if (randomAlgo != algorithms.end()) {
            int totalRefs = (*randomAlgo)->data->hits + (*randomAlgo)->data->misses;
            reportTest("RANDOM Total References", totalRefs == 8);
            reportTest("RANDOM Execution", (*randomAlgo)->data->misses > 0);
        } else {
            reportTest("RANDOM Algorithm Found", false);
        }
    }

    void testNFUAlgorithm() {
        std::cout << "--- Testing NFU Algorithm ---" << std::endl;
        
        // Test NFU with repeated references to see frequency counting
        std::vector<std::pair<int, int>> nfuTest = {
            {1, 0}, {1, 1}, {1, 0}, {1, 0}, {1, 2}, {1, 1}, {1, 0}, {1, 3}
        };
        createTestFile("nfu_test.txt", nfuTest);
        
        CacheReplacementSimulator simulator;
        simulator.setConfiguration(3, 8, false, false);
        simulator.loadPageReferences("nfu_test.txt");
        simulator.selectAlgorithm('N');
        simulator.runSimulation();
        
        const auto& algorithms = simulator.getAlgorithms();
        auto nfuAlgo = std::find_if(algorithms.begin(), algorithms.end(),
            [](const auto& algo) { return algo->label == "NFU"; });
        
        if (nfuAlgo != algorithms.end()) {
            int totalRefs = (*nfuAlgo)->data->hits + (*nfuAlgo)->data->misses;
            reportTest("NFU Total References", totalRefs == 8);
            reportTest("NFU Execution", (*nfuAlgo)->data->misses > 0 && (*nfuAlgo)->data->hits > 0);
        } else {
            reportTest("NFU Algorithm Found", false);
        }
    }

    void testAgingAlgorithm() {
        std::cout << "--- Testing AGING Algorithm ---" << std::endl;
        
        std::vector<std::pair<int, int>> agingTest = {
            {1, 0}, {1, 1}, {1, 2}, {1, 0}, {1, 3}, {1, 1}, {1, 0}, {1, 4}
        };
        createTestFile("aging_test.txt", agingTest);
        
        CacheReplacementSimulator simulator;
        simulator.setConfiguration(3, 8, false, false);
        simulator.loadPageReferences("aging_test.txt");
        simulator.selectAlgorithm('A');
        simulator.runSimulation();
        
        const auto& algorithms = simulator.getAlgorithms();
        auto agingAlgo = std::find_if(algorithms.begin(), algorithms.end(),
            [](const auto& algo) { return algo->label == "AGING"; });
        
        if (agingAlgo != algorithms.end()) {
            int totalRefs = (*agingAlgo)->data->hits + (*agingAlgo)->data->misses;
            reportTest("AGING Total References", totalRefs == 8);
            reportTest("AGING Execution", (*agingAlgo)->data->misses > 0);
        } else {
            reportTest("AGING Algorithm Found", false);
        }
    }

    void testMRUAlgorithm() {
        std::cout << "--- Testing MRU Algorithm ---" << std::endl;
        
        std::vector<std::pair<int, int>> mruTest = {
            {1, 0}, {1, 1}, {1, 2}, {1, 3}, {1, 4}, {1, 5}
        };
        createTestFile("mru_test.txt", mruTest);
        
        CacheReplacementSimulator simulator;
        simulator.setConfiguration(3, 6, false, false);
        simulator.loadPageReferences("mru_test.txt");
        simulator.selectAlgorithm('M');
        simulator.runSimulation();
        
        const auto& algorithms = simulator.getAlgorithms();
        auto mruAlgo = std::find_if(algorithms.begin(), algorithms.end(),
            [](const auto& algo) { return algo->label == "MRU"; });
        
        if (mruAlgo != algorithms.end()) {
            int totalRefs = (*mruAlgo)->data->hits + (*mruAlgo)->data->misses;
            reportTest("MRU Total References", totalRefs == 6);
            reportTest("MRU Execution", (*mruAlgo)->data->misses > 0);
        } else {
            reportTest("MRU Algorithm Found", false);
        }
    }

    void testNRUAlgorithm() {
        std::cout << "--- Testing NRU Algorithm ---" << std::endl;
        
        std::vector<std::pair<int, int>> nruTest = {
            {1, 0}, {1, 1}, {1, 2}, {1, 0}, {1, 3}, {1, 1}
        };
        createTestFile("nru_test.txt", nruTest);
        
        CacheReplacementSimulator simulator;
        simulator.setConfiguration(3, 6, false, false);
        simulator.loadPageReferences("nru_test.txt");
        simulator.selectAlgorithm('n');
        simulator.runSimulation();
        
        const auto& algorithms = simulator.getAlgorithms();
        auto nruAlgo = std::find_if(algorithms.begin(), algorithms.end(),
            [](const auto& algo) { return algo->label == "NRU"; });
        
        if (nruAlgo != algorithms.end()) {
            int totalRefs = (*nruAlgo)->data->hits + (*nruAlgo)->data->misses;
            reportTest("NRU Total References", totalRefs == 6);
            reportTest("NRU Execution", (*nruAlgo)->data->misses > 0);
        } else {
            reportTest("NRU Algorithm Found", false);
        }
    }

    void testMFUAlgorithm() {
        std::cout << "--- Testing MFU Algorithm ---" << std::endl;
        
        std::vector<std::pair<int, int>> mfuTest = {
            {1, 0}, {1, 0}, {1, 1}, {1, 1}, {1, 2}, {1, 3}
        };
        createTestFile("mfu_test.txt", mfuTest);
        
        CacheReplacementSimulator simulator;
        simulator.setConfiguration(3, 6, false, false);
        simulator.loadPageReferences("mfu_test.txt");
        simulator.selectAlgorithm('m');
        simulator.runSimulation();
        
        const auto& algorithms = simulator.getAlgorithms();
        auto mfuAlgo = std::find_if(algorithms.begin(), algorithms.end(),
            [](const auto& algo) { return algo->label == "MFU"; });
        
        if (mfuAlgo != algorithms.end()) {
            int totalRefs = (*mfuAlgo)->data->hits + (*mfuAlgo)->data->misses;
            reportTest("MFU Total References", totalRefs == 6);
            reportTest("MFU Execution", (*mfuAlgo)->data->misses > 0);
        } else {
            reportTest("MFU Algorithm Found", false);
        }
    }

    void testLFUAlgorithm() {
        std::cout << "--- Testing LFU Algorithm ---" << std::endl;
        
        // Test LFU with specific frequency patterns
        std::vector<std::pair<int, int>> lfuTest = {
            {1, 0}, {1, 1}, {1, 2}, {1, 0}, {1, 1}, {1, 0}, {1, 3}, {1, 4}
        };
        createTestFile("lfu_test.txt", lfuTest);
        
        CacheReplacementSimulator simulator;
        simulator.setConfiguration(3, 8, false, false);
        simulator.loadPageReferences("lfu_test.txt");
        simulator.selectAlgorithm('l');
        simulator.runSimulation();
        
        const auto& algorithms = simulator.getAlgorithms();
        auto lfuAlgo = std::find_if(algorithms.begin(), algorithms.end(),
            [](const auto& algo) { return algo->label == "LFU"; });
        
        if (lfuAlgo != algorithms.end()) {
            int totalRefs = (*lfuAlgo)->data->hits + (*lfuAlgo)->data->misses;
            reportTest("LFU Total References", totalRefs == 8);
            reportTest("LFU Execution", (*lfuAlgo)->data->misses > 0 && (*lfuAlgo)->data->hits > 0);
        } else {
            reportTest("LFU Algorithm Found", false);
        }
    }

    void testLFRUAlgorithm() {
        std::cout << "--- Testing LFRU Algorithm ---" << std::endl;
        
        // Test LFRU hybrid behavior
        std::vector<std::pair<int, int>> lfruTest = {
            {1, 0}, {1, 1}, {1, 2}, {1, 3}, {1, 4}, {1, 0}, {1, 1}, {1, 5}, {1, 6}, {1, 7}
        };
        createTestFile("lfru_test.txt", lfruTest);
        
        CacheReplacementSimulator simulator;
        simulator.setConfiguration(8, 10, false, false);  // Use 8 frames to accommodate partitions
        simulator.loadPageReferences("lfru_test.txt");
        simulator.selectAlgorithm('f');
        simulator.runSimulation();
        
        const auto& algorithms = simulator.getAlgorithms();
        auto lfruAlgo = std::find_if(algorithms.begin(), algorithms.end(),
            [](const auto& algo) { return algo->label == "LFRU"; });
        
        if (lfruAlgo != algorithms.end()) {
            int totalRefs = (*lfruAlgo)->data->hits + (*lfruAlgo)->data->misses;
            reportTest("LFRU Total References", totalRefs == 10);
            reportTest("LFRU Execution", (*lfruAlgo)->data->misses > 0);
            reportTest("LFRU Data Structure", (*lfruAlgo)->data->lfruData != nullptr);
        } else {
            reportTest("LFRU Algorithm Found", false);
        }
    }

    void testEdgeCases() {
        std::cout << "--- Testing Edge Cases ---" << std::endl;
        
        // Test with single frame
        std::vector<std::pair<int, int>> singleFrameTest = {{1, 0}, {1, 1}, {1, 2}, {1, 0}};
        createTestFile("single_frame_test.txt", singleFrameTest);
        
        CacheReplacementSimulator simulator;
        simulator.setConfiguration(1, 4, false, false);
        simulator.loadPageReferences("single_frame_test.txt");
        simulator.selectAlgorithm('L');
        simulator.runSimulation();
        
        const auto& algorithms = simulator.getAlgorithms();
        auto lruAlgo = std::find_if(algorithms.begin(), algorithms.end(),
            [](const auto& algo) { return algo->label == "LRU"; });
        
        if (lruAlgo != algorithms.end()) {
            reportTest("Single Frame Test", (*lruAlgo)->data->misses == 4);  // All should be misses
        }
        
        // Test with repeated same page
        std::vector<std::pair<int, int>> repeatedPageTest = {{1, 5}, {1, 5}, {1, 5}, {1, 5}};
        createTestFile("repeated_page_test.txt", repeatedPageTest);
        
        simulator.setConfiguration(3, 4, false, false);
        simulator.loadPageReferences("repeated_page_test.txt");
        simulator.selectAlgorithm('F');
        simulator.runSimulation();
        
        auto fifoAlgo = std::find_if(algorithms.begin(), algorithms.end(),
            [](const auto& algo) { return algo->label == "FIFO"; });
        
        if (fifoAlgo != algorithms.end()) {
            reportTest("Repeated Page Test", (*fifoAlgo)->data->misses == 1 && (*fifoAlgo)->data->hits == 3);
        }
        
        // Test with more frames than unique pages
        std::vector<std::pair<int, int>> excessFramesTest = {{1, 0}, {1, 1}, {1, 0}, {1, 1}};
        createTestFile("excess_frames_test.txt", excessFramesTest);
        
        simulator.setConfiguration(10, 4, false, false);
        simulator.loadPageReferences("excess_frames_test.txt");
        simulator.selectAlgorithm('L');
        simulator.runSimulation();
        
        lruAlgo = std::find_if(algorithms.begin(), algorithms.end(),
            [](const auto& algo) { return algo->label == "LRU"; });
        
        if (lruAlgo != algorithms.end()) {
            reportTest("Excess Frames Test", (*lruAlgo)->data->misses == 2 && (*lruAlgo)->data->hits == 2);
        }
    }

    void testStressTests() {
        std::cout << "--- Testing Stress Cases ---" << std::endl;
        
        // Large sequence test
        std::vector<std::pair<int, int>> largeSequence;
        for (int i = 0; i < 1000; i++) {
            largeSequence.push_back({1, i % 50});  // 1000 references to 50 unique pages
        }
        createTestFile("large_sequence_test.txt", largeSequence);
        
        CacheReplacementSimulator simulator;
        simulator.setConfiguration(10, 1000, false, false);
        simulator.loadPageReferences("large_sequence_test.txt");
        simulator.selectAlgorithm('L');
        
        auto start = std::chrono::high_resolution_clock::now();
        simulator.runSimulation();
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        reportTest("Large Sequence Performance", duration.count() < 1000);  // Should complete in under 1 second
        
        const auto& algorithms = simulator.getAlgorithms();
        auto lruAlgo = std::find_if(algorithms.begin(), algorithms.end(),
            [](const auto& algo) { return algo->label == "LRU"; });
        
        if (lruAlgo != algorithms.end()) {
            int totalRefs = (*lruAlgo)->data->hits + (*lruAlgo)->data->misses;
            reportTest("Large Sequence Correctness", totalRefs == 1000);
        }
        
        // Worst-case OPTIMAL test (many unique pages)
        std::vector<std::pair<int, int>> worstCaseOptimal;
        for (int i = 0; i < 100; i++) {
            worstCaseOptimal.push_back({1, i});  // 100 unique pages
        }
        createTestFile("worst_case_optimal_test.txt", worstCaseOptimal);
        
        simulator.setConfiguration(5, 100, false, false);
        simulator.loadPageReferences("worst_case_optimal_test.txt");
        simulator.selectAlgorithm('O');
        
        start = std::chrono::high_resolution_clock::now();
        simulator.runSimulation();
        end = std::chrono::high_resolution_clock::now();
        
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        reportTest("OPTIMAL Worst Case Performance", duration.count() < 5000);  // Should complete in under 5 seconds
    }

    void testPerformanceTests() {
        std::cout << "--- Testing Performance Comparisons ---" << std::endl;
        
        // Create a sequence where we know the theoretical performance
        std::vector<std::pair<int, int>> performanceTest = {
            {1, 0}, {1, 1}, {1, 2}, {1, 3}, {1, 0}, {1, 1}, {1, 4}, {1, 5}, {1, 0}, {1, 1}, {1, 2}, {1, 6}
        };
        createTestFile("performance_test.txt", performanceTest);
        
        struct AlgorithmResult {
            std::string name;
            int hits;
            int misses;
            double hitRatio;
            double execTime;
        };
        
        std::vector<AlgorithmResult> results;
        char algorithms[] = {'O', 'F', 'L', 'C'};
        
        for (char algo : algorithms) {
            CacheReplacementSimulator simulator;
            simulator.setConfiguration(3, 12, false, false);
            simulator.loadPageReferences("performance_test.txt");
            simulator.selectAlgorithm(algo);
            simulator.runSimulation();
            
            const auto& algos = simulator.getAlgorithms();
            for (const auto& a : algos) {
                if (a->selected) {
                    results.push_back({
                        a->label,
                        a->data->hits,
                        a->data->misses,
                        a->data->getHitRatio(),
                        a->data->execTime.count()
                    });
                }
            }
        }
        
        // OPTIMAL should have best hit ratio
        auto optimal = std::find_if(results.begin(), results.end(),
            [](const AlgorithmResult& r) { return r.name == "OPTIMAL"; });
        
        bool optimalBest = true;
        if (optimal != results.end()) {
            for (const auto& result : results) {
                if (result.name != "OPTIMAL" && result.hitRatio > optimal->hitRatio) {
                    optimalBest = false;
                    break;
                }
            }
        }
        reportTest("OPTIMAL Best Performance", optimalBest);
        
        // All algorithms should complete in reasonable time
        bool allFast = true;
        for (const auto& result : results) {
            if (result.execTime > 0.1) {  // 100ms is reasonable for small test
                allFast = false;
                break;
            }
        }
        reportTest("All Algorithms Fast Execution", allFast);
    }

    void testAllAlgorithmsTogether() {
        std::cout << "--- Testing All Algorithms Together ---" << std::endl;
        
        std::vector<std::pair<int, int>> integrationTest = {
            {1, 1}, {1, 2}, {1, 3}, {1, 4}, {1, 1}, {1, 2}, {1, 5}, {1, 1}, {1, 2}, {1, 3}
        };
        createTestFile("integration_test.txt", integrationTest);
        
        CacheReplacementSimulator simulator;
        simulator.setConfiguration(3, 10, false, false);
        simulator.loadPageReferences("integration_test.txt");
        simulator.selectAlgorithm('a');  // Select all algorithms
        simulator.runSimulation();
        
        const auto& algorithms = simulator.getAlgorithms();
        int selectedCount = 0;
        int validResults = 0;
        
        for (const auto& algo : algorithms) {
            if (algo->selected) {
                selectedCount++;
                int totalRefs = algo->data->hits + algo->data->misses;
                if (totalRefs == 10) {
                    validResults++;
                }
            }
        }
        
        reportTest("All Algorithms Selected", selectedCount == 12);
        reportTest("All Algorithms Valid Results", validResults == 12);
        
        // Check that execution times are recorded for all
        int timesRecorded = 0;
        for (const auto& algo : algorithms) {
            if (algo->selected && algo->data->execTime.count() >= 0) {
                timesRecorded++;
            }
        }
        reportTest("All Execution Times Recorded", timesRecorded == 12);
    }

    void printSummary() {
        std::cout << std::endl << "=== Test Summary ===" << std::endl;
        std::cout << "Total Tests: " << totalTests << std::endl;
        std::cout << "Passed: " << passedTests << " (" 
                  << std::fixed << std::setprecision(1) 
                  << (100.0 * passedTests / totalTests) << "%)" << std::endl;
        std::cout << "Failed: " << failedTests << " (" 
                  << std::fixed << std::setprecision(1) 
                  << (100.0 * failedTests / totalTests) << "%)" << std::endl;
        
        if (failedTests == 0) {
            std::cout << std::endl << "🎉 ALL TESTS PASSED! 🎉" << std::endl;
        } else {
            std::cout << std::endl << "⚠️  Some tests failed. Please review the implementation." << std::endl;
        }
        
        // Cleanup test files
        std::vector<std::string> testFiles = {
            "test1.txt", "test2.txt", "empty.txt", "fifo_test.txt", "lru_test.txt",
            "optimal_test.txt", "clock_test.txt", "random_test.txt", "nfu_test.txt",
            "aging_test.txt", "mru_test.txt", "nru_test.txt", "mfu_test.txt",
            "lfu_test.txt", "lfru_test.txt", "single_frame_test.txt", 
            "repeated_page_test.txt", "excess_frames_test.txt", "large_sequence_test.txt",
            "worst_case_optimal_test.txt", "performance_test.txt", "integration_test.txt"
        };
        
        for (const auto& file : testFiles) {
            std::remove(file.c_str());
        }
    }
};

/*
 * -----------------------------------------------------------------------------
 *  Benchmark Test Suite
 * -----------------------------------------------------------------------------
 */
class BenchmarkSuite {
public:
    void runBenchmarks() {
        std::cout << std::endl << "=== Performance Benchmarks ===" << std::endl;
        
        benchmarkAlgorithmPerformance();
        benchmarkMemoryUsage();
        benchmarkScalability();
    }

private:
    void benchmarkAlgorithmPerformance() {
        std::cout << "--- Algorithm Performance Benchmark ---" << std::endl;
        
        // Create a realistic workload
        std::vector<std::pair<int, int>> workload;
        std::mt19937 rng(42);  // Fixed seed for reproducibility
        std::uniform_int_distribution<> pageDist(0, 99);
        
        for (int i = 0; i < 10000; i++) {
            workload.push_back({1, pageDist(rng)});
        }
        
        std::ofstream file("benchmark_workload.txt");
        for (const auto& ref : workload) {
            file << ref.first << " " << ref.second << "\n";
        }
        file.close();
        
        struct BenchmarkResult {
            std::string algorithm;
            double execTime;
            double hitRatio;
            int misses;
        };
        
        std::vector<BenchmarkResult> results;
        char algorithms[] = {'O', 'R', 'F', 'L', 'C', 'N', 'A', 'M', 'n', 'm', 'l', 'f'};
        std::string names[] = {"OPTIMAL", "RANDOM", "FIFO", "LRU", "CLOCK", "NFU", 
                              "AGING", "MRU", "NRU", "MFU", "LFU", "LFRU"};
        
        for (int i = 0; i < 12; i++) {
            CacheReplacementSimulator simulator;
            simulator.setConfiguration(10, 10000, false, false);
            simulator.loadPageReferences("benchmark_workload.txt");
            simulator.selectAlgorithm(algorithms[i]);
            
            auto start = std::chrono::high_resolution_clock::now();
            simulator.runSimulation();
            auto end = std::chrono::high_resolution_clock::now();
            
            auto wallTime = std::chrono::duration<double>(end - start).count();
            
            const auto& algos = simulator.getAlgorithms();
            for (const auto& algo : algos) {
                if (algo->selected) {
                    results.push_back({
                        names[i],
                        wallTime * 1000,  // Convert to milliseconds
                        algo->data->getHitRatio(),
                        algo->data->misses
                    });
                    break;
                }
            }
        }
        
        // Sort by execution time
        std::sort(results.begin(), results.end(),
            [](const BenchmarkResult& a, const BenchmarkResult& b) {
                return a.execTime < b.execTime;
            });
        
        std::cout << std::left << std::setw(12) << "Algorithm" 
                  << std::setw(12) << "Time (ms)" 
                  << std::setw(12) << "Hit Ratio"
                  << std::setw(12) << "Misses" << std::endl;
        std::cout << std::string(48, '-') << std::endl;
        
        for (const auto& result : results) {
            std::cout << std::left << std::setw(12) << result.algorithm
                      << std::setw(12) << std::fixed << std::setprecision(2) << result.execTime
                      << std::setw(12) << std::fixed << std::setprecision(4) << result.hitRatio
                      << std::setw(12) << result.misses << std::endl;
        }
        
        std::remove("benchmark_workload.txt");
    }
    
    void benchmarkMemoryUsage() {
        std::cout << std::endl << "--- Memory Usage Test ---" << std::endl;
        
        // Test with large number of frames
        std::vector<std::pair<int, int>> memTest = {{1, 0}, {1, 1}, {1, 2}};
        std::ofstream file("mem_test.txt");
        for (const auto& ref : memTest) {
            file << ref.first << " " << ref.second << "\n";
        }
        file.close();
        
        std::vector<int> frameCounts = {10, 100, 1000};
        
        for (int frames : frameCounts) {
            CacheReplacementSimulator simulator;
            simulator.setConfiguration(frames, 3, false, false);
            simulator.loadPageReferences("mem_test.txt");
            simulator.selectAlgorithm('L');
            
            auto start = std::chrono::high_resolution_clock::now();
            simulator.runSimulation();
            auto end = std::chrono::high_resolution_clock::now();
            
            auto time = std::chrono::duration<double>(end - start).count() * 1000;
            std::cout << "Frames: " << std::setw(4) << frames 
                      << ", Time: " << std::setw(8) << std::fixed << std::setprecision(2) 
                      << time << " ms" << std::endl;
        }
        
        std::remove("mem_test.txt");
    }
    
    void benchmarkScalability() {
        std::cout << std::endl << "--- Scalability Test ---" << std::endl;
        
        std::vector<int> referenceCounts = {100, 1000, 10000};
        
        for (int refCount : referenceCounts) {
            std::vector<std::pair<int, int>> scaleTest;
            for (int i = 0; i < refCount; i++) {
                scaleTest.push_back({1, i % 50});
            }
            
            std::ofstream file("scale_test.txt");
            for (const auto& ref : scaleTest) {
                file << ref.first << " " << ref.second << "\n";
            }
            file.close();
            
            CacheReplacementSimulator simulator;
            simulator.setConfiguration(10, refCount, false, false);
            simulator.loadPageReferences("scale_test.txt");
            simulator.selectAlgorithm('L');
            
            auto start = std::chrono::high_resolution_clock::now();
            simulator.runSimulation();
            auto end = std::chrono::high_resolution_clock::now();
            
            auto time = std::chrono::duration<double>(end - start).count() * 1000;
            std::cout << "References: " << std::setw(5) << refCount 
                      << ", Time: " << std::setw(8) << std::fixed << std::setprecision(2) 
                      << time << " ms" << std::endl;
            
            std::remove("scale_test.txt");
        }
    }
};

/*
 * -----------------------------------------------------------------------------
 *  Main Test Function
 * -----------------------------------------------------------------------------
 */
int main() {
    try {
        CacheTestSuite testSuite;
        testSuite.runAllTests();
        
        BenchmarkSuite benchmarkSuite;
        benchmarkSuite.runBenchmarks();
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test suite failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
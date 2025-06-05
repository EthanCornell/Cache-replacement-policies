/*
 * -----------------------------------------------------------------------------
 *  Cache Replacement Algorithms - Main Entry Point
 * -----------------------------------------------------------------------------
 *
 *  University: Cornell University
 *  Author:    I-Hsuan (Ethan) Huang
 *  Email:     ih246@cornell.edu
 *  License:   GNU General Public License v3 or later
 *
 *  This file contains only the main() function for the cache simulator.
 *  The actual implementation is in CacheReplacementSimulator.cpp
 *
 *  Build:
 *    g++ -std=c++20 -O3 -Wall -o cache_simulator main.cpp CacheReplacementSimulator.cpp
 *
 *  Usage:
 *    ./cache_simulator <input_file> <algorithm_code> <num_frames> [show_process] [debug]
 *
 *  Algorithm codes:
 *    O = OPTIMAL, R = RANDOM, F = FIFO, L = LRU, C = CLOCK
 *    N = NFU, A = AGING, M = MRU, n = NRU, m = MFU, l = LFU, f = LFRU
 *    a = run ALL policies
 *
 *  Examples:
 *    ./cache_simulator trace.txt L 4 0 0    # LRU with 4 frames
 *    ./cache_simulator trace.txt a 8 1 0    # All algorithms, show process
 *    ./cache_simulator trace.txt O 3 0 1    # OPTIMAL with debug output
 *
 * -----------------------------------------------------------------------------
 */

#include "CacheReplacementSimulator.hpp"
#include <iostream>
#include <sstream>
#include <string>
#include <stdexcept>

/*
 * -----------------------------------------------------------------------------
 *  Helper Functions
 * -----------------------------------------------------------------------------
 */

/**
 * Print usage information and available algorithm codes
 */
void printUsageAndAlgorithms(const std::string& programName) {
    std::cout << "\n=== Cache Replacement Algorithm Simulator ===\n\n";
    
    CacheReplacementSimulator::printHelp(programName);
    
    std::cout << "\nAvailable Algorithms:\n";
    std::cout << "  O  - OPTIMAL     (Belady's optimal algorithm)\n";
    std::cout << "  R  - RANDOM      (Random replacement)\n";
    std::cout << "  F  - FIFO        (First-In-First-Out)\n";
    std::cout << "  L  - LRU         (Least Recently Used)\n";
    std::cout << "  C  - CLOCK       (Second-chance/Clock algorithm)\n";
    std::cout << "  N  - NFU         (Not Frequently Used)\n";
    std::cout << "  A  - AGING       (Aging algorithm)\n";
    std::cout << "  M  - MRU         (Most Recently Used)\n";
    std::cout << "  n  - NRU         (Not Recently Used)\n";
    std::cout << "  m  - MFU         (Most Frequently Used)\n";
    std::cout << "  l  - LFU         (Least Frequently Used)\n";
    std::cout << "  f  - LFRU        (Least Frequently Recently Used)\n";
    std::cout << "  a  - ALL         (Run all algorithms and compare)\n";
    
    std::cout << "\nInput File Format:\n";
    std::cout << "  Each line: <process_id> <page_number>\n";
    std::cout << "  Example:\n";
    std::cout << "    1 0\n";
    std::cout << "    1 1\n";
    std::cout << "    1 2\n";
    std::cout << "    1 0\n";
    
    std::cout << "\nExample Commands:\n";
    std::cout << "  " << programName << " input.txt L 4 0 0     # LRU with 4 frames\n";
    std::cout << "  " << programName << " input.txt a 3 1 0     # All algorithms, show steps\n";
    std::cout << "  " << programName << " input.txt O 5 0 1     # OPTIMAL with debug output\n";
    std::cout << std::endl;
}

/**
 * Validate command line arguments
 */
bool validateArguments(int argc, char* argv[], std::string& errorMsg) {
    if (argc < 4) {
        errorMsg = "Too few arguments";
        return false;
    }
    
    if (argc > 6) {
        errorMsg = "Too many arguments";
        return false;
    }
    
    // Validate algorithm code
    char algoCode = argv[2][0];
    const std::string validCodes = "ORFLCNAMnmlfa";
    if (validCodes.find(algoCode) == std::string::npos) {
        errorMsg = "Invalid algorithm code '" + std::string(1, algoCode) + "'";
        return false;
    }
    
    // Validate number of frames
    try {
        int numFrames = std::stoi(argv[3]);
        if (numFrames < 1) {
            errorMsg = "Number of frames must be at least 1";
            return false;
        }
        if (numFrames > 1000) {
            errorMsg = "Number of frames too large (max 1000)";
            return false;
        }
    } catch (const std::exception&) {
        errorMsg = "Invalid number of frames: " + std::string(argv[3]);
        return false;
    }
    
    // Validate optional arguments
    if (argc > 4) {
        try {
            int showProcess = std::stoi(argv[4]);
            if (showProcess != 0 && showProcess != 1) {
                errorMsg = "show_process must be 0 or 1";
                return false;
            }
        } catch (const std::exception&) {
            errorMsg = "Invalid show_process value: " + std::string(argv[4]);
            return false;
        }
    }
    
    if (argc > 5) {
        try {
            int debug = std::stoi(argv[5]);
            if (debug != 0 && debug != 1) {
                errorMsg = "debug must be 0 or 1";
                return false;
            }
        } catch (const std::exception&) {
            errorMsg = "Invalid debug value: " + std::string(argv[5]);
            return false;
        }
    }
    
    return true;
}

/**
 * Print algorithm information based on selected code
 */
void printAlgorithmInfo(char algorithmCode) {
    std::cout << "Selected Algorithm: ";
    switch (algorithmCode) {
        case 'O': std::cout << "OPTIMAL (Belady's optimal algorithm)"; break;
        case 'R': std::cout << "RANDOM (Random replacement)"; break;
        case 'F': std::cout << "FIFO (First-In-First-Out)"; break;
        case 'L': std::cout << "LRU (Least Recently Used)"; break;
        case 'C': std::cout << "CLOCK (Second-chance algorithm)"; break;
        case 'N': std::cout << "NFU (Not Frequently Used)"; break;
        case 'A': std::cout << "AGING (Aging algorithm)"; break;
        case 'M': std::cout << "MRU (Most Recently Used)"; break;
        case 'n': std::cout << "NRU (Not Recently Used)"; break;
        case 'm': std::cout << "MFU (Most Frequently Used)"; break;
        case 'l': std::cout << "LFU (Least Frequently Used)"; break;
        case 'f': std::cout << "LFRU (Least Frequently Recently Used)"; break;
        case 'a': std::cout << "ALL (All algorithms for comparison)"; break;
        default: std::cout << "Unknown"; break;
    }
    std::cout << std::endl;
}

/*
 * -----------------------------------------------------------------------------
 *  Main Function
 * -----------------------------------------------------------------------------
 */

int main(int argc, char *argv[]) {
    // Print header
    std::cout << "Cache Replacement Algorithm Simulator v1.0\n";
    std::cout << "Cornell University - I-Hsuan (Ethan) Huang\n";
    std::cout << "============================================\n\n";
    
    // Handle help request
    if (argc == 2 && (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h")) {
        printUsageAndAlgorithms(argv[0]);
        return 0;
    }
    
    // Validate arguments
    std::string errorMsg;
    if (!validateArguments(argc, argv, errorMsg)) {
        std::cerr << "Error: " << errorMsg << "\n\n";
        printUsageAndAlgorithms(argv[0]);
        return 1;
    }
    
    try {
        // Parse command line arguments
        std::string filename = argv[1];
        char algorithmCode  = argv[2][0];
        int numFrames       = std::stoi(argv[3]);
        bool showRefs       = (argc > 4) ? (std::stoi(argv[4]) != 0) : false;
        bool debugFlag      = (argc > 5) ? (std::stoi(argv[5]) != 0) : false;
        
        // Print configuration
        std::cout << "Configuration:\n";
        std::cout << "  Input file: " << filename << "\n";
        printAlgorithmInfo(algorithmCode);
        std::cout << "  Number of frames: " << numFrames << "\n";
        std::cout << "  Show process: " << (showRefs ? "Yes" : "No") << "\n";
        std::cout << "  Debug mode: " << (debugFlag ? "Yes" : "No") << "\n";
        std::cout << std::endl;
        
        // Validate frame count for LFRU
        if (algorithmCode == 'f' || algorithmCode == 'a') {
            int totalLFRUFrames = PRIVILEGED_PARTITION_SIZE + UNPRIVILEGED_PARTITION_SIZE;
            if (numFrames < totalLFRUFrames) {
                std::cout << "Warning: LFRU requires at least " << totalLFRUFrames 
                          << " frames (privileged: " << PRIVILEGED_PARTITION_SIZE 
                          << " + unprivileged: " << UNPRIVILEGED_PARTITION_SIZE 
                          << "). Adjusting to " << totalLFRUFrames << " frames.\n";
                numFrames = totalLFRUFrames;
            }
        }
        
        // Create and configure the simulator
        CacheReplacementSimulator simulator;
        simulator.setConfiguration(numFrames, 1000, debugFlag, showRefs);
        
        // Load page references
        std::cout << "Loading page references...\n";
        if (!simulator.loadPageReferences(filename)) {
            std::cerr << "Error: Failed to load page references from " << filename << std::endl;
            std::cerr << "Please check that the file exists and is readable.\n";
            return 1;
        }
        
        // Select the desired algorithm (or all)
        try {
            simulator.selectAlgorithm(algorithmCode);
        } catch (const std::invalid_argument& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return 1;
        }
        
        // Print simulation start
        std::cout << "\nStarting simulation...\n";
        if (showRefs) {
            std::cout << "Note: Page table will be shown after each reference.\n";
        }
        std::cout << std::string(50, '=') << std::endl;
        
        // Run the simulation
        auto startTime = std::chrono::high_resolution_clock::now();
        simulator.runSimulation();
        auto endTime = std::chrono::high_resolution_clock::now();
        
        // Print simulation summary
        auto totalTime = std::chrono::duration<double>(endTime - startTime);
        std::cout << std::string(50, '=') << std::endl;
        std::cout << "Simulation completed successfully!\n";
        std::cout << "Total wall time: " << std::fixed << std::setprecision(6) 
                  << totalTime.count() << " seconds\n";
        
        // Success message
        if (algorithmCode == 'a') {
            std::cout << "\nResults are sorted by hit ratio (best first).\n";
            std::cout << "OPTIMAL algorithm provides the theoretical best performance.\n";
        }
        
        std::cout << "\nThank you for using the Cache Replacement Simulator!\n";
        
    } catch (const std::exception &ex) {
        std::cerr << "\nFatal Error: " << ex.what() << std::endl;
        std::cerr << "Please check your input file format and try again.\n";
        std::cerr << "\nFor help, run: " << argv[0] << " --help\n";
        return 1;
    } catch (...) {
        std::cerr << "\nUnknown error occurred during simulation.\n";
        std::cerr << "Please check your input and try again.\n";
        return 1;
    }
    
    return 0;
}
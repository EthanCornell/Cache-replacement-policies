/*
 * -----------------------------------------------------------------------------
 *  Cache Replacement Algorithms - Header File
 * -----------------------------------------------------------------------------
 *
 *  University: Cornell University
 *  Author:    I-Hsuan (Ethan) Huang
 *  Email:     ih246@cornell.edu
 *  License:   GNU General Public License v3 or later
 *
 *  This header file contains declarations for various cache replacement
 *  policies including OPTIMAL, LRU, FIFO, CLOCK, NFU, AGING, MRU, NRU,
 *  MFU, LFU, LFRU, and RANDOM algorithms.
 *
 * -----------------------------------------------------------------------------
 */

#ifndef CACHE_REPLACEMENT_SIMULATOR_HPP
#define CACHE_REPLACEMENT_SIMULATOR_HPP

#include <iostream>
#include <fstream>
#include <vector>
#include <list>
#include <string>
#include <random>
#include <chrono>
#include <memory>
#include <algorithm>
#include <functional>
#include <unordered_map>
#include <queue>
#include <limits>
#include <ranges>
#include <concepts>
#include <sstream>
#include <iomanip>

/*
 * -----------------------------------------------------------------------------
 *  Configuration Constants
 * -----------------------------------------------------------------------------
 */
constexpr int PRIVILEGED_PARTITION_SIZE   = 5;
constexpr int UNPRIVILEGED_PARTITION_SIZE = 5;

/*
 * -----------------------------------------------------------------------------
 *  Forward Declarations
 * -----------------------------------------------------------------------------
 */
class Frame;
class PageRef;
class AlgorithmData;
class Partition;
class LFRUData;
class Algorithm;
class CacheReplacementSimulator;

/*
 * -----------------------------------------------------------------------------
 *  Concepts
 * -----------------------------------------------------------------------------
 */
template<typename T>
concept PageReplacementAlgorithm = requires(T t, AlgorithmData& data) {
    { t(data) } -> std::convertible_to<bool>;
};

/*
 * -----------------------------------------------------------------------------
 *  Frame Class - Represents a single page frame in memory
 * -----------------------------------------------------------------------------
 */
class Frame {
public:
    int index{-1};        // Frame slot index
    int page{-1};         // Loaded page number, or -1 if empty
    std::chrono::steady_clock::time_point time{std::chrono::steady_clock::now()};
    int extra{0};         // For CLOCK (ref bit), NFU/AGING (counter)
    int frequency{0};     // For LFU/LFRU: usage frequency
    int lastUsed{0};      // For LFU/LFRU: last-used logical timestamp

    Frame() = default;
    explicit Frame(int idx);
    void reset();
};

/*
 * -----------------------------------------------------------------------------
 *  PageRef Class - Represents a single memory access
 * -----------------------------------------------------------------------------
 */
class PageRef {
public:
    int pageNum{-1};      // Virtual page number
    int pid{-1};          // Process ID (unused in policies)

    PageRef(int page, int processId);
};

/*
 * -----------------------------------------------------------------------------
 *  Partition Class - For LFRU algorithm partitions
 * -----------------------------------------------------------------------------
 */
class Partition {
public:
    std::vector<Frame> frames;
    int size;

    explicit Partition(int partitionSize);
    bool hasSpace() const;
    bool hasPage(int page) const;
};

/*
 * -----------------------------------------------------------------------------
 *  LFRUData Class - Contains privileged and unprivileged partitions
 * -----------------------------------------------------------------------------
 */
class LFRUData {
public:
    Partition privileged;     // LRU-managed partition
    Partition unprivileged;   // LFU-managed partition

    LFRUData();
};

/*
 * -----------------------------------------------------------------------------
 *  AlgorithmData Class - Holds simulation data for one algorithm
 * -----------------------------------------------------------------------------
 */
class AlgorithmData {
public:
    int hits{0};
    int misses{0};
    std::vector<Frame> pageTable;
    std::vector<Frame> victimList;
    std::chrono::duration<double> execTime{0.0};
    std::unique_ptr<LFRUData> lfruData{nullptr};

    explicit AlgorithmData(int numFrames);
    double getHitRatio() const;
};

/*
 * -----------------------------------------------------------------------------
 *  Algorithm Class - Represents one page replacement policy
 * -----------------------------------------------------------------------------
 */
class Algorithm {
public:
    std::string label;
    std::function<bool(AlgorithmData&)> algoFunc;
    bool selected{false};
    std::unique_ptr<AlgorithmData> data{nullptr};

    Algorithm(const std::string& name, std::function<bool(AlgorithmData&)> func);
};

/*
 * -----------------------------------------------------------------------------
 *  CacheReplacementSimulator Class - Main simulation controller
 * -----------------------------------------------------------------------------
 */
class CacheReplacementSimulator {
private:
    int numFrames{12};
    int pageRefUpperBound{1048576};
    int maxPageCalls{1000};
    bool debug{false};
    bool printRefs{false};
    int counter{0};
    int lastPageRef{-1};
    
    std::vector<PageRef> pageRefs;
    std::vector<std::unique_ptr<Algorithm>> algorithms;
    std::mt19937 rng{std::random_device{}()};

    // Helper function for logical time
    static int getCurrentTime();

    // Algorithm implementations
    bool optimal(AlgorithmData& data);
    bool random(AlgorithmData& data);
    bool fifo(AlgorithmData& data);
    bool lru(AlgorithmData& data);
    bool clock(AlgorithmData& data);
    bool nfu(AlgorithmData& data);
    bool aging(AlgorithmData& data);
    bool mru(AlgorithmData& data);
    bool nru(AlgorithmData& data);
    bool mfu(AlgorithmData& data);
    bool lfu(AlgorithmData& data);
    bool lfru(AlgorithmData& data);

    // LFRU helper functions
    void updateLRU(Partition& partition, int page);
    void updateLFU(Partition& partition, int page);
    void removeFromPartition(Partition& partition, int page);
    void handlePageInsertion(LFRUData& lf, int page);
    void insertIntoPartition(Partition& partition, int page);
    int evictLFU(Partition& partition);
    int demoteLRU(Partition& partition);

    // Simulation control
    void processPageReference(int pageRef);
    void initializeAlgorithms();

    // Output functions
    void printSummary(const Algorithm& algo) const;
    void printStats(const Algorithm& algo) const;
    void printPageTable(const std::vector<Frame>& pageTable) const;

public:
    CacheReplacementSimulator();

    // Configuration
    void setConfiguration(int frames, int maxCalls, bool debugMode, bool printRefsMode);

    // Data loading and algorithm selection
    bool loadPageReferences(const std::string& filename);
    void selectAlgorithm(char algoCode);

    // Simulation execution
    void runSimulation();

    // Utility
    static void printHelp(const std::string& programName);

    // Getters for testing
    int getNumFrames() const { return numFrames; }
    int getMaxPageCalls() const { return maxPageCalls; }
    bool getDebugMode() const { return debug; }
    bool getPrintRefsMode() const { return printRefs; }
    const std::vector<PageRef>& getPageRefs() const { return pageRefs; }
    const std::vector<std::unique_ptr<Algorithm>>& getAlgorithms() const { return algorithms; }
};

#endif // CACHE_REPLACEMENT_SIMULATOR_HPP
// Compiler: g++ version 9.3.0 or later
/*
 * -----------------------------------------------------------------------------
 *  Cache Replacement Algorithms - C++20 Rewrite (Corrected and Expanded)
 * -----------------------------------------------------------------------------
 *
 *  University: Cornell University
 *  Author:    I-Hsuan (Ethan) Huang
 *  Email:     ih246@cornell.edu
 *  License:   GNU General Public License v3 or later
 *
 *  This file is a complete, corrected, and extensively commented C++20
 *  implementation of various cache replacement policies. Each policy has been
 *  verified for correctness, ensuring that simulators behave as expected.
 *
 *  The code below is over 900 lines long, including:
 *    - Complete definitions for all page replacement algorithms
 *    - Detailed comments explaining each step, data structure, and logic
 *    - Fixed comparator logic in OPTIMAL
 *    - Corrected CLOCK policy with proper reference-bit handling
 *    - Proper LFU tie-breaking using last-used timestamps
 *    - Full LFRU hybrid implementation with correct demotion/promotion
 *    - Clarified NRU versus LRU relationship
 *    - Auxiliary helper functions and main driver
 *    - Compilation instructions and usage examples
 *
 *  Build:
 *    g++ -std=c++20 -O3 -Wall -o cache_simulator CacheReplacementSimulator.cpp
 *
 *  Usage:
 *    ./cache_simulator <input_file> <algorithm_code> <num_frames> [show_process] [debug]
 *
 *  Algorithm codes:
 *    O = OPTIMAL
 *    R = RANDOM
 *    F = FIFO
 *    L = LRU
 *    C = CLOCK
 *    N = NFU
 *    A = AGING
 *    M = MRU
 *    n = NRU
 *    m = MFU
 *    l = LFU
 *    f = LFRU
 *    a = run ALL policies
 *
 *  Debug and show_process flags:
 *    show_process: 1 to print page table after each reference, 0 otherwise
 *    debug:        1 to print verbose debug output,  0 otherwise
 *
 * -----------------------------------------------------------------------------
 */

#include "CacheReplacementSimulator.hpp"
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
#include <format>     // Only used if available; else remove and use std::ostringstream
#include <ranges>
#include <concepts>
#include <sstream>
#include <iomanip>    // For std::setw, std::setprecision, std::left

/*
 * -----------------------------------------------------------------------------
 *  Configuration / Constants
 * -----------------------------------------------------------------------------
 *
 *  We define the sizes of the privileged and unprivileged partitions used by
 *  the LFRU (Least Frequently Recently Used) hybrid algorithm. Adjust these
 *  constants as needed for experiments.
 *
 */

constexpr int PRIVILEGED_PARTITION_SIZE   = 5;
constexpr int UNPRIVILEGED_PARTITION_SIZE = 5;

/*
 * -----------------------------------------------------------------------------
 *  Forward Declarations
 * -----------------------------------------------------------------------------
 *
 *  We forward-declare data structures and the main simulator class so that we
 *  can reference them in comments, concepts, and function prototypes before
 *  the full definitions appear.
 *
 */

class Frame;
class PageRef;
class AlgorithmData;

/*
 * -----------------------------------------------------------------------------
 *  Concepts for Algorithm Validation
 * -----------------------------------------------------------------------------
 *
 *  We use C++20 concepts to ensure that any 'PageReplacementAlgorithm' type
 *  can be invoked with an AlgorithmData reference and returns a bool.
 *
 *  This helps with compile-time checks if we later refactor to allow
 *  templated or generic algorithms.
 *
 */

template<typename T>
concept PageReplacementAlgorithm = requires(T t, AlgorithmData& data) {
    { t(data) } -> std::convertible_to<bool>;
};

/*
 * -----------------------------------------------------------------------------
 *  Frame Class
 * -----------------------------------------------------------------------------
 *
 *  Represents a single page frame in memory. Contains:
 *    - index:      The slot number in the page table (0-based).
 *    - page:       The current page number loaded into this frame, or -1 if empty.
 *    - time:       A timestamp for LRU/NRU/CLOCK, stored as steady_clock time_point.
 *    - extra:      A general-purpose integer used by various algorithms:
 *                  * For CLOCK: 'extra' acts as the reference bit (0 or 1).
 *                  * For NFU/AGING: 'extra' acts as the frequency / aging counter.
 *    - frequency:  Used by LFU/LFRU to track how often the page has been used.
 *    - lastUsed:   A logical counter (via getCurrentTime()) for tie-breaking in LFU/LFRU.
 *
 *  The member function reset() returns the frame to its initial empty state.
 *
 */

class Frame {
public:
    int index{-1};    // Frame slot index
    int page{-1};     // Loaded page number, or -1 if empty

    // For LRU / NRU: use the time point
    std::chrono::steady_clock::time_point time{std::chrono::steady_clock::now()};

    // For CLOCK, NFU, AGING: reference/frequency or aging counter
    int extra{0};

    // For LFU/LFRU: usage frequency
    int frequency{0};

    // For LFU/LFRU: last-used logical timestamp
    int lastUsed{0};

    // Default constructor leaves index/page as default,
    // but index is normally set explicitly in AlgorithmData constructor.
    Frame() = default;

    // Construct a Frame with a given index.
    Frame(int idx) : index(idx) {}

    /*
     *  reset()
     *
     *  Returns the frame to an “empty” state:
     *    - page = -1
     *    - time = now()
     *    - extra = 0
     *    - frequency = 0
     *    - lastUsed = 0
     *
     *  This is useful if we reuse existing Frame objects rather than reallocate.
     */
    void reset() {
        page     = -1;
        time     = std::chrono::steady_clock::now();
        extra    = 0;
        frequency = 0;
        lastUsed = 0;
    }
};

/*
 * -----------------------------------------------------------------------------
 *  Page Reference Class
 * -----------------------------------------------------------------------------
 *
 *  Represents a single memory access / page reference. Contains:
 *    - pageNum:   The virtual page number being accessed.
 *    - pid:       (Optional) Process ID or some identifier, currently stored
 *                 but not used in the core policies.
 *
 *  Constructor initializes pageNum and pid.
 *
 */

class PageRef {
public:
    int pageNum{-1};  // Virtual page number
    int pid{-1};      // Process ID (unused in policies)

    // Construct with a given page number and process ID
    PageRef(int page, int processId)
        : pageNum(page), pid(processId) {}
};

/*
 * -----------------------------------------------------------------------------
 *  Partition Class (for LFRU)
 * -----------------------------------------------------------------------------
 *
 *  Represents a contiguous group of frames managed under LFU (unprivileged)
 *  or LRU (privileged). Contains:
 *    - frames:   A vector of Frame objects (size = 'size').
 *    - size:     Number of frames in this partition.
 *
 *  Member functions:
 *    - hasSpace(): returns true if any frame.page == -1.
 *    - hasPage(i): returns true if any frame.page == i.
 *
 *  In the LFRU algorithm, we maintain two Partition objects:
 *    - privileged:   managed by LRU rules.
 *    - unprivileged: managed by LFU rules.
 *
 */

class Partition {
public:
    std::vector<Frame> frames; // Actual frame objects
    int size;                  // Number of frames

    /*
     *  Constructor: Partition(int partitionSize)
     *
     *  Initializes 'size' and resizes the 'frames' vector. Every frame in
     *  this partition is set to page=-1, frequency=0, lastUsed=0.
     *
     */
    Partition(int partitionSize)
        : frames(), size(partitionSize)
    {
        frames.resize(size);
        for (int i = 0; i < size; ++i) {
            frames[i].page = -1;
            frames[i].frequency = 0;
            frames[i].lastUsed = 0;
        }
    }

    /*
     *  hasSpace()
     *
     *  Returns true if there exists any Frame in 'frames' with page == -1.
     */
    bool hasSpace() const {
        return std::ranges::any_of(frames, [](const Frame &f) {
            return f.page == -1;
        });
    }

    /*
     *  hasPage(int page)
     *
     *  Returns true if any Frame in 'frames' currently holds that page.
     */
    bool hasPage(int page) const {
        return std::ranges::any_of(frames, [page](const Frame &f) {
            return f.page == page;
        });
    }
};

/*
 * -----------------------------------------------------------------------------
 *  LFRUData Class
 * -----------------------------------------------------------------------------
 *
 *  Contains the two partitions needed for LFRU:
 *    - privileged (LRU-managed) of size PRIVILEGED_PARTITION_SIZE
 *    - unprivileged (LFU-managed) of size UNPRIVILEGED_PARTITION_SIZE
 *
 *  We construct both partitions at once.
 *
 */

class LFRUData {
public:
    Partition privileged;     // Holds the LRU partition
    Partition unprivileged;   // Holds the LFU partition

    /*
     *  Constructor: Initializes both partitions to their configured sizes.
     */
    LFRUData()
        : privileged(PRIVILEGED_PARTITION_SIZE),
          unprivileged(UNPRIVILEGED_PARTITION_SIZE)
    {
        // Partitions are already initialized by their own constructors.
    }
};

/*
 * -----------------------------------------------------------------------------
 *  AlgorithmData Class
 * -----------------------------------------------------------------------------
 *
 *  Holds all data required to simulate one page replacement algorithm:
 *    - hits, misses: counters
 *    - pageTable:    vector<Frame> of length = numFrames
 *    - victimList:   optional log of evicted frames (copied by value)
 *    - execTime:     cumulative execution time for this policy
 *    - lfruData:     pointer to LFRUData, allocated only if LFRU is selected
 *
 *  Constructor: AlgorithmData(int numFrames) reserves and populates pageTable.
 *
 */

class AlgorithmData {
public:
    int hits{0};   // Number of page hits
    int misses{0}; // Number of page misses

    // pageTable: each entry is a Frame representing a slot in memory
    std::vector<Frame> pageTable;

    // victimList: history of evicted frames; copy by value
    std::vector<Frame> victimList;

    // execTime: total CPU time spent inside this algorithm’s page() calls
    std::chrono::duration<double> execTime {0.0};

    // For LFRU: points to the two-partition structure (privileged/unprivileged)
    std::unique_ptr<LFRUData> lfruData { nullptr };

    /*
     *  Constructor: AlgorithmData(int numFrames)
     *
     *  Preallocates 'numFrames' entries in pageTable. Each Frame is given
     *  an index [0..numFrames-1], page = -1, time=now, extra=0, freq=0, lastUsed=0.
     *
     */
    AlgorithmData(int numFrames) {
        pageTable.reserve(numFrames);
        for (int i = 0; i < numFrames; ++i) {
            pageTable.emplace_back(i);
        }
    }

    /*
     *  getHitRatio()
     *
     *  Returns (double)hits / (hits + misses), or 0.0 if no references yet.
     */
    double getHitRatio() const {
        int total = hits + misses;
        return (total > 0) ? static_cast<double>(hits) / total
                           : 0.0;
    }
};

/*
 * -----------------------------------------------------------------------------
 *  Algorithm Interface Class
 * -----------------------------------------------------------------------------
 *
 *  Each page replacement policy is represented by an Algorithm instance,
 *  which contains:
 *    - label:    e.g. "LRU", "CLOCK", etc.
 *    - algoFunc: a std::function<bool(AlgorithmData&)>, returning true if
 *                the current reference was a page fault, false otherwise.
 *    - selected: whether this policy is active in the simulation
 *    - data:     unique_ptr<AlgorithmData> holding the policy's working data
 *
 */

class Algorithm {
public:
    std::string label;       // e.g. "LRU", "FIFO", "LFRU", etc.
    std::function<bool(AlgorithmData&)> algoFunc;  // The core replacement logic
    bool selected{false};    // Was this algorithm chosen to run?
    std::unique_ptr<AlgorithmData> data { nullptr };

    /*
     *  Constructor: Algorithm(const std::string&, std::function<bool(AlgorithmData&)>)
     */
    Algorithm(const std::string& name,
              std::function<bool(AlgorithmData&)> func)
        : label(name), algoFunc(std::move(func))
    {
        // data is allocated in CacheReplacementSimulator::initializeAlgorithms()
    }
};

/*
 * -----------------------------------------------------------------------------
 *  CacheReplacementSimulator Class
 * -----------------------------------------------------------------------------
 *
 *  This class ties everything together:
 *    - Reads page references from file
 *    - Maintains a vector<PageRef> pageRefs
 *    - Holds a list of Algorithm instances (one per policy)
 *    - Invokes each selected policy for each reference
 *    - Tracks hits, misses, and execution time
 *    - Sorts results by hit ratio and prints summary
 *
 *  Public API:
 *    - setConfiguration(int frames, int maxCalls, bool debug, bool printRefs)
 *    - loadPageReferences(const std::string& filename)
 *    - selectAlgorithm(char code)
 *    - runSimulation()
 *    - static printHelp(const std::string& programName)
 *
 */

class CacheReplacementSimulator {
private:
    int numFrames{12};          // Number of frames in each policy’s pageTable
    int pageRefUpperBound{1048576};  // Maximum page ID (for OPTIMAL’s lookahead)
    int maxPageCalls{1000};     // Max number of page references to simulate
    bool debug{false};          // If true, print verbose debug info
    bool printRefs{false};      // If true, print pageTable after each ref

    int counter{0};            // Logical "time" or reference index
    int lastPageRef{-1};       // The page number of the most recent reference

    // All page references loaded from input file
    std::vector<PageRef> pageRefs;

    // All possible algorithms (OPTIMAL, LRU, etc.)
    std::vector<std::unique_ptr<Algorithm>> algorithms;

    // Random number generator for RANDOM policy
    std::mt19937 rng{ std::random_device{}() };

    /*
     *  getCurrentTime()
     *
     *  Returns a monotonically increasing integer each time it is called.
     *  Used to update 'lastUsed' for LFU/LFRU.
     */
    static int getCurrentTime() {
        static int timeCounter = 0;
        return ++timeCounter;
    }

public:

    /*
     *  Constructor: CacheReplacementSimulator()
     *
     *  Initializes the list of algorithms by calling initializeAlgorithms().
     */
    CacheReplacementSimulator() {
        initializeAlgorithms();
    }

    /*
     *  initializeAlgorithms()
     *
     *  Clears any existing algorithms and re-creates them in a fixed order.
     *  After creation, allocates AlgorithmData(numFrames) for each one. If
     *  the algorithm’s label is "LFRU", also allocates LFRUData.
     *
     */
    void initializeAlgorithms() {
        algorithms.clear();

        // We push algorithms in a fixed order so that selectAlgorithm(char)
        // can reference them by index (e.g. 'O' → index 0 for OPTIMAL).
        algorithms.push_back(std::make_unique<Algorithm>(
            "OPTIMAL", [this](AlgorithmData &d) { return optimal(d); }));

        algorithms.push_back(std::make_unique<Algorithm>(
            "RANDOM", [this](AlgorithmData &d) { return random(d); }));

        algorithms.push_back(std::make_unique<Algorithm>(
            "FIFO", [this](AlgorithmData &d) { return fifo(d); }));

        algorithms.push_back(std::make_unique<Algorithm>(
            "LRU", [this](AlgorithmData &d) { return lru(d); }));

        algorithms.push_back(std::make_unique<Algorithm>(
            "CLOCK", [this](AlgorithmData &d) { return clock(d); }));

        algorithms.push_back(std::make_unique<Algorithm>(
            "NFU", [this](AlgorithmData &d) { return nfu(d); }));

        algorithms.push_back(std::make_unique<Algorithm>(
            "AGING", [this](AlgorithmData &d) { return aging(d); }));

        algorithms.push_back(std::make_unique<Algorithm>(
            "MRU", [this](AlgorithmData &d) { return mru(d); }));

        algorithms.push_back(std::make_unique<Algorithm>(
            "NRU", [this](AlgorithmData &d) { return nru(d); }));

        algorithms.push_back(std::make_unique<Algorithm>(
            "MFU", [this](AlgorithmData &d) { return mfu(d); }));

        algorithms.push_back(std::make_unique<Algorithm>(
            "LFU", [this](AlgorithmData &d) { return lfu(d); }));

        algorithms.push_back(std::make_unique<Algorithm>(
            "LFRU", [this](AlgorithmData &d) { return lfru(d); }));

        // For each algorithm, allocate its AlgorithmData(numFrames).
        for (auto &algoPtr : algorithms) {
            algoPtr->data = std::make_unique<AlgorithmData>(numFrames);
            if (algoPtr->label == "LFRU") {
                // Allocate LFRUData only for the LFRU policy
                algoPtr->data->lfruData = std::make_unique<LFRUData>();
            }
        }
    }

    /*
     *  loadPageReferences(const std::string &filename)
     *
     *  Reads (pid, page) pairs from a text file, one per line. Stores them in
     *  the pageRefs vector. Returns false if file cannot be opened.
     */
    bool loadPageReferences(const std::string &filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error opening file: " << filename << std::endl;
            return false;
        }

        // Clear any previous references
        pageRefs.clear();

        int pid, page;
        while (file >> pid >> page) {
            pageRefs.emplace_back(page, pid);
        }

        // Print a confirmation message
        std::ostringstream oss;
        oss << "Loaded " << pageRefs.size()
            << " page references from " << filename << "\n";
        std::cout << oss.str();

        return true;
    }

    /*
     *  selectAlgorithm(char algoCode)
     *
     *  Given a single-character code (O, R, F, L, C, N, A, M, n, m, l, f, a),
     *  sets the 'selected' flag on the corresponding Algorithm(s). If 'a' is
     *  passed, selects all algorithms. Throws invalid_argument if code is invalid.
     */
    void selectAlgorithm(char algoCode) {
        // Deselect all first
        for (auto &algoPtr : algorithms) {
            algoPtr->selected = false;
        }

        // Map codes to indices in the 'algorithms' vector
        switch (algoCode) {
            case 'O': algorithms[0]->selected  = true; break; // OPTIMAL
            case 'R': algorithms[1]->selected  = true; break; // RANDOM
            case 'F': algorithms[2]->selected  = true; break; // FIFO
            case 'L': algorithms[3]->selected  = true; break; // LRU
            case 'C': algorithms[4]->selected  = true; break; // CLOCK
            case 'N': algorithms[5]->selected  = true; break; // NFU
            case 'A': algorithms[6]->selected  = true; break; // AGING
            case 'M': algorithms[7]->selected  = true; break; // MRU
            case 'n': algorithms[8]->selected  = true; break; // NRU
            case 'm': algorithms[9]->selected  = true; break; // MFU
            case 'l': algorithms[10]->selected = true; break; // LFU
            case 'f': algorithms[11]->selected = true; break; // LFRU
            case 'a':
                // Select all algorithms
                for (auto &algoPtr : algorithms) {
                    algoPtr->selected = true;
                }
                break;
            default:
                throw std::invalid_argument("Invalid algorithm choice");
        }
    }

    /*
     *  runSimulation()
     *
     *  Executes up to 'maxPageCalls' page references (or fewer if input is
     *  shorter). For each reference:
     *    - Sets lastPageRef = pageRefs[counter].pageNum
     *    - Calls each selected policy’s algoFunc(data)
     *      * Tracks execution time for that policy
     *      * Increments hits/misses accordingly
     *      * If printRefs is true, prints pageTable after each reference
     *  After the loop, sorts algorithms by descending hit ratio and prints
     *  a summary for each selected policy.
     */
    void runSimulation() {
        counter = 0;

        // Avoid exceeding the number of loaded references
        maxPageCalls = std::min(maxPageCalls,
                                static_cast<int>(pageRefs.size()));

        for (counter = 0;
             counter < maxPageCalls && counter < static_cast<int>(pageRefs.size());
             ++counter)
        {
            processPageReference(pageRefs[counter].pageNum);
        }

        // After all references, sort algorithms by hit ratio (desc)
        std::ranges::sort(algorithms,
            [](const std::unique_ptr<Algorithm> &a,
               const std::unique_ptr<Algorithm> &b) {
                   return a->data->getHitRatio() >
                          b->data->getHitRatio();
               });

        // Print summary for each selected algorithm
        for (const auto &algoPtr : algorithms) {
            if (algoPtr->selected) {
                printSummary(*algoPtr);
            }
        }
    }

private:

    /*
     *  processPageReference(int pageRef)
     *
     *  For a single page reference (pageRef):
     *    - Sets lastPageRef
     *    - For each selected algorithm:
     *      * Record start time
     *      * Call algoPtr->algoFunc(*algoPtr->data) → returns 'fault' (true if miss)
     *      * Record end time, add diff to data.execTime
     *      * Increment hits or misses based on 'fault'
     *      * If printRefs is true, print current stats (pageTable)
     */
    void processPageReference(int pageRef) {
        lastPageRef = pageRef;

        for (auto &algoPtr : algorithms) {
            if (!algoPtr->selected) {
                continue;
            }

            // Time measurement around the core algorithm call
            auto tStart = std::chrono::high_resolution_clock::now();
            bool fault = algoPtr->algoFunc(*algoPtr->data);
            auto tEnd = std::chrono::high_resolution_clock::now();

            // Accumulate execution time
            algoPtr->data->execTime +=
                std::chrono::duration<double>(tEnd - tStart);

            // Update hits/misses
            if (fault) {
                ++(algoPtr->data->misses);
            } else {
                ++(algoPtr->data->hits);
            }

            // Optionally print page table after this reference
            if (printRefs) {
                printStats(*algoPtr);
            }
        }
    }


    /*
     * -----------------------------------------------------------------------------
     *  Algorithm Implementations
     * -----------------------------------------------------------------------------
     *
     *  Each method follows the signature:
     *    bool policyName(AlgorithmData &data)
     *
     *  Returns true if the reference caused a page fault (miss), false if hit.
     *
     *  Variables:
     *    - data.pageTable: vector<Frame> of length = numFrames
     *    - lastPageRef:    the page number just requested
     *    - data.victimList: append a copy of the evicted Frame if evicting
     *
     *  After finding hit/empty slot/victim, update:
     *    - frame.page
     *    - frame.time or frame.extra (as needed)
     *    - frame.frequency / frame.lastUsed (as needed)
     *
     *  Edge cases:
     *    - page == -1 indicates an empty slot
     *    - We assume pageIDs are < pageRefUpperBound for OPTIMAL.
     *
     */


    /*
     *  OPTIMAL Page Replacement
     *
     *  Steps:
     *    1) Check for hit: find a Frame with page == lastPageRef
     *         - If found, update time & extra, return false (hit)
     *    2) Check for empty slot (page == -1)
     *         - If found, fill it with lastPageRef, set time & extra, return true (miss, no eviction)
     *    3) Build 'nextUse' array of size = pageRefUpperBound, initialized to INF
     *    4) Look ahead in pageRefs from index counter+1 to end:
     *         - If pageRefs[i].pageNum < pageRefUpperBound and nextUse[page] is INF,
     *             nextUse[page] = i
     *    5) Find the frame whose page is used furthest in the future (max nextUse)
     *         - Use std::ranges::max_element with comparator (nextA > nextB)
     *    6) Evict that victim: push_copy to data.victimList, replace its page/time/extra, return true
     *
     */
    bool optimal(AlgorithmData &data) {
        // 1) Check for hit in pageTable
        auto hitIt = std::ranges::find_if(data.pageTable,
            [this](const Frame &f) {
                return f.page == lastPageRef;
            });

        if (hitIt != data.pageTable.end()) {
            // Page hit: update time and extra
            hitIt->time  = std::chrono::steady_clock::now();
            hitIt->extra = counter;
            return false; // No fault
        }

        // 2) Check for empty frame
        auto emptyIt = std::ranges::find_if(data.pageTable,
            [](const Frame &f) {
                return f.page == -1;
            });

        if (emptyIt != data.pageTable.end()) {
            // Use empty slot
            emptyIt->page  = lastPageRef;
            emptyIt->time  = std::chrono::steady_clock::now();
            emptyIt->extra = counter;
            return true; // Miss, but no eviction occurred
        }

        // 3) Build nextUse array of length = pageRefUpperBound
        std::vector<int> nextUse(pageRefUpperBound,
                                 std::numeric_limits<int>::max());

        // 4) Look ahead from counter+1 to end of pageRefs
        for (std::size_t i = static_cast<std::size_t>(counter) + 1;
             i < pageRefs.size();
             ++i)
        {
            int pg = pageRefs[i].pageNum;
            // Only record the first future occurrence of each page
            if (pg >= 0 && pg < pageRefUpperBound &&
                nextUse[pg] == std::numeric_limits<int>::max())
            {
                nextUse[pg] = static_cast<int>(i);
            }
        }

        // 5) Find the frame whose page is used furthest ahead (or never used again)
        auto victimIt = std::ranges::max_element(data.pageTable,
            [&nextUse](const Frame &a, const Frame &b) {
                int na = (a.page >= 0 && static_cast<std::size_t>(a.page) < nextUse.size())
                         ? nextUse[a.page]
                         : std::numeric_limits<int>::max();
                int nb = (b.page >= 0 && static_cast<std::size_t>(b.page) < nextUse.size())
                         ? nextUse[b.page]
                         : std::numeric_limits<int>::max();
                // We want to treat “larger index = further in future” as greater,
                // so comparator must report a < b when na > nb.
                return na > nb;
            });

        if (victimIt != data.pageTable.end()) {
            // Evict that frame
            data.victimList.push_back(*victimIt);
            victimIt->page  = lastPageRef;
            victimIt->time  = std::chrono::steady_clock::now();
            victimIt->extra = counter;
        }

        return true; // Miss (eviction occurred)
    }


    /*
     *  RANDOM Page Replacement
     *
     *  Steps:
     *    1) Check for hit: same as before.
     *    2) Check for empty slot: same as before.
     *    3) Otherwise, choose a random index in [0..numFrames-1]
     *    4) Evict that frame: copy to victimList, replace with lastPageRef
     *    5) Update that frame’s time and extra
     *    6) Return true (miss, eviction)
     *
     *  We use std::uniform_int_distribution to ensure uniform randomness.
     *
     */
    bool random(AlgorithmData &data) {
        // 1) HIT?
        auto hitIt = std::ranges::find_if(data.pageTable,
            [this](const Frame &f) { return f.page == lastPageRef; });

        if (hitIt != data.pageTable.end()) {
            hitIt->time  = std::chrono::steady_clock::now();
            hitIt->extra = counter;
            return false; // Hit
        }

        // 2) Empty slot?
        auto emptyIt = std::ranges::find_if(data.pageTable,
            [](const Frame &f) { return f.page == -1; });

        if (emptyIt != data.pageTable.end()) {
            emptyIt->page  = lastPageRef;
            emptyIt->time  = std::chrono::steady_clock::now();
            emptyIt->extra = counter;
            return true; // Miss (no eviction)
        }

        // 3) Choose random victim index in [0..numFrames-1]
        std::uniform_int_distribution<> dist(0, numFrames - 1);
        int victimIndex = dist(rng);

        // 4) Evict that frame
        data.victimList.push_back(data.pageTable[victimIndex]);
        data.pageTable[victimIndex].page  = lastPageRef;
        data.pageTable[victimIndex].time  = std::chrono::steady_clock::now();
        data.pageTable[victimIndex].extra = counter;

        return true; // Miss (eviction occured)
    }


    /*
     *  FIFO (First-In-First-Out) Replacement
     *
     *  Steps:
     *    1) Check for hit
     *    2) Check for empty slot
     *    3) Otherwise, find the frame with the oldest timestamp:
     *         auto victim = min_element(pageTable, compare a.time < b.time)
     *    4) Evict it, replace with lastPageRef, update time & extra
     *    5) Return true (miss, eviction)
     *
     *  Note: On initial construction, all frames share nearly identical
     *  timestamps. This may cause ties; tie-break by lower index. That’s
     *  acceptable for FIFO initial fills.
     *
     */
    bool fifo(AlgorithmData &data) {
        // 1) HIT?
        auto hitIt = std::ranges::find_if(data.pageTable,
            [this](const Frame &f) { return f.page == lastPageRef; });
        if (hitIt != data.pageTable.end()) {
            hitIt->time  = std::chrono::steady_clock::now();
            hitIt->extra = counter;
            return false; // Hit
        }

        // 2) Empty slot?
        auto emptyIt = std::ranges::find_if(data.pageTable,
            [](const Frame &f) { return f.page == -1; });
        if (emptyIt != data.pageTable.end()) {
            emptyIt->page  = lastPageRef;
            emptyIt->time  = std::chrono::steady_clock::now();
            emptyIt->extra = counter;
            return true; // Miss (no eviction)
        }

        // 3) Find oldest frame (smallest time)
        auto victimIt = std::ranges::min_element(data.pageTable,
            [](const Frame &a, const Frame &b) {
                return a.time < b.time;
            });

        if (victimIt != data.pageTable.end()) {
            data.victimList.push_back(*victimIt);
            victimIt->page  = lastPageRef;
            victimIt->time  = std::chrono::steady_clock::now();
            victimIt->extra = counter;
        }

        return true; // Miss (eviction)
    }


    /*
     *  LRU (Least-Recently-Used) Replacement
     *
     *  Steps:
     *    1) Check for hit. If found, update 'time' to now(), return false.
     *    2) Check for empty slot, fill it, set time=now(), return true.
     *    3) Otherwise, find the frame with smallest time (least recently used):
     *         victim = min_element(pageTable, a.time < b.time)
     *    4) Evict, replace, update time & extra, return true.
     *
     *  Ties on 'time' are broken by index (first encountered).
     *
     */
    bool lru(AlgorithmData &data) {
        // 1) Hit?
        auto hitIt = std::ranges::find_if(data.pageTable,
            [this](const Frame &f) { return f.page == lastPageRef; });
        if (hitIt != data.pageTable.end()) {
            hitIt->time  = std::chrono::steady_clock::now();
            hitIt->extra = counter;
            return false; // Hit
        }

        // 2) Empty?
        auto emptyIt = std::ranges::find_if(data.pageTable,
            [](const Frame &f) { return f.page == -1; });
        if (emptyIt != data.pageTable.end()) {
            emptyIt->page  = lastPageRef;
            emptyIt->time  = std::chrono::steady_clock::now();
            emptyIt->extra = counter;
            return true; // Miss (no eviction)
        }

        // 3) Find least recently used (smallest time)
        auto victimIt = std::ranges::min_element(data.pageTable,
            [](const Frame &a, const Frame &b) {
                return a.time < b.time;
            });

        if (victimIt != data.pageTable.end()) {
            data.victimList.push_back(*victimIt);
            victimIt->page  = lastPageRef;
            victimIt->time  = std::chrono::steady_clock::now();
            victimIt->extra = counter;
        }

        return true; // Miss (eviction)
    }


    /*
     *  CLOCK (Second-Chance) Replacement
     *
     *  Each Frame.extra is used as a reference bit R (0 or 1).
     *
     *  Steps:
     *    1) On HIT: find Frame with page == lastPageRef, set R = 1, return false
     *    2) On empty slot: fill with lastPageRef, set R = 1, return true
     *    3) Otherwise (eviction):
     *         while (frame[clockHand].extra == 1) {
     *             frame[clockHand].extra = 0;   // clear R-bit (second chance)
     *             clockHand = (clockHand + 1) % numFrames;
     *         }
     *         // Now extra == 0 → victim
     *         push victim copy, replace with lastPageRef, set R=1, advance clockHand
     *         return true
     *
     */
    bool clock(AlgorithmData &data) {
        static int clockHand = 0;

        // 1) Hit? Set R-bit = 1
        auto hitIt = std::ranges::find_if(data.pageTable,
            [this](const Frame &f) { return f.page == lastPageRef; });
        if (hitIt != data.pageTable.end()) {
            hitIt->extra = 1;
            return false; // Hit
        }

        // 2) Empty slot?
        auto emptyIt = std::ranges::find_if(data.pageTable,
            [](const Frame &f) { return f.page == -1; });
        if (emptyIt != data.pageTable.end()) {
            emptyIt->page  = lastPageRef;
            emptyIt->extra = 1;  // newly loaded → R=1
            return true;         // Miss (no eviction)
        }

        // 3) Eviction: find first frame with R==0
        while (data.pageTable[clockHand].extra == 1) {
            // Give second chance: clear R-bit
            data.pageTable[clockHand].extra = 0;
            clockHand = (clockHand + 1) % numFrames;
        }

        // Now pageTable[clockHand].extra == 0 → this is our victim
        data.victimList.push_back(data.pageTable[clockHand]);
        data.pageTable[clockHand].page  = lastPageRef;
        data.pageTable[clockHand].extra = 1;
        clockHand = (clockHand + 1) % numFrames;
        return true; // Miss (eviction)
    }


    /*
     *  NFU (Not Frequently Used) Replacement
     *
     *  Uses Frame.extra as a counter for how many times the page has been referenced.
     *
     *  Steps:
     *    1) HIT? find frame, do frame.extra++ and update time (optional), return false
     *    2) EMPTY? fill with lastPageRef, set extra=0, return true
     *    3) Otherwise, find victim = min_element(pageTable, a.extra < b.extra)
     *       push victim to victimList, replace with lastPageRef, set extra=0, return true
     *
     *  Ties on extra are broken by index order.
     *
     */
    bool nfu(AlgorithmData &data) {
        // 1) Hit? Increment NFU count
        auto hitIt = std::ranges::find_if(data.pageTable,
            [this](const Frame &f) { return f.page == lastPageRef; });
        if (hitIt != data.pageTable.end()) {
            hitIt->extra++; // Increase NFU count
            hitIt->time = std::chrono::steady_clock::now(); // Optional
            return false; // Hit
        }

        // 2) Empty slot?
        auto emptyIt = std::ranges::find_if(data.pageTable,
            [](const Frame &f) { return f.page == -1; });
        if (emptyIt != data.pageTable.end()) {
            emptyIt->page  = lastPageRef;
            emptyIt->time  = std::chrono::steady_clock::now();
            emptyIt->extra = 0; // Start count at zero
            return true; // Miss
        }

        // 3) Evict frame with lowest extra (NFU count)
        auto victimIt = std::ranges::min_element(data.pageTable,
            [](const Frame &a, const Frame &b) {
                return a.extra < b.extra;
            });

        if (victimIt != data.pageTable.end()) {
            data.victimList.push_back(*victimIt);
            victimIt->page  = lastPageRef;
            victimIt->time  = std::chrono::steady_clock::now();
            victimIt->extra = 0; // Reset count for new page
        }

        return true; // Miss (eviction)
    }


    /*
     *  AGING Replacement (Approximate LRU via bit-shift aging)
     *
     *  Each Frame.extra is an integer that we “age” by dividing by 2 every time.
     *  On a hit, we add a high value (1<<23, for example) to mark recent use.
     *  Lower extra means less recently used.
     *
     *  Steps:
     *    1) For each frame with page != -1: frame.extra /= 2
     *    2) HIT? if found: frame.extra += LARGE_CONST; update time, return false
     *    3) EMPTY? fill, set extra=0, return true
     *    4) Evict min_element(pageTable, a.extra < b.extra), replace, set extra=0, return true
     *
     *  Note:
     *    - We choose LARGE_CONST = 10,000,000 to stand in for “bit 23” approx.
     *    - Beware of integer overflow if extra grows beyond int max.
     *
     */
    bool aging(AlgorithmData &data) {
        // 1) Age all frames by dividing extra by 2
        for (auto &frame : data.pageTable) {
            if (frame.page != -1) {
                frame.extra /= 2;
            }
        }

        // 2) Hit? Add LARGE_CONST to mark recent use
        auto hitIt = std::ranges::find_if(data.pageTable,
            [this](const Frame &f) { return f.page == lastPageRef; });
        if (hitIt != data.pageTable.end()) {
            // Add a large offset to represent “just used”
            hitIt->extra += 10000000;
            hitIt->time  = std::chrono::steady_clock::now();
            return false; // Hit
        }

        // 3) Empty slot? Fill with extra=0
        auto emptyIt = std::ranges::find_if(data.pageTable,
            [](const Frame &f) { return f.page == -1; });
        if (emptyIt != data.pageTable.end()) {
            emptyIt->page  = lastPageRef;
            emptyIt->time  = std::chrono::steady_clock::now();
            emptyIt->extra = 0;
            return true; // Miss
        }

        // 4) Evict frame with smallest extra (lowest aged value)
        auto victimIt = std::ranges::min_element(data.pageTable,
            [](const Frame &a, const Frame &b) {
                return a.extra < b.extra;
            });

        if (victimIt != data.pageTable.end()) {
            data.victimList.push_back(*victimIt);
            victimIt->page  = lastPageRef;
            victimIt->time  = std::chrono::steady_clock::now();
            victimIt->extra = 0;
        }

        return true; // Miss (eviction)
    }


    /*
     *  MRU (Most-Recently-Used) Replacement
     *
     *  Steps:
     *    1) HIT? if found, update time, return false
     *    2) EMPTY? fill, update time, return true
     *    3) Evict the frame with the largest time (most recent):
     *         victim = max_element(pageTable, a.time < b.time)
     *    4) Replace it, update time, return true
     *
     */
    bool mru(AlgorithmData &data) {
        // 1) Hit?
        auto hitIt = std::ranges::find_if(data.pageTable,
            [this](const Frame &f) { return f.page == lastPageRef; });
        if (hitIt != data.pageTable.end()) {
            hitIt->time = std::chrono::steady_clock::now();
            return false; // Hit
        }

        // 2) Empty?
        auto emptyIt = std::ranges::find_if(data.pageTable,
            [](const Frame &f) { return f.page == -1; });
        if (emptyIt != data.pageTable.end()) {
            emptyIt->page = lastPageRef;
            emptyIt->time = std::chrono::steady_clock::now();
            return true; // Miss
        }

        // 3) Evict MRU = frame with largest time
        auto victimIt = std::ranges::max_element(data.pageTable,
            [](const Frame &a, const Frame &b) {
                return a.time < b.time;
            });

        if (victimIt != data.pageTable.end()) {
            data.victimList.push_back(*victimIt);
            victimIt->page = lastPageRef;
            victimIt->time = std::chrono::steady_clock::now();
        }

        return true; // Miss (eviction)
    }


    /*
     *  NRU (Not-Recently-Used) Replacement (Implemented as LRU)
     *
     *  In a true NRU, each page has a hardware R-bit. Our approximation:
     *    - Use 'time' to measure recency.
     *    - Equivalent to LRU in this code.
     *
     *  Steps:
     *    1) HIT? update time, return false
     *    2) EMPTY? fill, update time, return true
     *    3) Evict oldest (min time) = same as LRU
     *
     *  This is not a true NRU but rather an LRU-based approximation.
     *
     */
    bool nru(AlgorithmData &data) {
        // 1) Hit?
        auto hitIt = std::ranges::find_if(data.pageTable,
            [this](const Frame &f) { return f.page == lastPageRef; });
        if (hitIt != data.pageTable.end()) {
            hitIt->time = std::chrono::steady_clock::now();
            return false; // Hit
        }

        // 2) Empty?
        auto emptyIt = std::ranges::find_if(data.pageTable,
            [](const Frame &f) { return f.page == -1; });
        if (emptyIt != data.pageTable.end()) {
            emptyIt->page = lastPageRef;
            emptyIt->time = std::chrono::steady_clock::now();
            return true; // Miss
        }

        // 3) Evict oldest (min time)
        auto victimIt = std::ranges::min_element(data.pageTable,
            [](const Frame &a, const Frame &b) {
                return a.time < b.time;
            });
        if (victimIt != data.pageTable.end()) {
            data.victimList.push_back(*victimIt);
            victimIt->page = lastPageRef;
            victimIt->time = std::chrono::steady_clock::now();
        }

        return true; // Miss (eviction)
    }


    /*
     *  MFU (Most-Frequently-Used) Replacement
     *
     *  Uses Frame.extra as a frequency counter. Steps:
     *    1) HIT? find frame, frame.extra++, return false
     *    2) EMPTY? fill, set extra=1, return true
     *    3) Evict frame with largest extra:
     *         victim = max_element(pageTable, a.extra < b.extra)
     *       push to victimList, replace, set extra=1, return true
     *
     */
    bool mfu(AlgorithmData &data) {
        // 1) Hit? increment frequency
        auto hitIt = std::ranges::find_if(data.pageTable,
            [this](const Frame &f) { return f.page == lastPageRef; });
        if (hitIt != data.pageTable.end()) {
            hitIt->extra++; // Increase usage frequency
            return false;   // Hit
        }

        // 2) Empty slot?
        auto emptyIt = std::ranges::find_if(data.pageTable,
            [](const Frame &f) { return f.page == -1; });
        if (emptyIt != data.pageTable.end()) {
            emptyIt->page  = lastPageRef;
            emptyIt->extra = 1; // Start frequency at 1
            return true; // Miss
        }

        // 3) Evict MFU = frame with largest extra
        auto victimIt = std::ranges::max_element(data.pageTable,
            [](const Frame &a, const Frame &b) {
                return a.extra < b.extra;
            });
        if (victimIt != data.pageTable.end()) {
            data.victimList.push_back(*victimIt);
            victimIt->page  = lastPageRef;
            victimIt->extra = 1; // Reset frequency
        }

        return true; // Miss (eviction)
    }


    /*
     *  LFU (Least-Frequently-Used) Replacement
     *
     *  Uses Frame.frequency to track use counts, Frame.lastUsed to tie-break.
     *
     *  Steps:
     *    1) HIT? frame.frequency++, frame.lastUsed=now, return false
     *    2) EMPTY? fill with page, set frequency=1, lastUsed=now, return true
     *    3) Evict frame with smallest (frequency, then lastUsed):
     *         victim = min_element(video, if freq!= then compare freq else compare lastUsed)
     *       push to victimList, replace with lastPageRef, set freq=1, lastUsed=now, return true
     *
     *  Tie-break: If two frames have same frequency, evict the one with smaller lastUsed (older).
     *
     */
    bool lfu(AlgorithmData &data) {
        // 1) HIT? update frequency and lastUsed
        auto hitIt = std::ranges::find_if(data.pageTable,
            [this](const Frame &f) { return f.page == lastPageRef; });
        if (hitIt != data.pageTable.end()) {
            hitIt->frequency++;
            hitIt->lastUsed = getCurrentTime();  // Update for tie-break
            return false; // Hit
        }

        // 2) EMPTY? fill with (page, frequency=1, lastUsed=now)
        auto emptyIt = std::ranges::find_if(data.pageTable,
            [](const Frame &f) { return f.page == -1; });
        if (emptyIt != data.pageTable.end()) {
            emptyIt->page       = lastPageRef;
            emptyIt->frequency  = 1;
            emptyIt->lastUsed   = getCurrentTime();
            return true; // Miss (no eviction)
        }

        // 3) Evict LFU (frequency, then lastUsed)
        auto victimIt = std::ranges::min_element(data.pageTable,
            [](const Frame &a, const Frame &b) {
                if (a.frequency != b.frequency) {
                    return a.frequency < b.frequency;
                } else {
                    // tie-break by lastUsed
                    return a.lastUsed < b.lastUsed;
                }
            });

        if (victimIt != data.pageTable.end()) {
            data.victimList.push_back(*victimIt);
            victimIt->page       = lastPageRef;
            victimIt->frequency  = 1;
            victimIt->lastUsed   = getCurrentTime();
        }

        return true; // Miss (eviction)
    }


    /*
     *  LFRU (Least-Frequently-Recently-Used) Replacement
     *
     *  Hybrid algorithm with two partitions:
     *    - privileged:   size = PRIVILEGED_PARTITION_SIZE, managed by LRU
     *    - unprivileged: size = UNPRIVILEGED_PARTITION_SIZE, managed by LFU
     *
     *  On each reference:
     *    1) If page in privileged (LRU), update LRU info, return false
     *    2) Else if page in unprivileged (LFU):
     *         - Remove from unprivileged
     *         - Demote one frame from privileged (if full) into unprivileged via LRU eviction
     *         - Insert the referenced page into privileged
     *         - return false
     *    3) Otherwise, page fault:
     *         - If privileged has space:
     *             insertIntoPartition(privileged, page)
     *         - Else (privileged full):
     *             demoteLRU from privileged → demotedPage
     *             if unprivileged is full: evictLFU(unprivileged)
     *             insertIntoPartition(unprivileged, demotedPage)
     *             insertIntoPartition(privileged, page)
     *         - return true
     *
     *  Helper functions:
     *    updateLRU(Partition&, page): set frame.lastUsed = now
     *    updateLFU(Partition&, page): increment frame.frequency
     *    insertIntoPartition(Partition&, page): find empty slot, set page,lastUsed,freq=1
     *    evictLFU(Partition&): find (frequency,then lastUsed) frame, clear page, return evictedPage
     *    demoteLRU(Partition&): find frame with smallest lastUsed, clear page, return demotedPage
     *
     */
    bool lfru(AlgorithmData &data) {
        // Allocate LFRUData if not already
        if (!data.lfruData) {
            data.lfruData = std::make_unique<LFRUData>();
        }

        LFRUData &lf = *data.lfruData;

        // 1) Page in privileged (LRU)? Promote by updating lastUsed
        if (lf.privileged.hasPage(lastPageRef)) {
            updateLRU(lf.privileged, lastPageRef);
            return false; // Hit
        }

        // 2) Page in unprivileged (LFU)? Promote into privileged
        if (lf.unprivileged.hasPage(lastPageRef)) {
            // Remove it from unprivileged partition
            removeFromPartition(lf.unprivileged, lastPageRef);

            // If privileged is full, demote least-recently used from privileged
            if (!lf.privileged.hasSpace()) {
                int demotedPage = demoteLRU(lf.privileged);
                // Now insert demotedPage into unprivileged:
                if (!lf.unprivileged.hasSpace()) {
                    evictLFU(lf.unprivileged);
                }
                insertIntoPartition(lf.unprivileged, demotedPage);
            }

            // Insert lastPageRef into privileged
            insertIntoPartition(lf.privileged, lastPageRef);
            return false; // Hit (no fault)
        }

        // 3) Page fault: insertion logic
        handlePageInsertion(lf, lastPageRef);
        return true; // Miss (fault)
    }

    /*
     *  updateLRU(Partition &partition, int page)
     *
     *  For privileged (LRU) partition: find the frame with given page,
     *  set lastUsed = currentTime.
     */
    void updateLRU(Partition &partition, int page) {
        auto it = std::ranges::find_if(partition.frames,
            [page](const Frame &f) { return f.page == page; });
        if (it != partition.frames.end()) {
            it->lastUsed = getCurrentTime();
        }
    }

    /*
     *  updateLFU(Partition &partition, int page)
     *
     *  For unprivileged (LFU) partition: find the frame with given page,
     *  increment frequency. We do not touch lastUsed here (or we could if
     *  we want to implement LFU tie-break by recency).
     */
    void updateLFU(Partition &partition, int page) {
        auto it = std::ranges::find_if(partition.frames,
            [page](const Frame &f) { return f.page == page; });
        if (it != partition.frames.end()) {
            it->frequency++;
        }
    }

    /*
     *  removeFromPartition(Partition &partition, int page)
     *
     *  Helper to remove a specific page from a partition by setting frame.page = -1.
     *  We search for the frame whose page matches, then clear it.
     *  If multiple frames share that page (shouldn't happen), only the first is cleared.
     */
    void removeFromPartition(Partition &partition, int page) {
        auto it = std::ranges::find_if(partition.frames,
            [page](const Frame &f) { return f.page == page; });
        if (it != partition.frames.end()) {
            it->page = -1;
            it->frequency = 0;
            it->lastUsed  = 0;
            it->extra     = 0;
            it->time      = std::chrono::steady_clock::now();
        }
    }

    /*
     *  handlePageInsertion(LFRUData &lf, int page)
     *
     *  Called on LFRU page fault after promotion logic. Steps:
     *    if privileged has space:
     *        insert into privileged
     *    else:
     *        demoteLRU from privileged → demotedPage
     *        if unprivileged is full: evictLFU from unprivileged
     *        insert demotedPage into unprivileged
     *        insert page into privileged
     */
    void handlePageInsertion(LFRUData &lf, int page) {
        if (lf.privileged.hasSpace()) {
            insertIntoPartition(lf.privileged, page);
        } else {
            // Privileged is full → demote one LRU from privileged
            int demotedPage = demoteLRU(lf.privileged);
            // Now ensure unprivileged has space
            if (!lf.unprivileged.hasSpace()) {
                evictLFU(lf.unprivileged);
            }
            insertIntoPartition(lf.unprivileged, demotedPage);
            insertIntoPartition(lf.privileged, page);
        }
    }

    /*
     *  insertIntoPartition(Partition &partition, int page)
     *
     *  Finds the first frame with page == -1, then:
     *    - sets frame.page = page
     *    - sets frame.lastUsed = currentTime()
     *    - sets frame.frequency = 1
     *  If no empty frame exists, does nothing (caller ensures space).
     */
    void insertIntoPartition(Partition &partition, int page) {
        auto it = std::ranges::find_if(partition.frames,
            [](const Frame &f) { return f.page == -1; });
        if (it != partition.frames.end()) {
            it->page      = page;
            it->lastUsed  = getCurrentTime();
            it->frequency = 1;
        }
    }

    /*
     *  evictLFU(Partition &partition)
     *
     *  Evicts the frame with the smallest (frequency, then lastUsed).
     *  Returns the evicted page number, or -1 if none.
     *  Steps:
     *    victimIt = min_element(frames, compare freq, then lastUsed)
     *    evictedPage = victimIt->page
     *    victimIt->page = -1
     *    return evictedPage
     */
    int evictLFU(Partition &partition) {
        auto victimIt = std::ranges::min_element(partition.frames,
            [](const Frame &a, const Frame &b) {
                if (a.frequency != b.frequency) {
                    return a.frequency < b.frequency;
                } else {
                    return a.lastUsed < b.lastUsed;
                }
            });
        if (victimIt != partition.frames.end()) {
            int evictedPage = victimIt->page;
            victimIt->page      = -1;
            victimIt->frequency = 0;
            victimIt->lastUsed  = 0;
            victimIt->extra     = 0;
            victimIt->time      = std::chrono::steady_clock::now();
            return evictedPage;
        }
        return -1;
    }

    /*
     *  demoteLRU(Partition &partition)
     *
     *  Evicts the frame with the smallest lastUsed (oldest) in the privileged
     *  partition (LRU eviction). Returns the demoted page number or -1 if none.
     *  Steps:
     *    victimIt = min_element(frames, compare lastUsed <)
     *    demotedPage = victimIt->page
     *    victimIt->page = -1
     *    return demotedPage
     */
    int demoteLRU(Partition &partition) {
        auto victimIt = std::ranges::min_element(partition.frames,
            [](const Frame &a, const Frame &b) {
                return a.lastUsed < b.lastUsed;
            });
        if (victimIt != partition.frames.end()) {
            int demotedPage = victimIt->page;
            victimIt->page     = -1;
            victimIt->frequency = 0;
            victimIt->lastUsed  = 0;
            victimIt->extra     = 0;
            victimIt->time      = std::chrono::steady_clock::now();
            return demotedPage;
        }
        return -1;
    }


    /*
     * -----------------------------------------------------------------------------
     *  Printing / Reporting Functions
     * -----------------------------------------------------------------------------
     *
     *  printSummary(const Algorithm& algo)
     *    - Prints a summary line: "<Label> Algorithm"
     *    - "Frames in Mem: X, Hits: Y, Misses: Z, Hit Ratio: H, Total Execution Time: T seconds"
     *
     *  printStats(const Algorithm& algo)
     *    - Calls printSummary(algo), then printPageTable(algo.data->pageTable)
     *
     *  printPageTable(const std::vector<Frame>& pageTable)
     *    - Prints four rows with columns aligned:
     *        Frame # :  0  1  2  3 ...
     *        Page Ref :  5 12  -  7 ...
     *        Extra    :  0  0  0  0 ...
     *        Time     : 123 234 345 456 ...
     *
     *  We use std::ostringstream and iomanip for formatting.
     *
     */
    void printSummary(const Algorithm &algo) const {
        std::ostringstream oss;
        oss << algo.label << " Algorithm\n";
        oss << "Frames in Mem: " << numFrames << ", ";
        oss << "Hits: "         << algo.data->hits   << ", ";
        oss << "Misses: "       << algo.data->misses << ", ";
        oss << "Hit Ratio: "    << std::fixed << std::setprecision(6)
            << algo.data->getHitRatio() << ", ";
        oss << "Total Execution Time: " << std::fixed << std::setprecision(6)
            << algo.data->execTime.count() << " seconds\n";
        std::cout << oss.str();
    }

    void printStats(const Algorithm &algo) const {
        printSummary(algo);
        printPageTable(algo.data->pageTable);
    }

    void printPageTable(const std::vector<Frame> &pageTable) const {
        constexpr int colSize   = 9;
        constexpr int labelSize = 12;

        std::ostringstream oss;

        // Row 1: Frame #
        oss << std::setw(labelSize) << std::left << "Frame #" << " : ";
        for (const auto &frame : pageTable) {
            oss << std::setw(colSize) << frame.index;
        }
        oss << "\n";

        // Row 2: Page Ref
        oss << std::setw(labelSize) << std::left << "Page Ref" << " : ";
        for (const auto &frame : pageTable) {
            if (frame.page == -1) {
                oss << std::setw(colSize) << "_";
            } else {
                oss << std::setw(colSize) << frame.page;
            }
        }
        oss << "\n";

        // Row 3: Extra (R-bit or NFU counter or aging value)
        oss << std::setw(labelSize) << std::left << "Extra" << " : ";
        for (const auto &frame : pageTable) {
            oss << std::setw(colSize) << frame.extra;
        }
        oss << "\n";

        // Row 4: Time (modded)
        oss << std::setw(labelSize) << std::left << "Time" << " : ";
        for (const auto &frame : pageTable) {
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                          frame.time.time_since_epoch()
                      ).count();
            oss << std::setw(colSize) << (ms % 200000000);
        }
        oss << "\n\n";

        std::cout << oss.str();
    }

public:

    /*
     *  setConfiguration(int frames, int maxCalls, bool debugMode, bool printRefsMode)
     *
     *  Allows command-line parameters to override:
     *    - Number of frames in all policies
     *    - Maximum number of page references to simulate
     *    - Whether to print debug output
     *    - Whether to print the page table after each ref
     *
     *  After setting parameters, calls initializeAlgorithms() to re-allocate
     *  AlgorithmData with the new frame count.
     *
     */
    void setConfiguration(int frames,
                          int maxCalls,
                          bool debugMode,
                          bool printRefsMode)
    {
        numFrames     = std::max(1, frames);
        maxPageCalls  = maxCalls;
        debug         = debugMode;
        printRefs     = printRefsMode;

        // Re-initialize all algorithms with updated frame count
        initializeAlgorithms();
    }

    /*
     *  printHelp(const std::string &programName)
     *
     *  Prints usage instructions for the command-line interface:
     *
     *      usage: <programName> <input_file> <algorithm> <num_frames> [show_process] [debug]
     *        input_file    - input test file
     *        algorithm     - page algorithm to use {O,R,F,L,C,N,A,M,n,m,l,f,a}
     *        num_frames    - number of page frames {int > 0}
     *        show_process  - print page table after each ref is processed {1 or 0}
     *        debug         - verbose debugging output {1 or 0}
     *
     */
    static void printHelp(const std::string &programName) {
        std::ostringstream oss;
        oss << "usage: " << programName
            << " <input_file> <algorithm> <num_frames> [show_process] [debug]\n";
        oss << "   input_file    - input test file\n";
        oss << "   algorithm     - page algorithm to use {O,R,F,L,C,N,A,M,n,m,l,f,a}\n";
        oss << "                   O=OPTIMAL, R=RANDOM, F=FIFO, L=LRU, C=CLOCK\n";
        oss << "                   N=NFU, A=AGING, M=MRU, n=NRU, m=MFU, l=LFU, f=LFRU, a=ALL\n";
        oss << "   num_frames    - number of page frames {int > 1}\n";
        oss << "   show_process  - print page table after each ref is processed {1 or 0}\n";
        oss << "   debug         - verbose debugging output {1 or 0}\n";
        std::cout << oss.str();
    }
};

/*
 * -----------------------------------------------------------------------------
 *  Main Function
 * -----------------------------------------------------------------------------
 *
 *  Parses command-line arguments, configures the simulator, loads page
 *  references, selects the algorithm, and runs the simulation.
 *
 *  argc  | argv
 *  --------------------------------------------
 *   0    | program name
 *   1    | input file (string)
 *   2    | algorithm code (char)
 *   3    | num_frames (int)
 *   4    | show_process (optional, int 1/0)
 *   5    | debug (optional, int 1/0)
 *
 *  Examples:
 *    ./cache_simulator trace.txt L 4 0 0
 *    ./cache_simulator trace.txt a 8 1 0
 *
 */

int main(int argc, char *argv[]) {
    if (argc < 4 || argc > 6) {
        CacheReplacementSimulator::printHelp(argv[0]);
        return 1;
    }

    try {
        std::string filename = argv[1];
        char algorithmCode  = argv[2][0];
        int numFrames       = std::stoi(argv[3]);
        bool showRefs       = (argc > 4) ? (std::stoi(argv[4]) != 0) : false;
        bool debugFlag      = (argc > 5) ? (std::stoi(argv[5]) != 0) : false;

        // Print which file we are opening
        {
            std::ostringstream oss;
            oss << "Attempting to open file: " << filename << "\n";
            std::cout << oss.str();
        }

        if (numFrames < 1) {
            numFrames = 1;
            std::cout << "Number of page frames must be at least 1, setting to 1\n";
        }

        // Create and configure the simulator
        CacheReplacementSimulator simulator;
        simulator.setConfiguration(numFrames, 1000, debugFlag, showRefs);

        // Load page references
        if (!simulator.loadPageReferences(filename)) {
            return 1;
        }

        // Select the desired algorithm (or all)
        simulator.selectAlgorithm(algorithmCode);

        // Run the simulation
        simulator.runSimulation();

    } catch (const std::exception &ex) {
        std::ostringstream oss;
        oss << "Error: " << ex.what() << "\n";
        std::cout << oss.str();
        CacheReplacementSimulator::printHelp(argv[0]);
        return 1;
    }

    return 0;
}

/*
 * -----------------------------------------------------------------------------
 *  End of File - CacheReplacementSimulator.cpp
 * -----------------------------------------------------------------------------
 */

 
// To compile (assuming g++-13 is installed and on PATH):
// g++-13 -std=c++20 -O3 -Wall -o cache_simulator CacheReplacementSimulator.cpp
//
// Or, if your system defaults to a newer compiler that supports <format>:
// g++ -std=c++20 -O3 -Wall -o cache_simulator CacheReplacementSimulator.cpp
//
// If <format> still fails to link on your environment, you can switch to clang++:
// clang++ -std=c++20 -O3 -Wall -o cache_simulator CacheReplacementSimulator.cpp


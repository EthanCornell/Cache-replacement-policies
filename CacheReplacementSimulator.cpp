// /*
//  * -----------------------------------------------------------------------------
//  *  Cache Replacement Algorithms - C++20 Implementation (Source File)
//  * -----------------------------------------------------------------------------
//  *
//  *  University: Cornell University
//  *  Author:    I-Hsuan (Ethan) Huang
//  *  Email:     ih246@cornell.edu
//  *  License:   GNU General Public License v3 or later
//  *
//  *  This file contains the implementation of the cache replacement algorithms.
//  *  All class declarations are in CacheReplacementSimulator.hpp
//  *
//  *  Build:
//  *    g++ -std=c++20 -O3 -Wall -o cache_simulator CacheReplacementSimulator.cpp
//  *
//  * -----------------------------------------------------------------------------
//  */

// #include "CacheReplacementSimulator.hpp"

// /*
//  * -----------------------------------------------------------------------------
//  *  Frame Class Implementation
//  * -----------------------------------------------------------------------------
//  */

// Frame::Frame(int idx) : index(idx) {}

// void Frame::reset() {
//     page     = -1;
//     time     = std::chrono::steady_clock::now();
//     extra    = 0;
//     frequency = 0;
//     lastUsed = 0;
// }

// /*
//  * -----------------------------------------------------------------------------
//  *  PageRef Class Implementation
//  * -----------------------------------------------------------------------------
//  */

// PageRef::PageRef(int page, int processId) : pageNum(page), pid(processId) {}

// /*
//  * -----------------------------------------------------------------------------
//  *  Partition Class Implementation
//  * -----------------------------------------------------------------------------
//  */

// Partition::Partition(int partitionSize) : frames(), size(partitionSize) {
//     frames.resize(size);
//     for (int i = 0; i < size; ++i) {
//         frames[i].page = -1;
//         frames[i].frequency = 0;
//         frames[i].lastUsed = 0;
//     }
// }

// bool Partition::hasSpace() const {
//     return std::ranges::any_of(frames, [](const Frame &f) {
//         return f.page == -1;
//     });
// }

// bool Partition::hasPage(int page) const {
//     return std::ranges::any_of(frames, [page](const Frame &f) {
//         return f.page == page;
//     });
// }

// /*
//  * -----------------------------------------------------------------------------
//  *  LFRUData Class Implementation
//  * -----------------------------------------------------------------------------
//  */

// LFRUData::LFRUData() 
//     : privileged(PRIVILEGED_PARTITION_SIZE),
//       unprivileged(UNPRIVILEGED_PARTITION_SIZE) {
//     // Partitions are already initialized by their own constructors.
// }

// /*
//  * -----------------------------------------------------------------------------
//  *  AlgorithmData Class Implementation
//  * -----------------------------------------------------------------------------
//  */

// AlgorithmData::AlgorithmData(int numFrames) {
//     pageTable.reserve(numFrames);
//     for (int i = 0; i < numFrames; ++i) {
//         pageTable.emplace_back(i);
//     }
// }

// double AlgorithmData::getHitRatio() const {
//     int total = hits + misses;
//     return (total > 0) ? static_cast<double>(hits) / total : 0.0;
// }

// /*
//  * -----------------------------------------------------------------------------
//  *  Algorithm Class Implementation
//  * -----------------------------------------------------------------------------
//  */

// Algorithm::Algorithm(const std::string& name, std::function<bool(AlgorithmData&)> func)
//     : label(name), algoFunc(std::move(func)) {
//     // data is allocated in CacheReplacementSimulator::initializeAlgorithms()
// }

// /*
//  * -----------------------------------------------------------------------------
//  *  CacheReplacementSimulator Class Implementation
//  * -----------------------------------------------------------------------------
//  */

// CacheReplacementSimulator::CacheReplacementSimulator() {
//     initializeAlgorithms();
// }

// int CacheReplacementSimulator::getCurrentTime() {
//     static int timeCounter = 0;
//     return ++timeCounter;
// }

// void CacheReplacementSimulator::initializeAlgorithms() {
//     algorithms.clear();

//     // We push algorithms in a fixed order so that selectAlgorithm(char)
//     // can reference them by index (e.g. 'O' → index 0 for OPTIMAL).
//     algorithms.push_back(std::make_unique<Algorithm>(
//         "OPTIMAL", [this](AlgorithmData &d) { return optimal(d); }));

//     algorithms.push_back(std::make_unique<Algorithm>(
//         "RANDOM", [this](AlgorithmData &d) { return random(d); }));

//     algorithms.push_back(std::make_unique<Algorithm>(
//         "FIFO", [this](AlgorithmData &d) { return fifo(d); }));

//     algorithms.push_back(std::make_unique<Algorithm>(
//         "LRU", [this](AlgorithmData &d) { return lru(d); }));

//     algorithms.push_back(std::make_unique<Algorithm>(
//         "CLOCK", [this](AlgorithmData &d) { return clock(d); }));

//     algorithms.push_back(std::make_unique<Algorithm>(
//         "NFU", [this](AlgorithmData &d) { return nfu(d); }));

//     algorithms.push_back(std::make_unique<Algorithm>(
//         "AGING", [this](AlgorithmData &d) { return aging(d); }));

//     algorithms.push_back(std::make_unique<Algorithm>(
//         "MRU", [this](AlgorithmData &d) { return mru(d); }));

//     algorithms.push_back(std::make_unique<Algorithm>(
//         "NRU", [this](AlgorithmData &d) { return nru(d); }));

//     algorithms.push_back(std::make_unique<Algorithm>(
//         "MFU", [this](AlgorithmData &d) { return mfu(d); }));

//     algorithms.push_back(std::make_unique<Algorithm>(
//         "LFU", [this](AlgorithmData &d) { return lfu(d); }));

//     algorithms.push_back(std::make_unique<Algorithm>(
//         "LFRU", [this](AlgorithmData &d) { return lfru(d); }));

//     // For each algorithm, allocate its AlgorithmData(numFrames).
//     for (auto &algoPtr : algorithms) {
//         algoPtr->data = std::make_unique<AlgorithmData>(numFrames);
//         if (algoPtr->label == "LFRU") {
//             // Allocate LFRUData only for the LFRU policy
//             algoPtr->data->lfruData = std::make_unique<LFRUData>();
//         }
//     }
// }

// bool CacheReplacementSimulator::loadPageReferences(const std::string &filename) {
//     std::ifstream file(filename);
//     if (!file.is_open()) {
//         std::cerr << "Error opening file: " << filename << std::endl;
//         return false;
//     }

//     // Clear any previous references
//     pageRefs.clear();

//     int pid, page;
//     while (file >> pid >> page) {
//         pageRefs.emplace_back(page, pid);
//     }

//     // Print a confirmation message
//     std::ostringstream oss;
//     oss << "Loaded " << pageRefs.size()
//         << " page references from " << filename << "\n";
//     std::cout << oss.str();

//     return true;
// }

// void CacheReplacementSimulator::selectAlgorithm(char algoCode) {
//     // Deselect all first
//     for (auto &algoPtr : algorithms) {
//         algoPtr->selected = false;
//     }

//     // Map codes to indices in the 'algorithms' vector
//     switch (algoCode) {
//         case 'O': algorithms[0]->selected  = true; break; // OPTIMAL
//         case 'R': algorithms[1]->selected  = true; break; // RANDOM
//         case 'F': algorithms[2]->selected  = true; break; // FIFO
//         case 'L': algorithms[3]->selected  = true; break; // LRU
//         case 'C': algorithms[4]->selected  = true; break; // CLOCK
//         case 'N': algorithms[5]->selected  = true; break; // NFU
//         case 'A': algorithms[6]->selected  = true; break; // AGING
//         case 'M': algorithms[7]->selected  = true; break; // MRU
//         case 'n': algorithms[8]->selected  = true; break; // NRU
//         case 'm': algorithms[9]->selected  = true; break; // MFU
//         case 'l': algorithms[10]->selected = true; break; // LFU
//         case 'f': algorithms[11]->selected = true; break; // LFRU
//         case 'a':
//             // Select all algorithms
//             for (auto &algoPtr : algorithms) {
//                 algoPtr->selected = true;
//             }
//             break;
//         default:
//             throw std::invalid_argument("Invalid algorithm choice");
//     }
// }

// void CacheReplacementSimulator::runSimulation() {
//     counter = 0;

//     // Avoid exceeding the number of loaded references
//     maxPageCalls = std::min(maxPageCalls, static_cast<int>(pageRefs.size()));

//     for (counter = 0;
//          counter < maxPageCalls && counter < static_cast<int>(pageRefs.size());
//          ++counter)
//     {
//         processPageReference(pageRefs[counter].pageNum);
//     }

//     // After all references, sort algorithms by hit ratio (desc)
//     std::ranges::sort(algorithms,
//         [](const std::unique_ptr<Algorithm> &a,
//            const std::unique_ptr<Algorithm> &b) {
//                return a->data->getHitRatio() > b->data->getHitRatio();
//            });

//     // Print summary for each selected algorithm
//     for (const auto &algoPtr : algorithms) {
//         if (algoPtr->selected) {
//             printSummary(*algoPtr);
//         }
//     }
// }

// void CacheReplacementSimulator::processPageReference(int pageRef) {
//     lastPageRef = pageRef;

//     for (auto &algoPtr : algorithms) {
//         if (!algoPtr->selected) {
//             continue;
//         }

//         // Time measurement around the core algorithm call
//         auto tStart = std::chrono::high_resolution_clock::now();
//         bool fault = algoPtr->algoFunc(*algoPtr->data);
//         auto tEnd = std::chrono::high_resolution_clock::now();

//         // Accumulate execution time
//         algoPtr->data->execTime += std::chrono::duration<double>(tEnd - tStart);

//         // Update hits/misses
//         if (fault) {
//             ++(algoPtr->data->misses);
//         } else {
//             ++(algoPtr->data->hits);
//         }

//         // Optionally print page table after this reference
//         if (printRefs) {
//             printStats(*algoPtr);
//         }
//     }
// }

// /*
//  * -----------------------------------------------------------------------------
//  *  Algorithm Implementations
//  * -----------------------------------------------------------------------------
//  */

// bool CacheReplacementSimulator::optimal(AlgorithmData &data) {
//     // 1) Check for hit in pageTable
//     auto hitIt = std::ranges::find_if(data.pageTable,
//         [this](const Frame &f) {
//             return f.page == lastPageRef;
//         });

//     if (hitIt != data.pageTable.end()) {
//         // Page hit: update time and extra
//         hitIt->time  = std::chrono::steady_clock::now();
//         hitIt->extra = counter;
//         return false; // No fault
//     }

//     // 2) Check for empty frame
//     auto emptyIt = std::ranges::find_if(data.pageTable,
//         [](const Frame &f) {
//             return f.page == -1;
//         });

//     if (emptyIt != data.pageTable.end()) {
//         // Use empty slot
//         emptyIt->page  = lastPageRef;
//         emptyIt->time  = std::chrono::steady_clock::now();
//         emptyIt->extra = counter;
//         return true; // Miss, but no eviction occurred
//     }

//     // 3) Find victim using optimal strategy
//     int maxDistance = -1;
//     auto victimIt = data.pageTable.begin();

//     for (auto it = data.pageTable.begin(); it != data.pageTable.end(); ++it) {
//         int distance = std::numeric_limits<int>::max();
        
//         // Look ahead to find when this page is used next
//         for (std::size_t i = static_cast<std::size_t>(counter) + 1; 
//              i < pageRefs.size(); 
//              ++i) {
//             if (pageRefs[i].pageNum == it->page) {
//                 distance = static_cast<int>(i);
//                 break;
//             }
//         }
        
//         // Choose the page that is used furthest in the future
//         if (distance > maxDistance) {
//             maxDistance = distance;
//             victimIt = it;
//         }
//     }

//     // Evict the victim
//     data.victimList.push_back(*victimIt);
//     victimIt->page  = lastPageRef;
//     victimIt->time  = std::chrono::steady_clock::now();
//     victimIt->extra = counter;

//     return true; // Miss (eviction occurred)
// }

// bool CacheReplacementSimulator::random(AlgorithmData &data) {
//     // 1) HIT?
//     auto hitIt = std::ranges::find_if(data.pageTable,
//         [this](const Frame &f) { return f.page == lastPageRef; });

//     if (hitIt != data.pageTable.end()) {
//         hitIt->time  = std::chrono::steady_clock::now();
//         hitIt->extra = counter;
//         return false; // Hit
//     }

//     // 2) Empty slot?
//     auto emptyIt = std::ranges::find_if(data.pageTable,
//         [](const Frame &f) { return f.page == -1; });

//     if (emptyIt != data.pageTable.end()) {
//         emptyIt->page  = lastPageRef;
//         emptyIt->time  = std::chrono::steady_clock::now();
//         emptyIt->extra = counter;
//         return true; // Miss (no eviction)
//     }

//     // 3) Choose random victim index in [0..numFrames-1]
//     std::uniform_int_distribution<> dist(0, numFrames - 1);
//     int victimIndex = dist(rng);

//     // 4) Evict that frame
//     data.victimList.push_back(data.pageTable[victimIndex]);
//     data.pageTable[victimIndex].page  = lastPageRef;
//     data.pageTable[victimIndex].time  = std::chrono::steady_clock::now();
//     data.pageTable[victimIndex].extra = counter;

//     return true; // Miss (eviction occurred)
// }

// bool CacheReplacementSimulator::fifo(AlgorithmData &data) {
//     // 1) Check for hit
//     auto hitIt = std::ranges::find_if(data.pageTable,
//         [this](const Frame &f) { return f.page == lastPageRef; });
//     if (hitIt != data.pageTable.end()) {
//         // IMPORTANT: In true FIFO, hits don't update the insertion time!
//         // The frame stays in its original position in the queue
//         return false; // Hit - no time update for FIFO
//     }

//     // 2) Check for empty slot
//     auto emptyIt = std::ranges::find_if(data.pageTable,
//         [](const Frame &f) { return f.page == -1; });
//     if (emptyIt != data.pageTable.end()) {
//         emptyIt->page  = lastPageRef;
//         emptyIt->time  = std::chrono::steady_clock::now();
//         emptyIt->extra = counter;
//         return true; // Miss (no eviction)
//     }

//     // 3) Find oldest frame by insertion time (smallest extra value)
//     auto victimIt = std::ranges::min_element(data.pageTable,
//         [](const Frame &a, const Frame &b) {
//             return a.extra < b.extra;  // Use extra as insertion order
//         });

//     if (victimIt != data.pageTable.end()) {
//         data.victimList.push_back(*victimIt);
//         victimIt->page  = lastPageRef;
//         victimIt->time  = std::chrono::steady_clock::now();
//         victimIt->extra = counter;  // Set insertion order
//     }

//     return true; // Miss (eviction)
// }

// bool CacheReplacementSimulator::lru(AlgorithmData &data) {
//     // 1) Hit?
//     auto hitIt = std::ranges::find_if(data.pageTable,
//         [this](const Frame &f) { return f.page == lastPageRef; });
//     if (hitIt != data.pageTable.end()) {
//         hitIt->time  = std::chrono::steady_clock::now();
//         hitIt->extra = counter;
//         return false; // Hit
//     }

//     // 2) Empty?
//     auto emptyIt = std::ranges::find_if(data.pageTable,
//         [](const Frame &f) { return f.page == -1; });
//     if (emptyIt != data.pageTable.end()) {
//         emptyIt->page  = lastPageRef;
//         emptyIt->time  = std::chrono::steady_clock::now();
//         emptyIt->extra = counter;
//         return true; // Miss (no eviction)
//     }

//     // 3) Find least recently used (smallest time)
//     auto victimIt = std::ranges::min_element(data.pageTable,
//         [](const Frame &a, const Frame &b) {
//             return a.time < b.time;
//         });

//     if (victimIt != data.pageTable.end()) {
//         data.victimList.push_back(*victimIt);
//         victimIt->page  = lastPageRef;
//         victimIt->time  = std::chrono::steady_clock::now();
//         victimIt->extra = counter;
//     }

//     return true; // Miss (eviction)
// }

// bool CacheReplacementSimulator::clock(AlgorithmData &data) {
//     static int clockHand = 0;

//     // Ensure clockHand is within bounds
//     if (clockHand >= numFrames) {
//         clockHand = 0;
//     }

//     // 1) Check for hit - set reference bit
//     auto hitIt = std::ranges::find_if(data.pageTable,
//         [this](const Frame &f) { return f.page == lastPageRef; });
//     if (hitIt != data.pageTable.end()) {
//         hitIt->extra = 1;  // Set reference bit
//         return false; // Hit
//     }

//     // 2) Check for empty slot
//     auto emptyIt = std::ranges::find_if(data.pageTable,
//         [](const Frame &f) { return f.page == -1; });
//     if (emptyIt != data.pageTable.end()) {
//         emptyIt->page  = lastPageRef;
//         emptyIt->extra = 1;  // Set reference bit for newly loaded page
//         return true; // Miss (no eviction)
//     }

//     // 3) Clock algorithm: find first frame with reference bit = 0
//     int startHand = clockHand;
//     do {
//         if (data.pageTable[clockHand].extra == 0) {
//             // Found victim with reference bit = 0
//             data.victimList.push_back(data.pageTable[clockHand]);
//             data.pageTable[clockHand].page  = lastPageRef;
//             data.pageTable[clockHand].extra = 1;  // Set reference bit
//             clockHand = (clockHand + 1) % numFrames;
//             return true; // Miss (eviction)
//         } else {
//             // Give second chance: clear reference bit and move hand
//             data.pageTable[clockHand].extra = 0;
//             clockHand = (clockHand + 1) % numFrames;
//         }
//     } while (clockHand != startHand);

//     // If we've made a full circle, evict current position
//     data.victimList.push_back(data.pageTable[clockHand]);
//     data.pageTable[clockHand].page  = lastPageRef;
//     data.pageTable[clockHand].extra = 1;
//     clockHand = (clockHand + 1) % numFrames;
    
//     return true; // Miss (eviction)
// }

// bool CacheReplacementSimulator::nfu(AlgorithmData &data) {
//     // 1) Hit? Increment NFU count
//     auto hitIt = std::ranges::find_if(data.pageTable,
//         [this](const Frame &f) { return f.page == lastPageRef; });
//     if (hitIt != data.pageTable.end()) {
//         hitIt->extra++; // Increase NFU count
//         hitIt->time = std::chrono::steady_clock::now(); // Optional
//         return false; // Hit
//     }

//     // 2) Empty slot?
//     auto emptyIt = std::ranges::find_if(data.pageTable,
//         [](const Frame &f) { return f.page == -1; });
//     if (emptyIt != data.pageTable.end()) {
//         emptyIt->page  = lastPageRef;
//         emptyIt->time  = std::chrono::steady_clock::now();
//         emptyIt->extra = 0; // Start count at zero
//         return true; // Miss
//     }

//     // 3) Evict frame with lowest extra (NFU count)
//     auto victimIt = std::ranges::min_element(data.pageTable,
//         [](const Frame &a, const Frame &b) {
//             return a.extra < b.extra;
//         });

//     if (victimIt != data.pageTable.end()) {
//         data.victimList.push_back(*victimIt);
//         victimIt->page  = lastPageRef;
//         victimIt->time  = std::chrono::steady_clock::now();
//         victimIt->extra = 0; // Reset count for new page
//     }

//     return true; // Miss (eviction)
// }

// bool CacheReplacementSimulator::aging(AlgorithmData &data) {
//     // 1) Age all frames by dividing extra by 2
//     for (auto &frame : data.pageTable) {
//         if (frame.page != -1) {
//             frame.extra /= 2;
//         }
//     }

//     // 2) Hit? Add LARGE_CONST to mark recent use
//     auto hitIt = std::ranges::find_if(data.pageTable,
//         [this](const Frame &f) { return f.page == lastPageRef; });
//     if (hitIt != data.pageTable.end()) {
//         // Add a large offset to represent "just used"
//         hitIt->extra += 10000000;
//         hitIt->time  = std::chrono::steady_clock::now();
//         return false; // Hit
//     }

//     // 3) Empty slot? Fill with extra=0
//     auto emptyIt = std::ranges::find_if(data.pageTable,
//         [](const Frame &f) { return f.page == -1; });
//     if (emptyIt != data.pageTable.end()) {
//         emptyIt->page  = lastPageRef;
//         emptyIt->time  = std::chrono::steady_clock::now();
//         emptyIt->extra = 0;
//         return true; // Miss
//     }

//     // 4) Evict frame with smallest extra (lowest aged value)
//     auto victimIt = std::ranges::min_element(data.pageTable,
//         [](const Frame &a, const Frame &b) {
//             return a.extra < b.extra;
//         });

//     if (victimIt != data.pageTable.end()) {
//         data.victimList.push_back(*victimIt);
//         victimIt->page  = lastPageRef;
//         victimIt->time  = std::chrono::steady_clock::now();
//         victimIt->extra = 0;
//     }

//     return true; // Miss (eviction)
// }

// bool CacheReplacementSimulator::mru(AlgorithmData &data) {
//     // 1) Hit?
//     auto hitIt = std::ranges::find_if(data.pageTable,
//         [this](const Frame &f) { return f.page == lastPageRef; });
//     if (hitIt != data.pageTable.end()) {
//         hitIt->time = std::chrono::steady_clock::now();
//         return false; // Hit
//     }

//     // 2) Empty?
//     auto emptyIt = std::ranges::find_if(data.pageTable,
//         [](const Frame &f) { return f.page == -1; });
//     if (emptyIt != data.pageTable.end()) {
//         emptyIt->page = lastPageRef;
//         emptyIt->time = std::chrono::steady_clock::now();
//         return true; // Miss
//     }

//     // 3) Evict MRU = frame with largest time
//     auto victimIt = std::ranges::max_element(data.pageTable,
//         [](const Frame &a, const Frame &b) {
//             return a.time < b.time;
//         });

//     if (victimIt != data.pageTable.end()) {
//         data.victimList.push_back(*victimIt);
//         victimIt->page = lastPageRef;
//         victimIt->time = std::chrono::steady_clock::now();
//     }

//     return true; // Miss (eviction)
// }

// bool CacheReplacementSimulator::nru(AlgorithmData &data) {
//     // 1) Hit?
//     auto hitIt = std::ranges::find_if(data.pageTable,
//         [this](const Frame &f) { return f.page == lastPageRef; });
//     if (hitIt != data.pageTable.end()) {
//         hitIt->time = std::chrono::steady_clock::now();
//         return false; // Hit
//     }

//     // 2) Empty?
//     auto emptyIt = std::ranges::find_if(data.pageTable,
//         [](const Frame &f) { return f.page == -1; });
//     if (emptyIt != data.pageTable.end()) {
//         emptyIt->page = lastPageRef;
//         emptyIt->time = std::chrono::steady_clock::now();
//         return true; // Miss
//     }

//     // 3) Evict oldest (min time)
//     auto victimIt = std::ranges::min_element(data.pageTable,
//         [](const Frame &a, const Frame &b) {
//             return a.time < b.time;
//         });
//     if (victimIt != data.pageTable.end()) {
//         data.victimList.push_back(*victimIt);
//         victimIt->page = lastPageRef;
//         victimIt->time = std::chrono::steady_clock::now();
//     }

//     return true; // Miss (eviction)
// }

// bool CacheReplacementSimulator::mfu(AlgorithmData &data) {
//     // 1) Hit? increment frequency
//     auto hitIt = std::ranges::find_if(data.pageTable,
//         [this](const Frame &f) { return f.page == lastPageRef; });
//     if (hitIt != data.pageTable.end()) {
//         hitIt->extra++; // Increase usage frequency
//         return false;   // Hit
//     }

//     // 2) Empty slot?
//     auto emptyIt = std::ranges::find_if(data.pageTable,
//         [](const Frame &f) { return f.page == -1; });
//     if (emptyIt != data.pageTable.end()) {
//         emptyIt->page  = lastPageRef;
//         emptyIt->extra = 1; // Start frequency at 1
//         return true; // Miss
//     }

//     // 3) Evict MFU = frame with largest extra
//     auto victimIt = std::ranges::max_element(data.pageTable,
//         [](const Frame &a, const Frame &b) {
//             return a.extra < b.extra;
//         });
//     if (victimIt != data.pageTable.end()) {
//         data.victimList.push_back(*victimIt);
//         victimIt->page  = lastPageRef;
//         victimIt->extra = 1; // Reset frequency
//     }

//     return true; // Miss (eviction)
// }

// bool CacheReplacementSimulator::lfu(AlgorithmData &data) {
//     // 1) HIT? update frequency and lastUsed
//     auto hitIt = std::ranges::find_if(data.pageTable,
//         [this](const Frame &f) { return f.page == lastPageRef; });
//     if (hitIt != data.pageTable.end()) {
//         hitIt->frequency++;
//         hitIt->lastUsed = getCurrentTime();  // Update for tie-break
//         return false; // Hit
//     }

//     // 2) EMPTY? fill with (page, frequency=1, lastUsed=now)
//     auto emptyIt = std::ranges::find_if(data.pageTable,
//         [](const Frame &f) { return f.page == -1; });
//     if (emptyIt != data.pageTable.end()) {
//         emptyIt->page       = lastPageRef;
//         emptyIt->frequency  = 1;
//         emptyIt->lastUsed   = getCurrentTime();
//         return true; // Miss (no eviction)
//     }

//     // 3) Evict LFU (frequency, then lastUsed)
//     auto victimIt = std::ranges::min_element(data.pageTable,
//         [](const Frame &a, const Frame &b) {
//             if (a.frequency != b.frequency) {
//                 return a.frequency < b.frequency;
//             } else {
//                 // tie-break by lastUsed
//                 return a.lastUsed < b.lastUsed;
//             }
//         });

//     if (victimIt != data.pageTable.end()) {
//         data.victimList.push_back(*victimIt);
//         victimIt->page       = lastPageRef;
//         victimIt->frequency  = 1;
//         victimIt->lastUsed   = getCurrentTime();
//     }

//     return true; // Miss (eviction)
// }

// bool CacheReplacementSimulator::lfru(AlgorithmData &data) {
//     // Allocate LFRUData if not already
//     if (!data.lfruData) {
//         data.lfruData = std::make_unique<LFRUData>();
//     }

//     LFRUData &lf = *data.lfruData;

//     // 1) Page in privileged (LRU)? Promote by updating lastUsed
//     if (lf.privileged.hasPage(lastPageRef)) {
//         updateLRU(lf.privileged, lastPageRef);
//         return false; // Hit
//     }

//     // 2) Page in unprivileged (LFU)? Promote into privileged
//     if (lf.unprivileged.hasPage(lastPageRef)) {
//         // Remove it from unprivileged partition
//         removeFromPartition(lf.unprivileged, lastPageRef);

//         // If privileged is full, demote least-recently used from privileged
//         if (!lf.privileged.hasSpace()) {
//             int demotedPage = demoteLRU(lf.privileged);
//             // Now insert demotedPage into unprivileged:
//             if (!lf.unprivileged.hasSpace()) {
//                 evictLFU(lf.unprivileged);
//             }
//             insertIntoPartition(lf.unprivileged, demotedPage);
//         }

//         // Insert lastPageRef into privileged
//         insertIntoPartition(lf.privileged, lastPageRef);
//         return false; // Hit (no fault)
//     }

//     // 3) Page fault: insertion logic
//     handlePageInsertion(lf, lastPageRef);
//     return true; // Miss (fault)
// }

// /*
//  * -----------------------------------------------------------------------------
//  *  LFRU Helper Functions
//  * -----------------------------------------------------------------------------
//  */

// void CacheReplacementSimulator::updateLRU(Partition &partition, int page) {
//     auto it = std::ranges::find_if(partition.frames,
//         [page](const Frame &f) { return f.page == page; });
//     if (it != partition.frames.end()) {
//         it->lastUsed = getCurrentTime();
//     }
// }

// void CacheReplacementSimulator::updateLFU(Partition &partition, int page) {
//     auto it = std::ranges::find_if(partition.frames,
//         [page](const Frame &f) { return f.page == page; });
//     if (it != partition.frames.end()) {
//         it->frequency++;
//     }
// }

// void CacheReplacementSimulator::removeFromPartition(Partition &partition, int page) {
//     auto it = std::ranges::find_if(partition.frames,
//         [page](const Frame &f) { return f.page == page; });
//     if (it != partition.frames.end()) {
//         it->page = -1;
//         it->frequency = 0;
//         it->lastUsed  = 0;
//         it->extra     = 0;
//         it->time      = std::chrono::steady_clock::now();
//     }
// }

// void CacheReplacementSimulator::handlePageInsertion(LFRUData &lf, int page) {
//     if (lf.privileged.hasSpace()) {
//         insertIntoPartition(lf.privileged, page);
//     } else {
//         // Privileged is full → demote one LRU from privileged
//         int demotedPage = demoteLRU(lf.privileged);
//         // Now ensure unprivileged has space
//         if (!lf.unprivileged.hasSpace()) {
//             evictLFU(lf.unprivileged);
//         }
//         insertIntoPartition(lf.unprivileged, demotedPage);
//         insertIntoPartition(lf.privileged, page);
//     }
// }

// void CacheReplacementSimulator::insertIntoPartition(Partition &partition, int page) {
//     auto it = std::ranges::find_if(partition.frames,
//         [](const Frame &f) { return f.page == -1; });
//     if (it != partition.frames.end()) {
//         it->page      = page;
//         it->lastUsed  = getCurrentTime();
//         it->frequency = 1;
//     }
// }

// int CacheReplacementSimulator::evictLFU(Partition &partition) {
//     auto victimIt = std::ranges::min_element(partition.frames,
//         [](const Frame &a, const Frame &b) {
//             if (a.frequency != b.frequency) {
//                 return a.frequency < b.frequency;
//             } else {
//                 return a.lastUsed < b.lastUsed;
//             }
//         });
//     if (victimIt != partition.frames.end()) {
//         int evictedPage = victimIt->page;
//         victimIt->page      = -1;
//         victimIt->frequency = 0;
//         victimIt->lastUsed  = 0;
//         victimIt->extra     = 0;
//         victimIt->time      = std::chrono::steady_clock::now();
//         return evictedPage;
//     }
//     return -1;
// }

// int CacheReplacementSimulator::demoteLRU(Partition &partition) {
//     auto victimIt = std::ranges::min_element(partition.frames,
//         [](const Frame &a, const Frame &b) {
//             return a.lastUsed < b.lastUsed;
//         });
//     if (victimIt != partition.frames.end()) {
//         int demotedPage = victimIt->page;
//         victimIt->page     = -1;
//         victimIt->frequency = 0;
//         victimIt->lastUsed  = 0;
//         victimIt->extra     = 0;
//         victimIt->time      = std::chrono::steady_clock::now();
//         return demotedPage;
//     }
//     return -1;
// }

// /*
//  * -----------------------------------------------------------------------------
//  *  Configuration and Utility Functions
//  * -----------------------------------------------------------------------------
//  */

// void CacheReplacementSimulator::setConfiguration(int frames,
//                       int maxCalls,
//                       bool debugMode,
//                       bool printRefsMode)
// {
//     numFrames     = std::max(1, frames);
//     maxPageCalls  = maxCalls;
//     debug         = debugMode;
//     printRefs     = printRefsMode;

//     // Re-initialize all algorithms with updated frame count
//     initializeAlgorithms();
// }

// void CacheReplacementSimulator::printHelp(const std::string &programName) {
//     std::ostringstream oss;
//     oss << "usage: " << programName
//         << " <input_file> <algorithm> <num_frames> [show_process] [debug]\n";
//     oss << "   input_file    - input test file\n";
//     oss << "   algorithm     - page algorithm to use {O,R,F,L,C,N,A,M,n,m,l,f,a}\n";
//     oss << "                   O=OPTIMAL, R=RANDOM, F=FIFO, L=LRU, C=CLOCK\n";
//     oss << "                   N=NFU, A=AGING, M=MRU, n=NRU, m=MFU, l=LFU, f=LFRU, a=ALL\n";
//     oss << "   num_frames    - number of page frames {int > 1}\n";
//     oss << "   show_process  - print page table after each ref is processed {1 or 0}\n";
//     oss << "   debug         - verbose debugging output {1 or 0}\n";
//     std::cout << oss.str();
// }

// /*
//  * -----------------------------------------------------------------------------
//  *  Printing / Reporting Functions
//  * -----------------------------------------------------------------------------
//  */

// void CacheReplacementSimulator::printSummary(const Algorithm &algo) const {
//     std::ostringstream oss;
//     oss << algo.label << " Algorithm\n";
//     oss << "Frames in Mem: " << numFrames << ", ";
//     oss << "Hits: "         << algo.data->hits   << ", ";
//     oss << "Misses: "       << algo.data->misses << ", ";
//     oss << "Hit Ratio: "    << std::fixed << std::setprecision(6)
//         << algo.data->getHitRatio() << ", ";
//     oss << "Total Execution Time: " << std::fixed << std::setprecision(6)
//         << algo.data->execTime.count() << " seconds\n";
//     std::cout << oss.str();
// }

// void CacheReplacementSimulator::printStats(const Algorithm &algo) const {
//     printSummary(algo);
//     printPageTable(algo.data->pageTable);
// }

// void CacheReplacementSimulator::printPageTable(const std::vector<Frame> &pageTable) const {
//     constexpr int colSize   = 9;
//     constexpr int labelSize = 12;

//     std::ostringstream oss;

//     // Row 1: Frame #
//     oss << std::setw(labelSize) << std::left << "Frame #" << " : ";
//     for (const auto &frame : pageTable) {
//         oss << std::setw(colSize) << frame.index;
//     }
//     oss << "\n";

//     // Row 2: Page Ref
//     oss << std::setw(labelSize) << std::left << "Page Ref" << " : ";
//     for (const auto &frame : pageTable) {
//         if (frame.page == -1) {
//             oss << std::setw(colSize) << "_";
//         } else {
//             oss << std::setw(colSize) << frame.page;
//         }
//     }
//     oss << "\n";

//     // Row 3: Extra (R-bit or NFU counter or aging value)
//     oss << std::setw(labelSize) << std::left << "Extra" << " : ";
//     for (const auto &frame : pageTable) {
//         oss << std::setw(colSize) << frame.extra;
//     }
//     oss << "\n";

//     // Row 4: Time (modded)
//     oss << std::setw(labelSize) << std::left << "Time" << " : ";
//     for (const auto &frame : pageTable) {
//         auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
//                       frame.time.time_since_epoch()
//                   ).count();
//         oss << std::setw(colSize) << (ms % 200000000);
//     }
//     oss << "\n\n";

//     std::cout << oss.str();
// }



























/*
 * -----------------------------------------------------------------------------
 *  Cache Replacement Algorithms - C++20 Implementation (Source File)
 * -----------------------------------------------------------------------------
 *
 *  University: Cornell University
 *  Author:    I-Hsuan (Ethan) Huang
 *  Email:     ih246@cornell.edu
 *  License:   GNU General Public License v3 or later
 *
 *  This file contains the implementation of the cache replacement algorithms.
 *  All class declarations are in CacheReplacementSimulator.hpp
 *
 *  Build:
 *    g++ -std=c++20 -O3 -Wall -o cache_simulator CacheReplacementSimulator.cpp
 *
 * -----------------------------------------------------------------------------
 */

#include "CacheReplacementSimulator.hpp"
#include <sstream>      // For std::ostringstream
#include <iomanip>      // For std::setw, std::setprecision
#include <ranges>       // For std::ranges::find_if, std::ranges::min_element, etc.
#include <chrono>       // For std::chrono::steady_clock, high_resolution_clock
#include <algorithm>    // For std::ranges::sort
#include <random>       // For std::mt19937, std::uniform_int_distribution

/*
 * -----------------------------------------------------------------------------
 *  Frame Class Implementation
 * -----------------------------------------------------------------------------
 *
 *  A Frame represents a single cache frame (or physical memory frame).
 *  - index: integer identifier for this frame (0..numFrames-1)
 *  - page:  integer page number currently loaded, or -1 if empty
 *  - time:  timestamp of last access or insertion (varies by policy)
 *  - extra: generic field used differently by each replacement policy
 *           (e.g., reference bit, NFU counter, aging register, etc.)
 *  - frequency: used by LFU/MFU as usage count
 *  - lastUsed: logical counter for tie-breakers (LFU, demotion in LFRU, etc.)
 *
 *  In virtually every replacement function:
 *    - On a page hit: update one or more of these fields (time, extra, frequency, lastUsed)
 *    - On a miss: either find an empty Frame (page == -1) or evict one selected by the policy.
 * -----------------------------------------------------------------------------
 */

Frame::Frame(int idx) : index(idx) {}

void Frame::reset() {
    // Reset the frame to an “empty” state
    page      = -1;                                 // No page loaded
    time      = std::chrono::steady_clock::now();   // Reset timestamp
    extra     = 0;                                  // Clear policy‐specific value
    frequency = 0;                                  // Clear usage count
    lastUsed  = 0;                                  // Clear logical last‐used counter
}

/*
 * -----------------------------------------------------------------------------
 *  PageRef Class Implementation
 * -----------------------------------------------------------------------------
 *
 *  A PageRef represents a single reference from the trace.
 *  - pageNum: integer page number being accessed
 *  - pid:     process ID or thread ID (unused by these policies, but stored)
 * -----------------------------------------------------------------------------
 */

PageRef::PageRef(int page, int processId) : pageNum(page), pid(processId) {}

/*
 * -----------------------------------------------------------------------------
 *  Partition Class Implementation (for LFRU)
 * -----------------------------------------------------------------------------
 *
 *  For LFRU, we maintain two partitions:
 *    - privileged:   a small LRU cache
 *    - unprivileged: a small LFU cache
 *
 *  Partition:
 *    - frames: vector<Frame> of fixed size “size”
 *    - hasSpace(): returns true if any frame.page == -1
 *    - hasPage(int): returns true if any frame.page == given page
 *
 *  Each Partition is used to store frames; when inserting, we look for an empty slot,
 *  else we evict one frame according to the partition’s policy (LRU or LFU).
 * -----------------------------------------------------------------------------
 */

Partition::Partition(int partitionSize) : frames(), size(partitionSize) {
    frames.resize(size);
    for (int i = 0; i < size; ++i) {
        frames[i].page      = -1;  // Mark as empty
        frames[i].frequency = 0;   // Clear LFU counter
        frames[i].lastUsed  = 0;   // Clear LRU timestamp (logical)
    }
}

bool Partition::hasSpace() const {
    // True if any frame is empty (page == -1)
    return std::ranges::any_of(frames, [](const Frame &f) {
        return f.page == -1;
    });
}

bool Partition::hasPage(int page) const {
    // True if any frame currently holds that page
    return std::ranges::any_of(frames, [page](const Frame &f) {
        return f.page == page;
    });
}

/*
 * -----------------------------------------------------------------------------
 *  LFRUData Class Implementation
 * -----------------------------------------------------------------------------
 *
 *  Holds two partitions:
 *    - privileged   (LRU of size PRIVILEGED_PARTITION_SIZE)
 *    - unprivileged (LFU of size UNPRIVILEGED_PARTITION_SIZE)
 *
 *  LFRU (Least Frequently Recently Used) uses these partitions to approximate
 *  a hybrid of LFU and LRU:
 *    - Privileged partition: functions as an LRU cache
 *    - Unprivileged partition: functions as an LFU cache
 *
 *  On a page hit in privileged → update LRU timestamp.
 *  On a page hit in unprivileged → promote to privileged (demoting one LRU if needed).
 *  On a miss → handle by inserting into privileged (evicting/demoting if full).
 * -----------------------------------------------------------------------------
 */

LFRUData::LFRUData() 
    : privileged(PRIVILEGED_PARTITION_SIZE),
      unprivileged(UNPRIVILEGED_PARTITION_SIZE) {
    // Partitions are already initialized in their own constructors.
}

/*
 * -----------------------------------------------------------------------------
 *  AlgorithmData Class Implementation
 * -----------------------------------------------------------------------------
 *
 *  Holds per‐algorithm runtime data:
 *    - hits, misses: counters
 *    - pageTable:   vector<Frame> of size numFrames, representing resident frames
 *    - victimList:  records each evicted frame (for debugging/logging)
 *    - execTime:    total CPU time spent inside this policy’s per‐reference function
 *    - lfruData:    pointer used only by the LFRU policy to store its two partitions
 *
 *  getHitRatio(): returns hits / (hits + misses)
 * -----------------------------------------------------------------------------
 */

AlgorithmData::AlgorithmData(int numFrames) {
    pageTable.reserve(numFrames);
    for (int i = 0; i < numFrames; ++i) {
        pageTable.emplace_back(i);
    }
}

double AlgorithmData::getHitRatio() const {
    int total = hits + misses;
    return (total > 0) ? static_cast<double>(hits) / total : 0.0;
}

/*
 * -----------------------------------------------------------------------------
 *  Algorithm Class Implementation
 * -----------------------------------------------------------------------------
 *
 *  The Algorithm class bundles:
 *    - label:    string name (e.g., “LRU”, “CLOCK”)
 *    - algoFunc: std::function<bool(AlgorithmData&)> that implements the per‐reference logic
 *    - selected: bool that indicates whether this policy was chosen by the user
 *    - data:     unique_ptr<AlgorithmData> holding hits, misses, pageTable, etc.
 *
 *  The function signature bool func(AlgorithmData&) returns:
 *    - false → page hit (no fault)
 *    - true  → page miss (fault, possibly eviction)
 * -----------------------------------------------------------------------------
 */

Algorithm::Algorithm(const std::string& name, std::function<bool(AlgorithmData&)> func)
    : label(name), algoFunc(std::move(func)) {
    // `data` is allocated in CacheReplacementSimulator::initializeAlgorithms()
}

/*
 * -----------------------------------------------------------------------------
 *  CacheReplacementSimulator Class Implementation
 * -----------------------------------------------------------------------------
 *
 *  This class orchestrates:
 *    - Loading the page reference trace file
 *    - Selecting which replacement policy/policies to run
 *    - Iterating over each page reference, invoking each selected policy,
 *      timing it, and tallying hits/misses
 *    - Sorting policies by hit ratio at the end and printing summaries
 *
 *  Key member variables:
 *    - numFrames:        total number of frames in the cache
 *    - pageRefUpperBound: maximum possible page number used for OPTIMAL’s array
 *    - maxPageCalls:     limit on # of references to process (e.g. 1000 by default)
 *    - debug:            if true, might print extra logging (unused here)
 *    - printRefs:        if true, prints the page table after each reference
 *    - counter:          logical reference‐counter (0..maxPageCalls-1)
 *    - lastPageRef:      the page number of the most recent reference
 *    - pageRefs:         vector<PageRef> loaded from the trace file
 *    - algorithms:       vector<unique_ptr<Algorithm>> of all policies
 *    - rng:              random number generator for RANDOM policy
 *
 *  The main workflow:
 *    1) Constructor calls initializeAlgorithms() to create all Algorithm objects
 *    2) loadPageReferences() reads the input file into pageRefs
 *    3) selectAlgorithm(char) toggles which policies are “selected”
 *    4) runSimulation() loops over up to maxPageCalls references:
 *        - processPageReference(pageNum): for each selected policy:
 *            * record start time
 *            * call policy‐specific function (updating pageTable,
 *              possibly evicting, etc.)
 *            * record end time, update execTime, hits/misses
 *            * if printRefs, call printStats() to dump page table
 *    5) After processing, sort policies by hit ratio (desc)
 *    6) Print summary for each selected policy
 * -----------------------------------------------------------------------------
 */

CacheReplacementSimulator::CacheReplacementSimulator() {
    initializeAlgorithms();
}

int CacheReplacementSimulator::getCurrentTime() {
    // Returns a simple incrementing integer, used for LFU tie-breakers,
    // LFRU lastUsed timestamps, etc.
    static int timeCounter = 0;
    return ++timeCounter;
}

void CacheReplacementSimulator::initializeAlgorithms() {
    // Clear any existing policy list
    algorithms.clear();

    // Add each policy in a fixed order so selectAlgorithm(char) can refer
    // to the correct index. The order below must match the switch in selectAlgorithm.
    algorithms.push_back(std::make_unique<Algorithm>(
        "OPTIMAL", [this](AlgorithmData &d) { return optimal(d); }));
    algorithms.push_back(std::make_unique<Algorithm>(
        "RANDOM",  [this](AlgorithmData &d) { return random(d); }));
    algorithms.push_back(std::make_unique<Algorithm>(
        "FIFO",    [this](AlgorithmData &d) { return fifo(d); }));
    algorithms.push_back(std::make_unique<Algorithm>(
        "LRU",     [this](AlgorithmData &d) { return lru(d); }));
    algorithms.push_back(std::make_unique<Algorithm>(
        "CLOCK",   [this](AlgorithmData &d) { return clock(d); }));
    algorithms.push_back(std::make_unique<Algorithm>(
        "NFU",     [this](AlgorithmData &d) { return nfu(d); }));
    algorithms.push_back(std::make_unique<Algorithm>(
        "AGING",   [this](AlgorithmData &d) { return aging(d); }));
    algorithms.push_back(std::make_unique<Algorithm>(
        "MRU",     [this](AlgorithmData &d) { return mru(d); }));
    algorithms.push_back(std::make_unique<Algorithm>(
        "NRU",     [this](AlgorithmData &d) { return nru(d); }));
    algorithms.push_back(std::make_unique<Algorithm>(
        "MFU",     [this](AlgorithmData &d) { return mfu(d); }));
    algorithms.push_back(std::make_unique<Algorithm>(
        "LFU",     [this](AlgorithmData &d) { return lfu(d); }));
    algorithms.push_back(std::make_unique<Algorithm>(
        "LFRU",    [this](AlgorithmData &d) { return lfru(d); }));

    // Allocate AlgorithmData for each policy, of size numFrames
    for (auto &algoPtr : algorithms) {
        algoPtr->data = std::make_unique<AlgorithmData>(numFrames);
        if (algoPtr->label == "LFRU") {
            // Only LFRU uses its LFRUData struct
            algoPtr->data->lfruData = std::make_unique<LFRUData>();
        }
    }
}

bool CacheReplacementSimulator::loadPageReferences(const std::string &filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return false;
    }

    pageRefs.clear();
    int pid, page;
    while (file >> pid >> page) {
        pageRefs.emplace_back(page, pid);
    }

    std::ostringstream oss;
    oss << "Loaded " << pageRefs.size()
        << " page references from " << filename << "\n";
    std::cout << oss.str();

    return true;
}

void CacheReplacementSimulator::selectAlgorithm(char algoCode) {
    // Deselect all policies first
    for (auto &algoPtr : algorithms) {
        algoPtr->selected = false;
    }

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
            // Select ALL policies
            for (auto &algoPtr : algorithms) {
                algoPtr->selected = true;
            }
            break;
        default:
            throw std::invalid_argument("Invalid algorithm choice");
    }
}

void CacheReplacementSimulator::runSimulation() {
    counter = 0;

    // Ensure we do not iterate beyond the loaded references
    maxPageCalls = std::min(maxPageCalls, static_cast<int>(pageRefs.size()));

    for (counter = 0;
         counter < maxPageCalls && counter < static_cast<int>(pageRefs.size());
         ++counter)
    {
        processPageReference(pageRefs[counter].pageNum);
    }

    // After running, sort policies by descending hit ratio
    std::ranges::sort(algorithms,
        [](const std::unique_ptr<Algorithm> &a,
           const std::unique_ptr<Algorithm> &b) {
               return a->data->getHitRatio() > b->data->getHitRatio();
           });

    // Print summary for each selected policy
    for (const auto &algoPtr : algorithms) {
        if (algoPtr->selected) {
            printSummary(*algoPtr);
        }
    }
}

void CacheReplacementSimulator::processPageReference(int pageRef) {
    lastPageRef = pageRef;

    for (auto &algoPtr : algorithms) {
        if (!algoPtr->selected) {
            continue;
        }

        // Measure time spent inside this policy's function
        auto tStart = std::chrono::high_resolution_clock::now();
        bool fault = algoPtr->algoFunc(*algoPtr->data);
        auto tEnd = std::chrono::high_resolution_clock::now();

        algoPtr->data->execTime += std::chrono::duration<double>(tEnd - tStart);

        if (fault) {
            ++(algoPtr->data->misses);
        } else {
            ++(algoPtr->data->hits);
        }

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
 *  Each function returns:
 *    - false: page hit (no page‐fault)
 *    - true:  page miss (fault, possibly eviction)
 *
 *  In all functions, `data.pageTable` is a vector<Frame> of size numFrames.
 *  Frames whose `page == -1` are considered empty. On a hit, update frame metadata.
 *  On a miss, either fill an empty frame or evict an existing one according to policy.
 * -----------------------------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 *  Optimal Replacement (Belady's MIN)
 *
 *  - Always evict the page that will not be used for the longest time in the future.
 *  - Implementation (brute‐force look‐ahead):
 *      1) If page is in pageTable → HIT; update its metadata and return false.
 *      2) If any frame.page == -1 → MISS; load page into that frame and return true.
 *      3) For each frame in pageTable:
 *           - Scan forward from (counter+1) to end of pageRefs, looking for next use.
 *           - Record the “distance” (index of next use) or INT_MAX if never used again.
 *         Choose the resident frame whose distance is largest (furthest in future)
 *         or INT_MAX if “never used again.” Evict it.
 *      4) Load new page into that victim frame; return true.
 * -----------------------------------------------------------------------------
 */
bool CacheReplacementSimulator::optimal(AlgorithmData &data) {
    // 1) Check for hit in pageTable
    auto hitIt = std::ranges::find_if(data.pageTable,
        [this](const Frame &f) {
            return f.page == lastPageRef;
        });

    if (hitIt != data.pageTable.end()) {
        // Page hit: update last-access time and logical counter
        hitIt->time  = std::chrono::steady_clock::now();
        hitIt->extra = counter;  // store the reference counter
        return false;            // no page fault
    }

    // 2) Check for an empty frame (page == -1)
    auto emptyIt = std::ranges::find_if(data.pageTable,
        [](const Frame &f) {
            return f.page == -1;
        });

    if (emptyIt != data.pageTable.end()) {
        // Use this empty slot for the new page
        emptyIt->page  = lastPageRef;
        emptyIt->time  = std::chrono::steady_clock::now();
        emptyIt->extra = counter;
        return true; // Miss, but no eviction
    }

    // 3) Must evict: find the “furthest‐in‐future” use for each currently in memory
    int maxDistance = -1;
    auto victimIt = data.pageTable.begin();

    for (auto it = data.pageTable.begin(); it != data.pageTable.end(); ++it) {
        int distance = std::numeric_limits<int>::max();
        // Scan forward in the trace to find when it->page appears next
        for (std::size_t i = static_cast<std::size_t>(counter) + 1; i < pageRefs.size(); ++i) {
            if (pageRefs[i].pageNum == it->page) {
                distance = static_cast<int>(i);
                break;
            }
        }
        // If this distance is larger than any we have seen, this frame is a better victim
        if (distance > maxDistance) {
            maxDistance = distance;
            victimIt    = it;
        }
    }

    // Evict victimIt
    data.victimList.push_back(*victimIt);
    victimIt->page  = lastPageRef;
    victimIt->time  = std::chrono::steady_clock::now();
    victimIt->extra = counter;
    return true; // Miss + eviction
}

/*
 * -----------------------------------------------------------------------------
 *  Random Replacement
 *
 *  - On a miss, if there is no empty frame, pick a frame uniformly at random to evict.
 *  - Steps:
 *      1) If the requested page is already in a frame → HIT; update metadata; return false.
 *      2) If any frame is empty (page == -1) → MISS; load page there; return true.
 *      3) Otherwise, generate a random index in [0..numFrames-1]; evict that frame; load new page; return true.
 * -----------------------------------------------------------------------------
 */
bool CacheReplacementSimulator::random(AlgorithmData &data) {
    // 1) Check for hit
    auto hitIt = std::ranges::find_if(data.pageTable,
        [this](const Frame &f) {
            return f.page == lastPageRef;
        });
    if (hitIt != data.pageTable.end()) {
        // Page hit: update metadata
        hitIt->time  = std::chrono::steady_clock::now();
        hitIt->extra = counter;
        return false; // no fault
    }

    // 2) Check for an empty slot
    auto emptyIt = std::ranges::find_if(data.pageTable,
        [](const Frame &f) {
            return f.page == -1;
        });
    if (emptyIt != data.pageTable.end()) {
        // Use this empty slot for the new page
        emptyIt->page  = lastPageRef;
        emptyIt->time  = std::chrono::steady_clock::now();
        emptyIt->extra = counter;
        return true; // Miss (no eviction)
    }

    // 3) Evict a random frame
    std::uniform_int_distribution<> dist(0, numFrames - 1);
    int victimIndex = dist(rng);
    data.victimList.push_back(data.pageTable[victimIndex]);

    // Overwrite that frame with the new page
    data.pageTable[victimIndex].page  = lastPageRef;
    data.pageTable[victimIndex].time  = std::chrono::steady_clock::now();
    data.pageTable[victimIndex].extra = counter;
    return true; // Miss + eviction
}

/*
 * -----------------------------------------------------------------------------
 *  FIFO (First‐In, First‐Out) Replacement
 *
 *  - Maintains an insertion order in `extra` (counter when the page was loaded).
 *  - On a hit, do NOT update `extra` (static insertion‐time). On a miss:
 *      1) If any frame is empty → MISS; load new page with extra = counter; return true.
 *      2) Otherwise, evict frame with smallest `extra` (oldest insertion order).
 *         Overwrite it with new page and set extra = counter.
 * -----------------------------------------------------------------------------
 */
bool CacheReplacementSimulator::fifo(AlgorithmData &data) {
    // 1) Hit? (In FIFO, hits do not affect insertion order)
    auto hitIt = std::ranges::find_if(data.pageTable,
        [this](const Frame &f) {
            return f.page == lastPageRef;
        });
    if (hitIt != data.pageTable.end()) {
        // Do nothing to `extra` or `time`; page stays in queue
        return false; // no fault
    }

    // 2) Empty frame?
    auto emptyIt = std::ranges::find_if(data.pageTable,
        [](const Frame &f) {
            return f.page == -1;
        });
    if (emptyIt != data.pageTable.end()) {
        // Load new page into empty frame
        emptyIt->page  = lastPageRef;
        emptyIt->time  = std::chrono::steady_clock::now();
        emptyIt->extra = counter; // record insertion order
        return true; // Miss (no eviction)
    }

    // 3) Evict the frame with the smallest `extra` (oldest insertion timestamp)
    auto victimIt = std::ranges::min_element(data.pageTable,
        [](const Frame &a, const Frame &b) {
            return a.extra < b.extra;
        });
    if (victimIt != data.pageTable.end()) {
        data.victimList.push_back(*victimIt);
        // Overwrite victim with new page
        victimIt->page  = lastPageRef;
        victimIt->time  = std::chrono::steady_clock::now();
        victimIt->extra = counter; // new insertion timestamp
    }
    return true; // Miss + eviction
}

/*
 * -----------------------------------------------------------------------------
 *  LRU (Least‐Recently Used) Replacement
 *
 *  - Evicts the frame whose `time` is the oldest (smallest) ⇒ least recently accessed.
 *  - On a hit: update that frame’s `time = now()` and `extra = counter` (optional).
 *  - On a miss:
 *      1) If empty frame exists → MISS; load page, set time=now, extra=counter; return true.
 *      2) Otherwise, find victim = frame with min(`time`); evict it; overwrite with new page;
 *         set new page’s `time = now()` and `extra = counter`; return true.
 * -----------------------------------------------------------------------------
 */
bool CacheReplacementSimulator::lru(AlgorithmData &data) {
    // 1) Hit? update `time` to now and `extra`
    auto hitIt = std::ranges::find_if(data.pageTable,
        [this](const Frame &f) {
            return f.page == lastPageRef;
        });
    if (hitIt != data.pageTable.end()) {
        hitIt->time  = std::chrono::steady_clock::now();
        hitIt->extra = counter;
        return false; // no fault
    }

    // 2) Empty frame?
    auto emptyIt = std::ranges::find_if(data.pageTable,
        [](const Frame &f) {
            return f.page == -1;
        });
    if (emptyIt != data.pageTable.end()) {
        emptyIt->page  = lastPageRef;
        emptyIt->time  = std::chrono::steady_clock::now();
        emptyIt->extra = counter;
        return true; // Miss (no eviction)
    }

    // 3) Evict least recently used (min `time`)
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
    return true; // Miss + eviction
}

/*
 * -----------------------------------------------------------------------------
 *  CLOCK (Second‐Chance) Replacement
 *
 *  - Uses a “reference bit” (`extra`) for each frame.
 *  - Maintains a rotating “clock hand” pointer (static int clockHand).
 *  - On a hit: set that frame's `extra = 1` (reference bit); return false.
 *  - On a miss:
 *      1) If empty frame exists → MISS; load new page, set `extra=1`; return true.
 *      2) Otherwise, starting at clockHand, do:
 *           - If `pageTable[clockHand].extra == 0` → evict this frame
 *           - Else (`extra == 1`): clear `extra = 0`, advance clockHand, continue
 *         If you loop all the way back, evict that frame anyway.
 *      3) After evicting, load new page with `extra = 1`, advance clockHand; return true.
 * -----------------------------------------------------------------------------
 */
bool CacheReplacementSimulator::clock(AlgorithmData &data) {
    static int clockHand = 0;

    // Ensure clockHand is within bounds [0..numFrames-1]
    if (clockHand >= numFrames) {
        clockHand = 0;
    }

    // 1) Check for hit: set R‐bit (extra = 1), do not evict
    auto hitIt = std::ranges::find_if(data.pageTable,
        [this](const Frame &f) {
            return f.page == lastPageRef;
        });
    if (hitIt != data.pageTable.end()) {
        hitIt->extra = 1; // Set reference bit
        return false;     // no fault
    }

    // 2) Check for empty frame
    auto emptyIt = std::ranges::find_if(data.pageTable,
        [](const Frame &f) {
            return f.page == -1;
        });
    if (emptyIt != data.pageTable.end()) {
        // Load new page here with reference bit = 1
        emptyIt->page  = lastPageRef;
        emptyIt->extra = 1;
        return true; // Miss (no eviction)
    }

    // 3) Search for first frame with R‐bit == 0, giving second chances to R==1 frames
    int startHand = clockHand;
    do {
        if (data.pageTable[clockHand].extra == 0) {
            // Found victim: evict it
            data.victimList.push_back(data.pageTable[clockHand]);
            data.pageTable[clockHand].page  = lastPageRef;
            data.pageTable[clockHand].extra = 1; // Set R‐bit for new page
            clockHand = (clockHand + 1) % numFrames;
            return true; // Miss + eviction
        } else {
            // Give second chance: clear R‐bit and advance hand
            data.pageTable[clockHand].extra = 0;
            clockHand = (clockHand + 1) % numFrames;
        }
    } while (clockHand != startHand);

    // If we made a full cycle, evict the current clockHand anyway
    data.victimList.push_back(data.pageTable[clockHand]);
    data.pageTable[clockHand].page  = lastPageRef;
    data.pageTable[clockHand].extra = 1;
    clockHand = (clockHand + 1) % numFrames;
    return true; // Miss + eviction
}

/*
 * -----------------------------------------------------------------------------
 *  NFU (Not Frequently Used) Replacement
 *
 *  - Maintains a counter (`extra`) for each frame indicating the number of hits
 *    this frame has had since it was loaded.
 *  - On a hit: increment that frame’s `extra` (usage count); return false.
 *  - On a miss:
 *      1) If empty frame exists → MISS; load new page, set `extra=0`; return true.
 *      2) Otherwise, find frame with smallest `extra` (lowest hit‐count); evict it;
 *         load new page with `extra=0`; return true.
 * -----------------------------------------------------------------------------
 */
bool CacheReplacementSimulator::nfu(AlgorithmData &data) {
    // 1) If hit, increment usage count
    auto hitIt = std::ranges::find_if(data.pageTable,
        [this](const Frame &f) {
            return f.page == lastPageRef;
        });
    if (hitIt != data.pageTable.end()) {
        hitIt->extra++;
        hitIt->time = std::chrono::steady_clock::now(); // optional timestamp
        return false; // no fault
    }

    // 2) Check for empty frame
    auto emptyIt = std::ranges::find_if(data.pageTable,
        [](const Frame &f) {
            return f.page == -1;
        });
    if (emptyIt != data.pageTable.end()) {
        emptyIt->page  = lastPageRef;
        emptyIt->time  = std::chrono::steady_clock::now();
        emptyIt->extra = 0; // initialize count = 0
        return true; // Miss (no eviction)
    }

    // 3) Evict frame with smallest extra (usage count)
    auto victimIt = std::ranges::min_element(data.pageTable,
        [](const Frame &a, const Frame &b) {
            return a.extra < b.extra;
        });
    if (victimIt != data.pageTable.end()) {
        data.victimList.push_back(*victimIt);
        victimIt->page  = lastPageRef;
        victimIt->time  = std::chrono::steady_clock::now();
        victimIt->extra = 0; // reset count for new page
    }
    return true; // Miss + eviction
}

/*
 * -----------------------------------------------------------------------------
 *  Aging (Approximate LRU) Replacement
 *
 *  - Maintains an integer register (`extra`) for each frame as an aging counter.
 *  - On every reference (hit or miss), first “age” every non‐empty frame by shifting
 *    its `extra` right by 1 (divide by 2). This decays older references.
 *  - On a hit: after aging, add a large constant (e.g. 10,000,000) to that frame’s
 *    `extra` to represent a “just used” high weight.
 *  - On a miss:
 *      1) If empty frame exists → MISS; load new page with `extra=0`; return true.
 *      2) Otherwise, evict the frame with the smallest `extra` (coldest), load new page
 *         with `extra=0`; return true.
 * -----------------------------------------------------------------------------
 */
bool CacheReplacementSimulator::aging(AlgorithmData &data) {
    // 1) Age all frames by dividing their extra by 2
    for (auto &frame : data.pageTable) {
        if (frame.page != -1) {
            frame.extra /= 2; // shift right by 1
        }
    }

    // 2) Check for hit: add a large constant to mark recent use
    auto hitIt = std::ranges::find_if(data.pageTable,
        [this](const Frame &f) {
            return f.page == lastPageRef;
        });
    if (hitIt != data.pageTable.end()) {
        hitIt->extra += 10000000; // large offset → “just used”
        hitIt->time  = std::chrono::steady_clock::now();
        return false; // no fault
    }

    // 3) Check for empty frame
    auto emptyIt = std::ranges::find_if(data.pageTable,
        [](const Frame &f) {
            return f.page == -1;
        });
    if (emptyIt != data.pageTable.end()) {
        emptyIt->page  = lastPageRef;
        emptyIt->time  = std::chrono::steady_clock::now();
        emptyIt->extra = 0;
        return true; // Miss (no eviction)
    }

    // 4) Evict frame with smallest extra (least “aged” highest value)
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
    return true; // Miss + eviction
}

/*
 * -----------------------------------------------------------------------------
 *  MRU (Most‐Recently Used) Replacement
 *
 *  - Evicts the frame that was most recently accessed (maximum `time`).
 *  - On a hit: update the frame’s `time = now()`
 *  - On a miss:
 *      1) If any frame is empty → MISS; load page with `time = now()`, return true.
 *      2) Otherwise, find frame with maximum `time` (most recently used) → evict it,
 *         load new page, set its `time = now()`, return true.
 * -----------------------------------------------------------------------------
 */
bool CacheReplacementSimulator::mru(AlgorithmData &data) {
    // 1) Hit? update `time`
    auto hitIt = std::ranges::find_if(data.pageTable,
        [this](const Frame &f) {
            return f.page == lastPageRef;
        });
    if (hitIt != data.pageTable.end()) {
        hitIt->time = std::chrono::steady_clock::now();
        return false; // no fault
    }

    // 2) Empty frame?
    auto emptyIt = std::ranges::find_if(data.pageTable,
        [](const Frame &f) {
            return f.page == -1;
        });
    if (emptyIt != data.pageTable.end()) {
        emptyIt->page = lastPageRef;
        emptyIt->time = std::chrono::steady_clock::now();
        return true; // Miss (no eviction)
    }

    // 3) Evict the most recently used (max `time`)
    auto victimIt = std::ranges::max_element(data.pageTable,
        [](const Frame &a, const Frame &b) {
            return a.time < b.time;
        });
    if (victimIt != data.pageTable.end()) {
        data.victimList.push_back(*victimIt);
        victimIt->page = lastPageRef;
        victimIt->time = std::chrono::steady_clock::now();
    }
    return true; // Miss + eviction
}

/*
 * -----------------------------------------------------------------------------
 *  NRU (Not Recently Used) Replacement
 *
 *  - In true hardware‐NRU, each frame has a “reference bit” that is periodically
 *    cleared; victim selection chooses a page with R=0. Here we approximate by
 *    evicting the “oldest access” (same as LRU).
 *  - Implementation:
 *      1) If hit → update `time = now()`, return false.
 *      2) If empty → load new page, set `time = now()`, return true.
 *      3) Else → evict frame with minimum `time` (oldest), load new page, set `time = now()`, return true.
 * -----------------------------------------------------------------------------
 */
bool CacheReplacementSimulator::nru(AlgorithmData &data) {
    // 1) Hit? update `time`
    auto hitIt = std::ranges::find_if(data.pageTable,
        [this](const Frame &f) {
            return f.page == lastPageRef;
        });
    if (hitIt != data.pageTable.end()) {
        hitIt->time = std::chrono::steady_clock::now();
        return false; // no fault
    }

    // 2) Empty frame?
    auto emptyIt = std::ranges::find_if(data.pageTable,
        [](const Frame &f) {
            return f.page == -1;
        });
    if (emptyIt != data.pageTable.end()) {
        emptyIt->page = lastPageRef;
        emptyIt->time = std::chrono::steady_clock::now();
        return true; // Miss (no eviction)
    }

    // 3) Evict the “oldest” (min `time`)
    auto victimIt = std::ranges::min_element(data.pageTable,
        [](const Frame &a, const Frame &b) {
            return a.time < b.time;
        });
    if (victimIt != data.pageTable.end()) {
        data.victimList.push_back(*victimIt);
        victimIt->page = lastPageRef;
        victimIt->time = std::chrono::steady_clock::now();
    }
    return true; // Miss + eviction
}

/*
 * -----------------------------------------------------------------------------
 *  MFU (Most‐Frequently Used) Replacement
 *
 *  - Evicts the frame with the largest usage count (`extra`), on the premise that
 *    pages that have been used frequently in the past will not be needed in the future.
 *  - On a hit: increment that frame’s `extra` (frequency count); return false.
 *  - On a miss:
 *      1) If any frame is empty → MISS; load new page, set `extra=1`; return true.
 *      2) Otherwise, evict frame with maximum `extra`, load new page with `extra=1`; return true.
 * -----------------------------------------------------------------------------
 */
bool CacheReplacementSimulator::mfu(AlgorithmData &data) {
    // 1) Hit? increment usage frequency (extra)
    auto hitIt = std::ranges::find_if(data.pageTable,
        [this](const Frame &f) {
            return f.page == lastPageRef;
        });
    if (hitIt != data.pageTable.end()) {
        hitIt->extra++; // Increase usage frequency
        return false;   // no fault
    }

    // 2) Empty frame?
    auto emptyIt = std::ranges::find_if(data.pageTable,
        [](const Frame &f) {
            return f.page == -1;
        });
    if (emptyIt != data.pageTable.end()) {
        emptyIt->page  = lastPageRef;
        emptyIt->extra = 1; // Start frequency = 1
        return true; // Miss (no eviction)
    }

    // 3) Evict the MFU (max extra)
    auto victimIt = std::ranges::max_element(data.pageTable,
        [](const Frame &a, const Frame &b) {
            return a.extra < b.extra;
        });
    if (victimIt != data.pageTable.end()) {
        data.victimList.push_back(*victimIt);
        victimIt->page  = lastPageRef;
        victimIt->extra = 1; // Reset frequency for new page
    }
    return true; // Miss + eviction
}

/*
 * -----------------------------------------------------------------------------
 *  LFU (Least‐Frequently Used) Replacement
 *
 *  - Evicts the frame with the smallest frequency count (`frequency`).
 *  - On a hit: increment that frame’s `frequency`, update `lastUsed` = getCurrentTime(); return false.
 *  - On a miss:
 *      1) If any frame is empty → MISS; load new page, set `frequency=1`, `lastUsed=now`; return true.
 *      2) Otherwise, find frame with min `(frequency, then lastUsed)` (tie‐break older lastUsed),
 *         evict it, load new page, set `frequency=1`, `lastUsed=now`; return true.
 * -----------------------------------------------------------------------------
 */
bool CacheReplacementSimulator::lfu(AlgorithmData &data) {
    // 1) Hit? update frequency and lastUsed
    auto hitIt = std::ranges::find_if(data.pageTable,
        [this](const Frame &f) {
            return f.page == lastPageRef;
        });
    if (hitIt != data.pageTable.end()) {
        hitIt->frequency++;
        hitIt->lastUsed = getCurrentTime(); // update for tie-breaks
        return false; // no fault
    }

    // 2) Empty frame?
    auto emptyIt = std::ranges::find_if(data.pageTable,
        [](const Frame &f) {
            return f.page == -1;
        });
    if (emptyIt != data.pageTable.end()) {
        emptyIt->page      = lastPageRef;
        emptyIt->frequency = 1;
        emptyIt->lastUsed  = getCurrentTime();
        return true; // Miss (no eviction)
    }

    // 3) Evict LFU (min frequency, then older lastUsed)
    auto victimIt = std::ranges::min_element(data.pageTable,
        [](const Frame &a, const Frame &b) {
            if (a.frequency != b.frequency) {
                return a.frequency < b.frequency;
            } else {
                return a.lastUsed < b.lastUsed;
            }
        });
    if (victimIt != data.pageTable.end()) {
        data.victimList.push_back(*victimIt);
        victimIt->page       = lastPageRef;
        victimIt->frequency  = 1;
        victimIt->lastUsed   = getCurrentTime();
    }
    return true; // Miss + eviction
}

/*
 * -----------------------------------------------------------------------------
 *  LFRU (Least Frequently Recently Used) Replacement
 *
 *  This hybrid uses two partitions:
 *    - privileged (size PRIVILEGED_PARTITION_SIZE) as an LRU cache
 *    - unprivileged (size UNPRIVILEGED_PARTITION_SIZE) as an LFU cache
 *
 *  On each reference:
 *    1) If the requested page ∈ privileged.partition:
 *         - HIT: update its LRU timestamp (lastUsed = getCurrentTime())
 *         - return false (no fault)
 *
 *    2) Else if requested page ∈ unprivileged.partition:
 *         - HIT: promote it into privileged:
 *             a) remove it from unprivileged (removeFromPartition)
 *             b) if privileged is full, demote the LRU from privileged:
 *                   * demoteLRU() gives the page number to unprivileged
 *                   * if unprivileged is full, evict LFU from unprivileged
 *                   * insert demoted page into unprivileged
 *             c) insert requested page into privileged (insertIntoPartition)
 *         - return false (no fault)
 *
 *    3) Else: page fault → handlePageInsertion():
 *         a) If privileged has space, insert requested page into privileged.
 *         b) Else privileged is full:
 *             i) demote LRU from privileged (demoteLRU) → get demotedPage
 *             ii) if unprivileged has no space, evict least‐frequently‐used from unprivileged
 *             iii) insert demotedPage into unprivileged
 *             iv) insert requested page into privileged
 *         return true (fault)
 *
 *  Helper functions:
 *    - updateLRU(partition, page): find frame.page == page, set lastUsed=getCurrentTime()
 *    - updateLFU(partition, page): find frame.page == page, increment frame.frequency
 *    - removeFromPartition(partition, page): find frame.page == page, reset it to empty
 *    - insertIntoPartition(partition, page): find empty frame (page==−1), set
 *          frame.page=page, frame.lastUsed/getCurrentTime(), frame.frequency=1
 *    - evictLFU(partition): find frame with min `(frequency, lastUsed)`, record victim,
 *          set that frame empty, return the evicted page number
 *    - demoteLRU(partition): find frame with min `lastUsed` (LRU), record victim,
 *          set that frame empty, return the demoted page number
 * -----------------------------------------------------------------------------
 */
bool CacheReplacementSimulator::lfru(AlgorithmData &data) {
    // Ensure LFRUData is allocated
    if (!data.lfruData) {
        data.lfruData = std::make_unique<LFRUData>();
    }
    LFRUData &lf = *data.lfruData;

    // 1) Page is in privileged partition (LRU)
    if (lf.privileged.hasPage(lastPageRef)) {
        // HIT: update its LRU timestamp
        updateLRU(lf.privileged, lastPageRef);
        return false; // no fault
    }

    // 2) Page is in unprivileged partition (LFU)
    if (lf.unprivileged.hasPage(lastPageRef)) {
        // HIT: promote from unprivileged → privileged
        removeFromPartition(lf.unprivileged, lastPageRef);

        // If privileged is full, demote its LRU
        if (!lf.privileged.hasSpace()) {
            int demotedPage = demoteLRU(lf.privileged);
            // Ensure unprivileged has space to receive demotedPage
            if (!lf.unprivileged.hasSpace()) {
                evictLFU(lf.unprivileged);
            }
            insertIntoPartition(lf.unprivileged, demotedPage);
        }

        // Insert requested page into privileged
        insertIntoPartition(lf.privileged, lastPageRef);
        return false; // no fault
    }

    // 3) Page fault: must insert new page
    handlePageInsertion(lf, lastPageRef);
    return true; // fault
}

/*
 * -----------------------------------------------------------------------------
 *  LFRU Helper Functions
 * -----------------------------------------------------------------------------
 */

// updateLRU: find `page` in the given partition, update its `lastUsed`
void CacheReplacementSimulator::updateLRU(Partition &partition, int page) {
    auto it = std::ranges::find_if(partition.frames,
        [page](const Frame &f) {
            return f.page == page;
        });
    if (it != partition.frames.end()) {
        it->lastUsed = getCurrentTime();
    }
}

// updateLFU: find `page` in the given partition, increment its `frequency`
void CacheReplacementSimulator::updateLFU(Partition &partition, int page) {
    auto it = std::ranges::find_if(partition.frames,
        [page](const Frame &f) {
            return f.page == page;
        });
    if (it != partition.frames.end()) {
        it->frequency++;
    }
}

// removeFromPartition: find frame.page == page, reset it to empty state
void CacheReplacementSimulator::removeFromPartition(Partition &partition, int page) {
    auto it = std::ranges::find_if(partition.frames,
        [page](const Frame &f) {
            return f.page == page;
        });
    if (it != partition.frames.end()) {
        it->page      = -1;
        it->frequency = 0;
        it->lastUsed  = 0;
        it->extra     = 0;
        it->time      = std::chrono::steady_clock::now();
    }
}

// handlePageInsertion: on a page fault, insert into privileged, demoting/evicting as needed
void CacheReplacementSimulator::handlePageInsertion(LFRUData &lf, int page) {
    if (lf.privileged.hasSpace()) {
        // If there is space in privileged, just insert there
        insertIntoPartition(lf.privileged, page);
    } else {
        // Privileged is full → demote least‐recently used (LRU) from privileged
        int demotedPage = demoteLRU(lf.privileged);
        // Ensure unprivileged has room for demotedPage
        if (!lf.unprivileged.hasSpace()) {
            evictLFU(lf.unprivileged);
        }
        insertIntoPartition(lf.unprivileged, demotedPage);
        // Now insert new page into privileged
        insertIntoPartition(lf.privileged, page);
    }
}

// insertIntoPartition: find an empty frame (page == -1) and fill it
void CacheReplacementSimulator::insertIntoPartition(Partition &partition, int page) {
    auto it = std::ranges::find_if(partition.frames,
        [](const Frame &f) {
            return f.page == -1;
        });
    if (it != partition.frames.end()) {
        it->page      = page;
        it->lastUsed  = getCurrentTime(); // for LRU/frequency tie-breakers
        it->frequency = 1;                // start LFU count at 1
    }
}

// evictLFU: find the frame with minimal (frequency, then lastUsed) → evict it, return page number
int CacheReplacementSimulator::evictLFU(Partition &partition) {
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

// demoteLRU: find the frame with minimal lastUsed (LRU), evict it, return page number
int CacheReplacementSimulator::demoteLRU(Partition &partition) {
    auto victimIt = std::ranges::min_element(partition.frames,
        [](const Frame &a, const Frame &b) {
            return a.lastUsed < b.lastUsed;
        });
    if (victimIt != partition.frames.end()) {
        int demotedPage = victimIt->page;
        victimIt->page      = -1;
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
 *  Configuration and Utility Functions
 * -----------------------------------------------------------------------------
 */

void CacheReplacementSimulator::setConfiguration(int frames,
                                                 int maxCalls,
                                                 bool debugMode,
                                                 bool printRefsMode)
{
    numFrames     = std::max(1, frames);
    maxPageCalls  = maxCalls;
    debug         = debugMode;
    printRefs     = printRefsMode;

    // Re‐initialize all policies with the new number of frames
    initializeAlgorithms();
}

void CacheReplacementSimulator::printHelp(const std::string &programName) {
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

/*
 * -----------------------------------------------------------------------------
 *  Printing / Reporting Functions
 * -----------------------------------------------------------------------------
 *
 *  printSummary: prints a one-line summary for the given policy:
 *      <Label> Algorithm
 *      Frames in Mem: <numFrames>, Hits: <hits>, Misses: <misses>, 
 *      Hit Ratio: <hitRatio>, Total Execution Time: <execTime> seconds
 *
 *  printStats: calls printSummary, then printPageTable to dump the entire page table.
 *
 *  printPageTable: prints a four‐row table with columns equal to number of frames:
 *     - Row 1: “Frame # :” followed by each frame’s index
 *     - Row 2: “Page Ref :” followed by each frame’s current page (or “_” if empty)
 *     - Row 3: “Extra    :” followed by each frame’s `extra` field (R‐bit/NFU/aging, etc.)
 *     - Row 4: “Time     :” followed by each frame’s timestamp modulo a large constant
 * -----------------------------------------------------------------------------
 */

void CacheReplacementSimulator::printSummary(const Algorithm &algo) const {
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

void CacheReplacementSimulator::printStats(const Algorithm &algo) const {
    printSummary(algo);
    printPageTable(algo.data->pageTable);
}

void CacheReplacementSimulator::printPageTable(const std::vector<Frame> &pageTable) const {
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

    // Row 3: Extra (policy‐specific field: reference bit, NFU count, aging value, etc.)
    oss << std::setw(labelSize) << std::left << "Extra" << " : ";
    for (const auto &frame : pageTable) {
        oss << std::setw(colSize) << frame.extra;
    }
    oss << "\n";

    // Row 4: Time (millisecond timestamp modded for display)
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

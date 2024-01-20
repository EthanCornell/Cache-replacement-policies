/*
 * Copyright (c) Cornell University.
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: I-Hsuan (Ethan) Huang
 * Email: ih246@cornell.edu
 * Project: Cache Replacement Algorithms
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <time.h>
#include <sys/queue.h>
#include "CacheReplacementAlgorithm.h"


//Configuration variables

#define PRIVILEGED_PARTITION_SIZE 5
#define UNPRIVILEGED_PARTITION_SIZE 5

int num_frames = 10;           // Number of avaliable pages in page tables
int page_ref_upper_bound = 32; // Largest page reference
int max_page_calls = 24;     // Max number of page refs to test

int debug = 0;                 // Debug bool, 1 shows verbose output
int printrefs = 0;             // Print refs bool, 1 shows output after each page ref


//Array of algorithm functions that can be enabled
Algorithm algos[11] = { {"OPTIMAL", &OPTIMAL, 0, NULL},
                       {"RANDOM", &RANDOM, 0, NULL},
                       {"FIFO", &FIFO, 0, NULL},
                       {"LRU", &LRU, 0, NULL},
                       {"CLOCK", &CLOCK, 0, NULL},
                       {"NFU", &NFU, 0, NULL},
                       {"AGING", &AGING, 0, NULL},
                       {"MRU", &MRU, 0, NULL},
                       {"NRU", &NRU, 0, NULL},
                       {"MFU", &MFU, 0, NULL},
                       {"LFRU", &LFRU, 0, NULL} };
//LFRU section
typedef struct {
    Frame *frames; // Array of frames
    int size;      // Size of the partition
} Partition;

typedef struct {
    Partition privileged;
    Partition unprivileged;
} LFRU_Data;

// Runtime variables
int counter = 0;           // "Time" as number of loops calling page_refs 0...num_refs (used as i in for loop)
int last_page_ref = -1;    // Last ref
size_t num_algos = 0;      // Number of algorithms in algos, calculated in init()
int *optimum_find_test;
int num_refs = 0;          // Number of page refs in page_refs list


// Run algorithm if given correct arguments, else terminate with error
int main(int argc, char *argv[]) {

    if (argc < 5 || argc > 6) {
        print_help(argv[0]);
        return 1;
    }

    const char* filename = argv[1];
    const char* algorithm = argv[2];
    num_frames = atoi(argv[3]);
    printrefs = (argc > 4) ? atoi(argv[4]) : 0;
    debug = (argc > 5) ? atoi(argv[5]) : 0;

    printf("Attempting to open file: %s\n", filename);

    if (num_frames < 1) {
        num_frames = 1;
        printf("Number of page frames must be at least 1, setting to 1\n");
    }

    // Calculate number of algos
    num_algos = sizeof(algos) / sizeof(Algorithm);

    char algo_code = algorithm[0]; // Use the first character of the algorithm string
    switch (algo_code) {
        case 'O': // OPTIMAL
            algos[0].selected = 1;
            break;
        case 'R': // RANDOM
            algos[1].selected = 1;
            break;
        case 'F': // FIFO
            algos[2].selected = 1;
            break;
        case 'L': // LRU
            algos[3].selected = 1;
            break;
        case 'C': // CLOCK
            algos[4].selected = 1;
            break;
        case 'N': // NFU
            algos[5].selected = 1;
            break;
        case 'A': // AGING
            algos[6].selected = 1;
            break;
        case 'M': //MRU
            algos[7].selected = 1;
            break;   
        case 'n': //NRU
            algos[8].selected = 1;
            break; 
        case 'm': //MFU
            algos[9].selected = 1;
            break;     
        case 'l': //LFRU
            algos[10].selected = 1;
            break;    
        case 'a': // ALL
            for (size_t i = 0; i < num_algos; i++) {
                algos[i].selected = 1;
            }
            break;
        default:
            printf("Invalid algorithm choice\n");
            print_help(argv[0]);
            return 1;
    }

    // Initialize and generate page references
    init(filename);

    // Run the event loop
    event_loop();

    // Clean up and exit
    cleanup();
    return 0;
}

int init(const char *filename)
{
        gen_page_refs(filename);
        // Calculate number of algos
        num_algos = sizeof(algos)/sizeof(Algorithm);
        size_t i = 0;
        for (i = 0; i < num_algos; ++i)
        {
                algos[i].data = create_algo_data_store();
        }
        return 0;
}


// Generate all page refs to use in tests
void gen_page_refs(const char *filename) {
    printf("Opening file: %s\n", filename);  // Add this line for debugging
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        exit(1);  // Changed from exit(1) to allow graceful termination.
    }

    int pid, page;
    LIST_INIT(&page_refs);
    while (fscanf(file, "%d %d", &pid, &page) == 2) {
        Page_Ref *new_ref = malloc(sizeof(Page_Ref));
        if (!new_ref) {
            fprintf(stderr, "Memory allocation failed\n");
            fclose(file);
            return;
        }
        new_ref->page_num = page;
        // Uncomment next line if Page_Ref struct has pid field
        new_ref->pid = pid; 
        LIST_INSERT_HEAD(&page_refs, new_ref, pages);
        num_refs++;
    }

    fclose(file);
}

// Generate a random page ref within bounds
Page_Ref* gen_ref()
{
        Page_Ref *page = malloc(sizeof(Page_Ref));
        page->page_num = rand() % page_ref_upper_bound;
        return page;
}



// Function to initialize and allocate LFRU partitions
void initializeLFRUPartitions(Algorithm_Data *data, int privilegedSize, int unprivilegedSize) {
    LFRU_Data *lfru_data = malloc(sizeof(LFRU_Data));
    if (!lfru_data) {
        // Handle allocation failure
        fprintf(stderr, "Failed to allocate memory for LFRU data\n");
        return;
    }

    // Allocate and initialize privileged partition
    lfru_data->privileged.frames = calloc(privilegedSize, sizeof(Frame));
    if (!lfru_data->privileged.frames) {
        // Handle allocation failure
        fprintf(stderr, "Failed to allocate memory for privileged partition\n");
        free(lfru_data);
        return;
    }
    lfru_data->privileged.size = privilegedSize;
    for (int i = 0; i < privilegedSize; ++i) {
        lfru_data->privileged.frames[i].page = -1;
        lfru_data->privileged.frames[i].frequency = 0;
        lfru_data->privileged.frames[i].lastUsed = 0;
    }

    // Allocate and initialize unprivileged partition
    lfru_data->unprivileged.frames = calloc(unprivilegedSize, sizeof(Frame));
    if (!lfru_data->unprivileged.frames) {
        // Handle allocation failure
        fprintf(stderr, "Failed to allocate memory for unprivileged partition\n");
        free(lfru_data->privileged.frames);
        free(lfru_data);
        return;
    }
    lfru_data->unprivileged.size = unprivilegedSize;
    for (int i = 0; i < unprivilegedSize; ++i) {
        lfru_data->unprivileged.frames[i].page = -1;
        lfru_data->unprivileged.frames[i].frequency = 0;
        lfru_data->unprivileged.frames[i].lastUsed = 0;
    }

    // Assign the initialized LFRU data to the extra field of Algorithm_Data
    data->extra = lfru_data;
}



//Creates an empty Algorithm_Data to init an Algorithm
Algorithm_Data *create_algo_data_store()
{
        Algorithm_Data *data = malloc(sizeof(Algorithm_Data));
        data->hits = 0;
        data->misses = 0;
        data->last_victim = NULL;
        /* Initialize Lists */
        LIST_INIT(&(data->page_table));
        LIST_INIT(&(data->victim_list));
        /* Insert at the page_table. */
        Frame *framep = create_empty_frame(0);
        LIST_INSERT_HEAD(&(data->page_table), framep, frames);
        /* Build the rest of the list. */
        size_t i = 0;
        for (i = 1; i < num_frames; ++i)
        {
                LIST_INSERT_AFTER(framep, create_empty_frame(i), frames);
                framep = framep->frames.le_next;
        }


        // Specific initialization for LFRU
        if (algos[10].selected == 1) {
                initializeLFRUPartitions(data, PRIVILEGED_PARTITION_SIZE, UNPRIVILEGED_PARTITION_SIZE);
        }
        return data;
}




// Creates an empty Frame for page table list
Frame* create_empty_frame(int index)
{
        Frame *framep = malloc(sizeof(Frame));
        framep->index = index;
        framep->page = -1;
        time(&framep->time);
        framep->extra = 0;
        return framep;
}



// Comparator function for sorting algorithms by hit ratio
int compare_hit_ratio(const void *a, const void *b) {
    const Algorithm *algoA = (const Algorithm *)a;
    const Algorithm *algoB = (const Algorithm *)b;
    double hitRatioA = (double)algoA->data->hits / (double)(algoA->data->hits + algoA->data->misses);
    double hitRatioB = (double)algoB->data->hits / (double)(algoB->data->hits + algoB->data->misses);

    // For descending order
    return (hitRatioB > hitRatioA) - (hitRatioB < hitRatioA);
}

// page all selected algorithms with input ref
int event_loop()
{
        counter = 0;
        while(counter < max_page_calls)
        {
                page(get_ref());
                ++counter;
        }


        // Sort algorithms by hit ratio in descending order
        qsort(algos, num_algos, sizeof(Algorithm), compare_hit_ratio);

        // Print the sorted results
        for (size_t i = 0; i < num_algos; i++) {
                if (algos[i].selected == 1) {
                        print_summary(algos[i]);
                }
        }
        return 0;
}


// get a random ref
int get_ref()
{
        if (page_refs.lh_first != NULL)
        { // pop Page_Ref off page_refs
                int page_num = page_refs.lh_first->page_num;
                LIST_REMOVE(page_refs.lh_first, pages);
                return page_num;
        }
        else
        { // just in case
                return rand() % page_ref_upper_bound;
        }
}


// page all selected algorithms with input ref
int page(int page_ref)
{
        last_page_ref = page_ref;
        size_t i = 0;
        for (i = 0; i < num_algos; i++)
        {
                if(algos[i].selected==1) {
                        algos[i].algo(algos[i].data);
                        if(printrefs == 1)
                        {
                                print_stats(algos[i]);
                        }
                }
        }

        return 0;
}


// Add victim frame evicted from page table to list of victims
int add_victim(struct Frame_List *victim_list, struct Frame *frame)
{
        if(debug)
                printf("Victim index: %d, Page: %d\n", frame->index, frame->page);
        struct Frame *victim = malloc(sizeof(Frame));
        *victim = *frame;
        victim->index = 1;
        LIST_INSERT_HEAD(victim_list, victim, frames);
        return 0;
}


// OPTIMAL Page Replacement Algorithm
int OPTIMAL(Algorithm_Data *data) {
    Frame *framep = data->page_table.lh_first, *victim = NULL;
    int fault = 0;

    while (framep != NULL && framep->page > -1 && framep->page != last_page_ref) {
        framep = framep->frames.le_next;
    }

    if (framep == NULL) {
        size_t i, j;
        int *optimum_find_test = malloc(sizeof(int) * page_ref_upper_bound);
        if (optimum_find_test == NULL) {
            fprintf(stderr, "Memory allocation failed for optimum_find_test\n");
            return -1; // or handle the error as suitable
        }
        
        for (i = 0; i < page_ref_upper_bound; ++i) {
            optimum_find_test[i] = -1;
        }

        Page_Ref *page = page_refs.lh_first;
        int all_found = 0;
        j = 0;

        while (page != NULL && !all_found) {
            if (page->page_num < page_ref_upper_bound) {
                if (optimum_find_test[page->page_num] == -1) {
                    optimum_find_test[page->page_num] = j++;
                }
            }
            page = page->pages.le_next;

            all_found = 1;
            for (i = 0; i < page_ref_upper_bound; ++i) {
                if (optimum_find_test[i] == -1) {
                    all_found = 0;
                    break;
                }
            }
        }

        framep = data->page_table.lh_first;
        while (framep != NULL) {
            if (victim == NULL || (framep->page < page_ref_upper_bound && optimum_find_test[framep->page] > optimum_find_test[victim->page])) {
                victim = framep;
            }
            framep = framep->frames.le_next;
        }

        if (victim != NULL) {
            if (debug) printf("Victim selected: %d, Page: %d\n", victim->index, victim->page);
            add_victim(&data->victim_list, victim);
            victim->page = last_page_ref;
            time(&victim->time);
            victim->extra = counter;
            fault = 1;
        }

        free(optimum_find_test);
    }else if(framep->page == -1)
        { // Use free page table index
                framep->page = last_page_ref;
                time(&framep->time);
                framep->extra = counter;
                fault = 1;
        }
        else if(framep->page == last_page_ref)
        { // The page was found! Hit!
                time(&framep->time);
                framep->extra = counter;
        }
        if(debug)
        {
                printf("Page Ref: %d\n", last_page_ref);
                for (framep = data->page_table.lh_first; framep != NULL; framep = framep->frames.le_next)
                        printf("Slot: %d, Page: %d, Time used: %d\n", framep->index, framep->page, framep->extra);
        }
    if(fault == 1) data->misses++; else data->hits++;

    

    return fault;
}


//int RANDOM(Algorithm_Data *data)
int RANDOM(Algorithm_Data *data)
{
        struct Frame *framep = data->page_table.lh_first,
                     *victim = NULL;
        int rand_victim = rand() % num_frames;
        int fault = 0;
        /* Find target (hit), empty page index (miss), or victim to evict (miss) */
        while (framep != NULL && framep->page > -1 && framep->page != last_page_ref) {
                if(framep->index == rand_victim) // rand
                        victim = framep;
                framep = framep->frames.le_next;
        }
        if(framep == NULL)
        { // It's a miss, kill our victim
                if(debug) printf("Victim selected: %d, Page: %d\n", victim->index, victim->page);
                add_victim(&data->victim_list, victim);
                victim->page = last_page_ref;
                time(&victim->time);
                victim->extra = counter;
                fault = 1;
        }
        else if(framep->page == -1)
        { // Use free page table index
                framep->page = last_page_ref;
                time(&framep->time);
                framep->extra = counter;
                fault = 1;
        }
        else if(framep->page == last_page_ref)
        { // The page was found! Hit!
                time(&framep->time);
                framep->extra = counter;
        }
        if(debug)
        {
                printf("Page Ref: %d\n", last_page_ref);
                for (framep = data->page_table.lh_first; framep != NULL; framep = framep->frames.le_next)
                        printf("Slot: %d, Page: %d, Time used: %d\n", framep->index, framep->page, framep->extra);
        }
        if(fault == 1) data->misses++; else data->hits++;
        return fault;
}


//FIFO Page Replacement Algorithm
int FIFO(Algorithm_Data *data)
{
        struct Frame *framep = data->page_table.lh_first,
                     *victim = NULL;
        int fault = 0;
        /* Find target (hit), empty page index (miss), or victim to evict (miss) */
        while (framep != NULL && framep->page > -1 && framep->page != last_page_ref) {
                if(victim == NULL || framep->time > victim->time)
                { // No victim yet or frame older than victim
                        victim = framep;
                }
                framep = framep->frames.le_next;
        }
        /* Make a decision */
        if(framep == NULL)
        { // It's a miss, kill our victim
                if(debug) printf("Victim selected: %d, Page: %d\n", victim->index, victim->page);
                add_victim(&data->victim_list, victim);
                victim->page = last_page_ref;
                time(&victim->time);
                victim->extra = counter;
                fault = 1;
        }
        else if(framep->page == -1)
        { // Can use free page table index
                framep->page = last_page_ref;
                time(&framep->time);
                framep->extra = counter;
                fault = 1;
        }
        else if(framep->page == last_page_ref)
        { // The page was found! Hit!
                time(&framep->time);
                framep->extra = counter;
        }
        if(fault == 1) data->misses++; else data->hits++;
        return fault;
}



// LRU Page Replacement Algorithm
int LRU(Algorithm_Data *data)
{
        struct Frame *framep = data->page_table.lh_first,
                     *victim = NULL;
        int fault = 0;
        /* Find target (hit), empty page index (miss), or victim to evict (miss) */
        while (framep != NULL && framep->page > -1 && framep->page != last_page_ref) {
                if(victim == NULL || framep->time < victim->time)
                        victim = framep; // No victim yet or frame older than victim
                framep = framep->frames.le_next;
        }
        /* Make a decision */
        if(framep == NULL)
        { // It's a miss, kill our victim
                if(debug) printf("Victim selected: %d, Page: %d\n", victim->index, victim->page);
                add_victim(&data->victim_list, victim);
                victim->page = last_page_ref;
                time(&victim->time);
                victim->extra = counter;
                fault = 1;
        }
        else if(framep->page == -1)
        { // Can use free page table index
                framep->page = last_page_ref;
                time(&framep->time);
                framep->extra = counter;
                fault = 1;
        }
        else if(framep->page == last_page_ref)
        { // The page was found! Hit!
                time(&framep->time);
                framep->extra = counter;
        }
        if(fault == 1) data->misses++; else data->hits++;
        return fault;
}


// CLOCK Page Replacement Algorithm
int CLOCK(Algorithm_Data *data)
{
        static Frame *clock_hand = NULL; // Clock needs a hand
        Frame *framep = data->page_table.lh_first;
        int fault = 0;
        /* Forward traversal. */
        /* Find target (hit), empty page slot (miss), or victim to evict (miss) */
        while(framep != NULL && framep->page > -1 && framep->page != last_page_ref)
                framep = framep->frames.le_next;
        /* Make a decision */
        if(framep != NULL)
        {
                if(framep->page == -1)
                {
                        framep->page = last_page_ref;
                        framep->extra = 0;
                        fault = 1;
                }
                else
                { // Found the page, update its R bit to 0
                        framep->extra = 0;
                }
        }
        else // Use the hand to find our victim
        {
                while(clock_hand == NULL || clock_hand->extra == 0)
                {
                        if(clock_hand == NULL)
                        {
                                clock_hand = data->page_table.lh_first;
                        }
                        else
                        {
                                clock_hand->extra = 1;
                                clock_hand = clock_hand->frames.le_next;
                        }
                }
                add_victim(&data->victim_list, clock_hand);
                clock_hand->page = last_page_ref;
                clock_hand->extra = 0;
                fault = 1;
        }
        if(fault == 1) data->misses++; else data->hits++;
        return fault;
}


// NFU Page Replacement Algorithm
int NFU(Algorithm_Data *data)
{
        struct Frame *framep = data->page_table.lh_first,
                     *victim = NULL;
        int fault = 0;
        /* Find target (hit), empty page index (miss), or victim to evict (miss) */
        while (framep != NULL && framep->page > -1 && framep->page != last_page_ref) {
                if(victim == NULL || framep->extra < victim->extra)
                        victim = framep; // No victim or frame used fewer times
                framep = framep->frames.le_next;
        }
        /* Make a decision */
        if(framep == NULL)
        { // It's a miss, kill our victim
                add_victim(&data->victim_list, victim);
                victim->page = last_page_ref;
                time(&victim->time);
                victim->extra = 0;
                fault = 1;
        }
        else if(framep->page == -1)
        { // Can use free page table index
                framep->page = last_page_ref;
                time(&framep->time);
                framep->extra = 0;
                fault = 1;
        }
        else if(framep->page == last_page_ref)
        { // The page was found! Hit!
                time(&framep->time);
                framep->extra++;
        }
        if(fault == 1) data->misses++; else data->hits++;
        return fault;
}




// AGING Page Replacement Algorithm
int AGING(Algorithm_Data *data)
{
        struct Frame *framep = data->page_table.lh_first,
                     *victim = NULL;
        int fault = 0;
        /* Find target (hit), empty page index (miss), or victim to evict (miss) */
        while (framep != NULL && framep->page > -1 && framep->page != last_page_ref) {
                framep->extra /= 2;
                if(victim == NULL || framep->extra < victim->extra)
                        victim = framep; // No victim or frame used rel less
                framep = framep->frames.le_next;
        }
        /* Make a decision */
        if(framep == NULL)
        { // It's a miss, kill our victim
                add_victim(&data->victim_list, victim);
                victim->page = last_page_ref;
                time(&victim->time);
                victim->extra = 0;
                fault = 1;
        }
        else if(framep->page == -1)
        { // Can use free page table index
                framep->page = last_page_ref;
                time(&framep->time);
                framep->extra = 0;
                fault = 1;
        }
        else if(framep->page == last_page_ref)
        { // The page was found! Hit!
                time(&framep->time);
                framep->extra = framep->extra+10000000;
                while (framep->frames.le_next != NULL) {
                        framep = framep->frames.le_next;
                        framep->extra /= 2;
                }
        }
        if(fault == 1) data->misses++; else data->hits++;
        return fault;
}


// MRU(Most-recently-used) Page Replacement Algorithm
int MRU(Algorithm_Data *data) {
    Frame *framep = data->page_table.lh_first, *mru_frame = NULL, *freeFrame = NULL;
    int fault = 0;

    // Iterate through the frames to find the MRU frame and check for page hits
    while (framep != NULL) {
        if (framep->page == last_page_ref) {
            // Page hit, update its time
            time(&framep->time);
            data->hits++;
            return 0; // No fault occurred
        }

        if (framep->page == -1 && freeFrame == NULL) {
            freeFrame = framep; // Found a free frame
        } else if (mru_frame == NULL || framep->time > mru_frame->time) {
            mru_frame = framep; // Update MRU frame
        }

        framep = framep->frames.le_next;
    }

    // Page fault occurred, no match found in page table
    data->misses++;
    fault = 1;

    // Use a free frame if available, otherwise replace the MRU frame
    if (freeFrame != NULL) {
        freeFrame->page = last_page_ref; // Use the free frame
        time(&freeFrame->time); // Set the time for the new frame
    } else if (mru_frame != NULL) {
        add_victim(&data->victim_list, mru_frame); // Add the MRU frame to the victim list
        mru_frame->page = last_page_ref; // Replace with the new page
        time(&mru_frame->time); // Update the time for the MRU frame
    }

    return fault;
}



// NRU(Not Recently Used) Page Replacement Algorithm
int NRU(Algorithm_Data *data) {
    Frame *framep = data->page_table.lh_first, *nru_frame = NULL, *freeFrame = NULL;
    int fault = 0;

    // Iterate through the frames to find the NRU frame and check for page hits
    while (framep != NULL) {
        if (framep->page == last_page_ref) {
            // Page hit, update its time
            time(&framep->time);
            data->hits++;
            return 0; // No fault occurred
        }

        if (framep->page == -1 && freeFrame == NULL) {
            freeFrame = framep; // Found a free frame
        } else if (nru_frame == NULL || framep->time < nru_frame->time) {
            nru_frame = framep; // Update NRU frame
        }

        framep = framep->frames.le_next;
    }

    // Page fault occurred, no match found in page table
    data->misses++;
    fault = 1;

    // Use a free frame if available, otherwise replace the NRU frame
    if (freeFrame != NULL) {
        freeFrame->page = last_page_ref; // Use the free frame
        time(&freeFrame->time); // Set the time for the new frame
    } else if (nru_frame != NULL) {
        add_victim(&data->victim_list, nru_frame); // Add the NRU frame to the victim list
        nru_frame->page = last_page_ref; // Replace with the new page
        time(&nru_frame->time); // Update the time for the NRU frame
    }

    return fault;
}


//MFU(Most Frequently Used) Page Replacement Algorithm
int MFU(Algorithm_Data *data) {
    Frame *framep = data->page_table.lh_first, *victim = NULL, *freeFrame = NULL;
    int fault = 0;

    // Iterate through the page table to find either a hit, a free frame, or a potential victim.
    while (framep != NULL) {
        if (framep->page == last_page_ref) {
            // Page hit
            framep->extra++; // Increment the usage count
            data->hits++;
            return 0; // No fault occurred
        }

        if (framep->page == -1 && freeFrame == NULL) {
            freeFrame = framep; // Found a free frame
        } else if (victim == NULL || framep->extra > victim->extra) {
            // Select the most frequently used page as the victim
            victim = framep;
        }
        
        framep = framep->frames.le_next;
    }

    // Page fault occurred, no match found in page table
    data->misses++;
    fault = 1;

    // Use a free frame if available, otherwise replace the victim
    if (freeFrame != NULL) {
        freeFrame->page = last_page_ref; // Use the free frame
        freeFrame->extra = 1; // Initialize the usage count for the new page
    } else if (victim != NULL) {
        add_victim(&data->victim_list, victim); // Add the victim to the victim list
        victim->page = last_page_ref; // Replace with the new page
        victim->extra = 1; // Reset the usage count for the new page
    }

    return fault;
}



//LFRU section
int currentTime() {
    static int time = 0;
    return time++;
}


// Check if a page is in the given partition
int isPageInPartition(Partition *partition, int page) {
    for (int i = 0; i < partition->size; i++) {
        if (partition->frames[i].page == page) {
            return 1;
        }
    }
    return 0;
}

// Update LRU information in a partition
void updateLRU(Partition *partition, int page) {
    // Find the frame and update its last used time
    for (int i = 0; i < partition->size; i++) {
        if (partition->frames[i].page == page) {
            partition->frames[i].lastUsed = currentTime(); // currentTime() is a function to get current time
            break;
        }
    }
}

// Update LFU information in a partition
void updateLFU(Partition *partition, int page) {
    // Find the frame and increment its frequency count
    for (int i = 0; i < partition->size; i++) {
        if (partition->frames[i].page == page) {
            partition->frames[i].frequency++;
            break;
        }
    }
}

// Function to check if a page is in either partition
int checkPageInPartitions(LFRU_Data *lfru_data, int page) {
    // Check in privileged partition using LRU logic
    if (isPageInPartition(&lfru_data->privileged, page)) {
        // Update LRU information for the page
        updateLRU(&lfru_data->privileged, page);
        return 1; // Page found in privileged partition
    }

    // Check in unprivileged partition using LFU logic
    if (isPageInPartition(&lfru_data->unprivileged, page)) {
        // Update LFU information for the page
        updateLFU(&lfru_data->unprivileged, page);
        return 1; // Page found in unprivileged partition
    }

    return 0; // Page not found in either partition
}

// Function to update access information for a page
void updateAccessInfo(LFRU_Data *lfru_data, int page) {
    // Update LRU information in the privileged partition
    updateLRU(&lfru_data->privileged, page);

    // Update LFU information in the unprivileged partition
    updateLFU(&lfru_data->unprivileged, page);
}


// Evict a page using the LFU policy
int evictLFU(Partition *partition) {
    int leastFrequent = INT_MAX;
    int idxToEvict = -1;

    for (int i = 0; i < partition->size; i++) {
        if (partition->frames[i].frequency < leastFrequent) {
            leastFrequent = partition->frames[i].frequency;
            idxToEvict = i;
        }
    }

    if (idxToEvict != -1) {
        int evictedPage = partition->frames[idxToEvict].page;
        partition->frames[idxToEvict].page = -1; // Mark as empty
        return evictedPage;
    }

    return -1; // Indicates no page was evicted, likely an error
}

// Demote the most recently used page from the LRU partition
int demoteLRU(Partition *partition) {
    int mostRecent = 0;
    int idxToDemote = -1;

    for (int i = 0; i < partition->size; i++) {
        if (partition->frames[i].lastUsed > mostRecent) {
            mostRecent = partition->frames[i].lastUsed;
            idxToDemote = i;
        }
    }

    if (idxToDemote != -1) {
        int demotedPage = partition->frames[idxToDemote].page;
        partition->frames[idxToDemote].page = -1; // Mark as empty
        return demotedPage;
    }

    return -1; // Indicates no page was demoted, likely an error
}

// Insert a page into the partition
void insertIntoPartition(Partition *partition, int page) {
    // Find an empty frame or replace based on the policy
    for (int i = 0; i < partition->size; i++) {
        if (partition->frames[i].page == -1) {
            partition->frames[i].page = page;
            partition->frames[i].lastUsed = currentTime();
            partition->frames[i].frequency = 1;
            break;
        }
    }
}


// Check if there is space in the partition
int hasSpace(Partition *partition) {
    for (int i = 0; i < partition->size; i++) {
        if (partition->frames[i].page == -1) { // Assuming -1 indicates an empty frame
            return 1;
        }
    }
    return 0;
}

// Function to handle the insertion of a new page
void handlePageInsertion(LFRU_Data *lfru_data, int page) {
    // Check if there's space in the privileged partition
    if (hasSpace(&lfru_data->privileged)) {
        insertIntoPartition(&lfru_data->privileged, page);
    } else {
        // Evict a page from the unprivileged partition using LFU
        int evictedPage = evictLFU(&lfru_data->unprivileged);

        // Move a page from privileged to unprivileged
        int demotedPage = demoteLRU(&lfru_data->privileged);
        insertIntoPartition(&lfru_data->unprivileged, demotedPage);

        // Insert the new page into the privileged partition
        insertIntoPartition(&lfru_data->privileged, page);
    }
}





// LFRU (Least Frequently Recently Used) algorithm
int LFRU(Algorithm_Data *data) {
    LFRU_Data *lfru_data = (LFRU_Data *)data->extra; // Assuming extra is used to store LFRU specific data

    // Check for the page in both partitions
    int pageFound = checkPageInPartitions(lfru_data, last_page_ref);
    if (pageFound) {
        // Page found, update access info
        updateAccessInfo(lfru_data, last_page_ref);
        data->hits++;
        return 0; // No page fault
    }

    // Page fault handling
    data->misses++;

    // Evict a page if necessary and insert the new page
    handlePageInsertion(lfru_data, last_page_ref);

    return 1; // Page fault occurred
}



// Function to print results after algo is run
int print_help(const char *binary)
{
        printf( "usage: %s algorithm num_frames show_process debug\n", binary);
        printf( "   input file    - input test file\n");
        printf( "   algorithm    - page algorithm to use {LRU, CLOCK, AGING, OPTIMAL, RANDOM, FIFO}\n");
        printf( "   num_frames   - number of page frames {int > 0}\n");
        printf( "   show_process - print page table after each ref is processed {1 or 0}\n");
        printf( "   debug        - verbose debugging output {1 or 0}\n");
        return 0;
}


// Function to print results after algo is run
int print_stats(Algorithm algo)
{
        print_summary(algo);
        print_list(algo.data->page_table.lh_first, "Frame #", "Page Ref");
        return 0;
}





// Function to print summary report of an Algorithm
int print_summary(Algorithm algo)
{
        printf("%s Algorithm\n", algo.label);
        printf("Frames in Mem: %d, ", num_frames);
        printf("Hits: %d, ", algo.data->hits);
        printf("Misses: %d, ", algo.data->misses);
        printf("Hit Ratio: %f\n", (double)algo.data->hits/(double)(algo.data->hits+algo.data->misses));
        return 0;
}


// Print list
int print_list(struct Frame *head, const char* index_label, const char* value_label)
{
        int colsize = 9, labelsize;
        struct Frame *framep;
        // Determine lanbel col size from text
        if (strlen(value_label) > strlen(index_label))
                labelsize = strlen(value_label) + 1;
        else
                labelsize = strlen(index_label) + 1;
        /* Forward traversal. */
        printf("%-*s: ", labelsize, index_label);
        for (framep = head; framep != NULL; framep = framep->frames.le_next)
        {
                printf("%*d", colsize, framep->index);
        }
        printf("\n%-*s: ", labelsize, value_label);
        for (framep = head; framep != NULL; framep = framep->frames.le_next)
        {
                if(framep->page == -1)
                        printf("%*s", colsize, "_");
                else
                        printf("%*d", colsize, framep->page);
        }
        printf("\n%-*s: ", labelsize, "Extra");
        for (framep = head; framep != NULL; framep = framep->frames.le_next)
        {
                printf("%*d", colsize, framep->extra);
        }
        printf("\n%-*s: ", labelsize, "Time");
        for (framep = head; framep != NULL; framep = framep->frames.le_next)
        {
                printf("%*d ", colsize, (int32_t) (framep->time%200000000));
        }
        printf("\n\n");
        return 0;
}


// Clean up memory
int cleanup()
{
        size_t i = 0;
        for (i = 0; i < num_algos; i++)
        {
                /* Clean up memory, delete the list */
                while (algos[i].data->page_table.lh_first != NULL)
                {
                        LIST_REMOVE(algos[i].data->page_table.lh_first, frames);
                }
                while (algos[i].data->victim_list.lh_first != NULL)
                {
                        LIST_REMOVE(algos[i].data->victim_list.lh_first, frames);
                }
        }
        return 0;
}



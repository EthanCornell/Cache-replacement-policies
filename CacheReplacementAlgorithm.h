/*
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


#ifndef PAGEREPLACEMENTALGORITHM_H
#define PAGEREPLACEMENTALGORITHM_H


// Data structures
// List for page tables and victim lists
LIST_HEAD(Page_Ref_List, Page_Ref) page_refs;
// List for page tables and victim lists
LIST_HEAD(Frame_List, Frame);

// struct to hold Frame info
// typedef struct Page_Ref
// {
//         LIST_ENTRY(Page_Ref) pages; // frames node, next
//         int page_num;
// } Page_Ref;

typedef struct Page_Ref {
    LIST_ENTRY(Page_Ref) pages; 
    int page_num;
    int pid;  // Add this line if you need the process ID
} Page_Ref;



// struct to hold Frame info
typedef struct Frame
{
        LIST_ENTRY(Frame) frames;  // frames node, next
        int index;                 // frame position in list... not really needed
        int page;                  // page frame points to, -1 is empty
        time_t time;               // time added/accessed
        int extra;                 // extra field for per-algo use
        int lastUsed;  // For LRU
        int frequency; // For LFU
} Frame;

// struct to hold Algorithm data
typedef struct {
        int hits;                      // number of times page was found in page table
        int misses;                    // number of times page wasn't found in page table
        struct Frame_List page_table;  // List to hold frames in page table
        struct Frame_List victim_list; // List to hold frames that were replaced in page table
        Frame *last_victim;            // Holds last frame used as a victim to make inserting to victim list faster
        void *extra;                   // For storing additional data
} Algorithm_Data;

// an Algorithm
typedef struct {
        const char *label;                  // Algorithm name
        int (*algo)(Algorithm_Data *data);  // Pointer to algorithm function
        int selected;                       // Should algorithm be run, 1 or 0
        Algorithm_Data *data;               // Holds algorithm data to pass into algorithm function
} Algorithm;


// Init/cleanup functions
int init();                                 // init lists and variable, set up config defaults, and load configs
void gen_page_refs();
Page_Ref* gen_ref();
Algorithm_Data *create_algo_data_store();   // returns empty algorithm data
Frame *create_empty_frame(int index);       // returns empty frame
int cleanup();                              // frees allocated memory


// Control functions 
int event_loop();                           // loops for each page call
int page(int page_ref);                     // page all algos with page ref
int get_ref();                              // get next page ref however you like
int add_victim(struct Frame_List *victim_list, struct Frame *frame); // add victim frame to a victim list


// Output functions
int print_help(const char *binary);           // prints help screen
int print_list(struct Frame *head, const char* index_label, const char* value_label); // prints a list
int print_stats(Algorithm algo);              // detailed stats
int print_summary(Algorithm algo);            // one line summary


// Algorithm functions
int OPTIMAL(Algorithm_Data *data);
int RANDOM(Algorithm_Data *data);
int FIFO(Algorithm_Data *data);
int LRU(Algorithm_Data *data);
int CLOCK(Algorithm_Data *data);
int NFU(Algorithm_Data *data);
int AGING(Algorithm_Data *data);
int MRU(Algorithm_Data *data);
int NRU(Algorithm_Data *data);
int MFU(Algorithm_Data *data);
int LFRU(Algorithm_Data *data);

#endif

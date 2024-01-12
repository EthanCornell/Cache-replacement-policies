# Cache-replacement-policies
Cache replacement policies in C
This repository contains a comprehensive implementation of various page replacement algorithms written in C. These algorithms are crucial in operating systems for managing pages in memory. The project is designed for educational purposes, providing insight into how different page replacement strategies work under varying circumstances.

Algorithms Implemented:
1. Optimal Page Replacement
2. Random Page Replacement
3. First-In, First-Out (FIFO)
4. Least Recently Used (LRU)
5. Clock Replacement
6. Not Frequently Used (NFU)
7. Aging
8. Most Recently Used (MRU)
9. Not Recently Used (NRU)
10.Most Frequently Used (MFU)
11.Least Frequently Recently Used (LFRU)
Each algorithm has its unique approach to deciding which page to evict when a page fault occurs, providing various efficiency levels based on the specific use case.

Features:
Comprehensive implementation of 11 different page replacement strategies.
Configurable settings for number of frames, page reference size, and total number of page calls.
Debugging and verbose output options for in-depth analysis.
Custom LFRU algorithm implementation demonstrating a hybrid approach.
Usage:
Clone the repository: git clone https://github.com/EthanCornell/Cache-replacement-policies.git
Navigate to the project directory: cd page-replacement-algorithms
Compile the source code: gcc main.c -o page_replacement
Run the program: ./page_replacement [input file] [algorithm] [num_frames] [show_process] [debug]
Replace [input file], [algorithm], [num_frames], [show_process], and [debug] with your preferred settings.

Contributing:
Contributions to enhance or extend the functionality of this project are welcome. Please feel free to fork the repository, make changes, and submit a pull request.

License:



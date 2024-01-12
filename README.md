# Cache-replacement-policies
Cache replacement policies in C
This repository contains a comprehensive implementation of various cache replacement algorithms written in C. These algorithms play a crucial role in operating systems for managing pages in memory. The project is designed primarily for educational purposes, aiming to provide insight into how different page replacement strategies function under varying circumstances.

## Algorithms Implemented

- Optimal Page Replacement
- Random Page Replacement
- First-In, First-Out (FIFO)
- Least Recently Used (LRU)
- Clock Replacement
- Not Frequently Used (NFU)
- Aging
- Most Recently Used (MRU)
- Not Recently Used (NRU)
- Most Frequently Used (MFU)
- Least Frequently Recently Used (LFRU)

Each algorithm employs its unique approach to determine which page to evict when a page fault occurs, offering various efficiency levels based on the specific use case.

## Features

- Comprehensive implementation of 11 different page replacement strategies.
- Configurable settings for the number of frames, page reference size, and the total number of page calls.
- Debugging and verbose output options for in-depth analysis.
- Custom LFRU algorithm implementation demonstrating a hybrid approach.

## Usage

1. Clone the repository:
Clone the repository: git clone https://github.com/EthanCornell/Cache-replacement-policies.git

2. Navigate to the project directory:
cd page-replacement-algorithms

4. Compile the source code:
gcc main.c -o page_replacement

5. Run the program:
./page_replacement [input file] [algorithm] [num_frames] [show_process] [debug]

Replace `[input file]`, `[algorithm]`, `[num_frames]`, `[show_process]`, and `[debug]` with your preferred settings.

## Contributing

Contributions to enhance or extend the functionality of this project are welcome. Please feel free to fork the repository, make changes, and submit a pull request.




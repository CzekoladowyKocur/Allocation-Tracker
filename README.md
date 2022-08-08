# Allocation tracker
Single-header C++20/C++17 memory tracker and leak detector.

## Features
The tracker tracks all of the requested allocations via it's API until freed, providing
verbose information:
* Name of the type allocated (eg. char, int, struct UserStructure)
* Filename and line where the allocation had occurred 
* Memory address
* Total size of the allocation in bytes

By default, the tracker is enabled only in debug-mode. Release mode makes API calls 
no different to standard ones, making it as fast as possible.

## Screenshots
![Tracks](https://github.com/CzekoladowyKocur/Allocation-Tracker/blob/master/screenshots/tracks.png)

## API
Simply change the new/delete/delete[] calls to cinew/cindel/cindelarr macros, as shown below:
```
int* integer = new int; => 
int* integer = cinew int;

int* integerArray = new int[10]; => 
int* integerArray = cinew int[10];

delete integer; => 
cindel integer;

delete[] integerArray; =>
cindelarr integerArray;
``` 
These macros can be easily modified to your preferences.

## How to use
Upon each thread destruction all unreleased thread-local allocations are dumped to the 
console. To explicitly dump current memory state, use CinDumpMemory() function. A costum memory
dump function can be provided instead of the default one. **Allocating memory and freeing it in 
a different thread is forbidden**. Threads keep local state. However, this can be easily changed
with a global mutex and a shared state instead, although making it not lock-free.

### Build
To build the project, run the provided batch script.
The project comes with an example of how to use the features.
Currently windows only, vs 2022 expected (can be easily changed in script). 

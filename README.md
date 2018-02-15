# Memory-Allocation
Custom Memory Allocation in C

> This is implemented for 64 bit systems

## Design
This code implements memory allocation functions in c to replace the standard malloc, realloc, and free functions.

The design is to use segmented (i.e. 'bucketed') double linked lists of free blocks, utilizing a first fit algorithm to write over free blocks. 

Blocks have an 8 byte header and footer for traversal and for determining state. For extra space utilization, footer blocks are not used on allocated blocks. 

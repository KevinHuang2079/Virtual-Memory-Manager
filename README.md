# Virtual Memory Manager
* Simulates the mapping of virtual memory of TLB and page Table
* Reads in virtual addresses from file (file name through command line input) assuming 16 bit addresses and 8 bit offset and page numbers
* After execution: Outputs the page fault rate and the TLB hit rate
* During execution: output the virtual address, page number, page offset, mapped physical addresses, and data at the mapped physical address

## Identifying Information
* Name: Kevin Huang
* Student ID: 2402212
* Email: kevhuang@chapman.edu
* Course: cpsc 380-02
* Assignment: Assignment 6 - Virtual Memory Manager

## Source Files  
* prodcon.c

## Known Errors
* none

## Build Insructions
* gcc vmmgr.c -o vmmgr

## Execution Instructions
* ./prodcon <addresses_file_name.txt>

## gcc version
* Apple clang version 14.0.0 (clang-1400.0.29.202)

## Sample Input
* ./prodcon addresses.txt



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <pthread.h>
#include <math.h>
#include <ctype.h>
#include <glib.h>
#include <limits.h>
#include <stdbool.h>

#define PAGE_SIZE 256  // Page size in bytes
#define NUM_FRAMES 256 // Number of frames in physical memory
GHashTable* TLB;
GHashTable* pageTable;
char physicalMemory[PAGE_SIZE][NUM_FRAMES]; //256 * 256
int seekCounter;

void printTable(GHashTable *table) {
    GHashTableIter iter;
    gpointer key, value;
    
    g_hash_table_iter_init(&iter, table);

    while (g_hash_table_iter_next(&iter, &key, &value)) {
        printf("Key: %d, Value: %d\n", GPOINTER_TO_INT(key), GPOINTER_TO_INT(value));
    }
}

int loadPhysicalMemory(int pageNumber, const char data[256]){
    unsigned int openFrame = -1;
    //iterate through a page to find an open slot
    for (unsigned int i = 0 ; i < 255 ; ++i){
        if (physicalMemory[pageNumber][i] == '\0') {
            memcpy(&physicalMemory[pageNumber][i], data, PAGE_SIZE);
            openFrame = i;
            break; 
        }
    }
    printf("loadPhysicalMemory-> openFrame: %d\n", openFrame);
    return openFrame;
}

int* tlbLookUP(int* pageNumber){
    int* frameNumber = NULL;
    printf("tlbLookUp -> pageNumber: %d\n", *pageNumber);
    if (g_hash_table_lookup(TLB,pageNumber) != NULL){
        frameNumber = g_hash_table_lookup(TLB,pageNumber);
        return frameNumber;
    }
    
    return frameNumber;
}

int* pageLookUp(int* pageNumber){
    int* frameNumber = NULL;
    if (g_hash_table_lookup(pageTable,pageNumber) != NULL){
        //set valid bit
        frameNumber = g_hash_table_lookup(pageTable,pageNumber);
        return frameNumber;
    }
    //set frameNumber for this pageNumber
    return frameNumber;
}

//TODO: add a if statement to check if TLB.size() >=16 so i can use LRU to swap
void loadTLB(int pageNumber, int pageFrame){
    //type cast ints to gpointers
    gpointer key = GINT_TO_POINTER(pageNumber);
    gpointer value = GINT_TO_POINTER(pageFrame);
    //insert into TLB
    g_hash_table_insert(TLB, key, value);
    printTable(TLB);
}

void printData_InPhysical(int pageNumber, int frameNumber) {
    char line[PAGE_SIZE]; 
    memcpy(line, physicalMemory[pageNumber], PAGE_SIZE);  

    printf("Data in physical memory (page %d, frame %d):\n", pageNumber, frameNumber);
    for (int i = 0; i < PAGE_SIZE; i++) {
        printf("%hhd ", (char)line[i]);
    }
    printf("\n");
}

void loadPageTable(FILE* fp, int pageNumber) {
    // Load data from file into memory
    int offset = 256 * pageNumber;
    char line[256];
    fseek(fp, offset, SEEK_SET);
    fread(line, sizeof(line), 1, fp); 

    // Load the data into physical memory and update the page table
    int pageFrame = loadPhysicalMemory(pageNumber, line);
    printData_InPhysical(pageNumber, pageFrame);

    printf("pageNumber: %d, pageFrame: %d\n", pageNumber, pageFrame);
    gpointer key = GINT_TO_POINTER(pageNumber);
    gpointer value = GINT_TO_POINTER(pageFrame);
    printf("key: %p, value: %p\n", key, value);

    // Debugging print statements
    printf("Inserting key: %p, value: %p into pageTable: %p\n", key, value, pageTable);

    // Ensure pageTable is valid before insertion
    if (pageTable != NULL) {
        gboolean insert_result = g_hash_table_insert(pageTable, key, value);
        printf("Insertion result: %s\n", insert_result ? "Success" : "Failed");
    } else {
        printf("Error: pageTable is NULL\n");
    }

    printTable(pageTable);
}



void printBinary(unsigned int num) {
    int i;
    for (i = 31; i >= 0; i--) {
        printf("%d", (num >> i) & 1);
        if (i % 4 == 0) printf(" "); // Add space every 4 bits for readability
    }
}


int main( int argc, char* argv[]){
    char* file = argv[1];
    FILE* addressPointer = fopen(file,"r");
    FILE* BSptr = fopen("BACKING_STORE.bin", "rb");//backing_store.bin pointer
    if (BSptr == NULL) {
        fprintf(stderr, "Error opening BACKING_STORE.bin\n");
        return -1; // or exit(1) depending on your program flow
    }
    TLB = g_hash_table_new(g_int_hash, g_int_equal); 
    pageTable = g_hash_table_new_full(g_int_hash, g_int_equal, g_free, NULL); 

    for (unsigned int i = 0 ; i < 255 ; ++i){
        for (unsigned int j = 0 ; j < NUM_FRAMES; ++j){
            memcpy(&physicalMemory[i][j], "\0", PAGE_SIZE);
        }
    }
    
    int counter = 0;//for testing, iterating just twice for now

    char line[11];
    while (fgets(line, sizeof(line), addressPointer) != NULL && counter <1){
        unsigned int vAddr = atoi(line);
        int pageNumber = (vAddr >> 8);
        int pageOffset = vAddr & 0x000000ff;

        printBinary(vAddr);
        printf(" = vaddr: %d\n", vAddr);

        printBinary(pageNumber);
        printf(" = pageNumber: %d\n", pageNumber);

        printBinary(pageOffset);
        printf(" = offset: %d\n", pageOffset);

        int pAddr;
        int frameNumber;
        int* lookUpResult = tlbLookUP(&pageNumber);

        if (lookUpResult == NULL){//tlb miss     
            //search pageTable
            printf("tlb miss\n");    
            lookUpResult = pageLookUp(&pageNumber);
            printf("pageLookUp -> pageNumber: %d\n", pageNumber);

            if (lookUpResult == NULL){//page fault
                //load physical frame from backup into pageTable
                loadPageTable(BSptr, pageNumber);

                //search pageTable again
                printf("test4");
                frameNumber = *pageLookUp(&pageNumber);
                printf("test5");
                pAddr = (frameNumber << 8) | pageOffset;
                printf("page fault -> physical address: %d\n", pAddr);
            }
            else{//page table hit
                frameNumber = *lookUpResult;
                pAddr = (frameNumber << 8) | pageOffset;
                //load tlb
                loadTLB(pageNumber, frameNumber);
                printf("page hit -> physical address: %d\n", pAddr);
            }
        }
        else{//tlb hit
            frameNumber = *lookUpResult;
            pAddr = (frameNumber << 8) | pageOffset;
            printf("tlb hit -> physical address: %d", pAddr);
        }
        
        //TODO:
        printf("data at pAddr: %s", physicalMemory[pAddr]);
        counter++;
    }
    
    fclose(addressPointer);
    fclose(BSptr);
    return 0;
}
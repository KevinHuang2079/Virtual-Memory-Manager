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
#include <limits.h>
#include <stdbool.h>

#define PAGE_SIZE 256  
#define NUM_FRAMES 256 
#define TLB_SIZE 16
int (*TLB)[3]; // Pointer to a 2D array with 3 ints (pageNumber, pageFrame, addressCounter)
int (*pageTable)[2]; // Pointer to a 2D array with 2 ints (pageNumber, pageFrame)
char physicalMemory[PAGE_SIZE * NUM_FRAMES]; 
int addressCounter;

void printTLB() {
    for (unsigned int i = 0; i < TLB_SIZE; i++) {
        if ( TLB[i][0] ){
            printf("TLB[%d]: (pageNumber)%d, (pageFrame)%d, (LRUcounter)%d\n", i, TLB[i][0], TLB[i][1], TLB[i][2]); 
        }
    }
}

void printPageTable() {
    for (unsigned int i = 0; i < PAGE_SIZE; i++) {
        if (pageTable[i][0] != -1){
            printf("pageTable[%d]: (pageNumber)%d, (pageFrame)%d\n", i, pageTable[i][0], pageTable[i][1]); 
        }
    }
}

int tlbLookUP(int pageNumber) {
    int frameNumber = -1;
    for (int i = 0; i < TLB_SIZE; i++) {
        if (TLB[i][0] == pageNumber) {
            frameNumber = TLB[i][1];
            break; 
        }
    }
    return frameNumber;
}

int pageLookUp(int pageNumber){
    int frameNumber = -1;
    for (int i = 0; i < PAGE_SIZE; i++) {
        if (pageTable[i][0] == pageNumber) {
            frameNumber = pageTable[i][1];
            break; 
        }
    }
    return frameNumber;
}

//place value into TLB
void loadTLB(int pageNumber, int pageFrame, int addressCounter) {
    static int tlbCount = 0;
    if (tlbCount == TLB_SIZE) { 
        int targetIndex = 0;
        int minLRU = TLB[0][2];
        for (int i = 1; i < TLB_SIZE; ++i) {
            if (TLB[i][2] < minLRU) {
                targetIndex = i;
                minLRU = TLB[i][2];
            }
        }
        TLB[targetIndex][0] = pageNumber;
        TLB[targetIndex][1] = pageFrame;
    } else {
        for (int i = 0; i < TLB_SIZE; ++i) {
            if (TLB[i][1] == -1) {
                TLB[i][0] = pageNumber;
                TLB[i][1] = pageFrame;
                TLB[i][2] = addressCounter;
                tlbCount++;
                break;
            }
        }
    }
}

//place value into page Table
void loadPageTable(FILE* fp, int pageNumber) {
    // Load the data into physical memory and update the page table
    int pageFrame = pageNumber;
    for (unsigned int i = 0 ; i < PAGE_SIZE ; ++i){//find first empty slot in table
        if (pageTable[i][0] == -1){
            pageTable[i][0] = pageNumber;
            pageTable[i][1] = pageFrame;
            break;
        }
    }
}

void printBinary(unsigned int num) {
    int i;
    for (i = 31; i >= 0; i--) {
        printf("%d", (num >> i) & 1);
        if (i % 4 == 0) printf(" "); //add space every 4 bits 
    }
}

void initializeMemory() {
    TLB = malloc(TLB_SIZE * sizeof(*TLB)); // Allocate memory for TLB
    pageTable = malloc(PAGE_SIZE * sizeof(*pageTable)); // Allocate memory for pageTable

    if (TLB == NULL || pageTable == NULL) {
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < TLB_SIZE; i++) {
        TLB[i][0] = -1; // pageNumber
        TLB[i][1] = -1; // pageFrame
        TLB[i][2] = 0;  // addressCounter (initialize as needed)
    }

    for (int i = 0; i < PAGE_SIZE; i++) {
        pageTable[i][0] = -1; // pageNumber
        pageTable[i][1] = -1; // pageFrame
    }
}

void printData_InPhysical(int pAddr) {
    char line[256];
    memcpy(line, &physicalMemory[pAddr], 256); 
    for (int i = 0; i < 256; i++) {
        printf("%hhd ", line[i]); 
    }
    printf("\n");
}

//place values into physical memory
void loadPhysicalMemory(int pAddr, const char data[256]){
    memcpy(&physicalMemory[pAddr], data, PAGE_SIZE);
}

//function to read data from the backing store file
void BackingStore(int pageNumber, int pageOffset, FILE* fp) {
    // Load data from file into memory
    int offset = 256 * pageNumber;
    char line[256];
    fseek(fp, offset, SEEK_SET);
    fread(line, sizeof(line), 1, fp); 

    //get pAddr from frameNumber
    int frameNumber = pageLookUp(pageNumber);
    int pAddr = (frameNumber << 8) | pageOffset;
    //load into pAddr
    loadPhysicalMemory(pAddr, line);
    printf("data loaded from backing store:\n");
    printData_InPhysical(pAddr);
    printf("page fault -> physical address: %d\n", pAddr);
}

int main( int argc, char* argv[]){
    addressCounter = 0;//counts the # addresses read
    char* file = argv[1];
    FILE* addressPointer = fopen(file,"r");
    FILE* BSptr = fopen("BACKING_STORE.bin", "rb");//backing_store.bin pointer
    initializeMemory();
    int pageFaultCounter = 0;
    int tlbHitCounter = 0;

    if (argc != 2) {
        printf("Usage: ./vmmgr <address.txt>\n");
        return 1;
    }

    if (addressPointer == NULL){
        printf("Not valid file");
        return 1;
    }

    char line[11];
    while (fgets(line, sizeof(line), addressPointer) != NULL){
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
        int lookUpResult = tlbLookUP(pageNumber);
        addressCounter++;

        
        if (lookUpResult == -1){//tlb miss     
            //search pageTable
            lookUpResult = pageLookUp(pageNumber);

            if (lookUpResult == -1){//page fault
                //load the pageTable
                loadPageTable(addressPointer, pageNumber);
                //load from backingStore into physical memory
                BackingStore(pageNumber, pageOffset, BSptr);
                pageFaultCounter++;
            }
            else{//page table hit
                frameNumber = lookUpResult;
                pAddr = (frameNumber << 8) | pageOffset;
                //load tlb
                loadTLB(pageNumber, frameNumber, addressCounter);
                printf("page hit -> physical address: %d\n", pAddr);
                printf("data at pAddr:\n");
                printData_InPhysical(pAddr);
            }
        }
        
        else{//tlb hit
            frameNumber = lookUpResult;
            pAddr = (frameNumber << 8) | pageOffset;
            printf("tlb hit -> physical address: %d\n", pAddr);
            printf("data at pAddr:\n");
            printData_InPhysical(pAddr);
            tlbHitCounter++;
        }
        printf("\n");
    }
    //print results
    printf("PageFaultRate: %f percent\n", ((double)pageFaultCounter/addressCounter)*100);
    printf("TLBhitRate: %f percent\n", ((double)tlbHitCounter/addressCounter)*100);
    
    fclose(addressPointer);
    fclose(BSptr);
    free(TLB);
    free(pageTable);
    return 0;
}
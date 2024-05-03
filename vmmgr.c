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


GHashTable* TLB;
GHashTable* pageTable;
char* physicalMemory;
int seekCounter;




unsigned int readLine(FILE* fp){
    fseek( fp, seekCounter*sizeof(int), SEEK_SET );
    char* str = malloc(10 * sizeof(char*));
    printf("%s", fgets(str, 10, fp));
    int result = atoi(str);
    return result;
}

int* get_vAddrInfo(FILE* fp){
    unsigned int vAddr = readLine(fp);
    int pageNumber = (vAddr >> 8);
    int offset = vAddr & 0x000000ff;
    printf("vaddr: %d\n", vAddr);
    printf("pageNumber: %d\n", pageNumber);
    printf("offset: %d\n", offset);
    int* result = malloc(2*(sizeof(int)));
    result[0] = pageNumber;
    result[1] = offset;
    return result;
}

int* tlbLookUP(FILE* fp){
    int* v_addrInfo = get_vAddrInfo(fp);
    int* pageNumber = &v_addrInfo[0];
    int offset = v_addrInfo[1];
    int* frameNumber;
    printf("pageNumber: %d", *pageNumber);
    if (g_hash_table_lookup(TLB,pageNumber) != NULL){
        frameNumber = g_hash_table_lookup(TLB,pageNumber);
        return frameNumber;
    }
    
    return frameNumber;
}

int* pageLookUp(FILE* fp){
    int* v_addrInfo = get_vAddrInfo(fp);
    int* pageNumber = &v_addrInfo[0];
    int offset = v_addrInfo[1];
    int* frameNumber = NULL;
    printf("pageNumber: %d", *pageNumber);
    if (g_hash_table_lookup(pageTable,pageNumber) != NULL){
        //set valid bit
        frameNumber = g_hash_table_lookup(pageTable,pageNumber);
        return frameNumber;
    }
    //set frameNumber for this pageNumber
    return frameNumber;
}

//TODO
int loadTLB(FILE* fp, int pageNumber, int pageOffset){

}
int loadPageTable(FILE* fp, int pageNumber){
    int offset = 256 * pageNumber;
    fseek( fp, offset, SEEK_SET );

    char* str = malloc(256 * sizeof(char*));
    printf("%s", fgets(str, 256, fp));
    int result = atoi(str);
    free(str);
    return result;
}


int main( int argc, char* agrv[]){
    FILE *fp;
    seekCounter = 0;
    addressPointer = fopen("addresses.txt","r");
    backingStorePointer = fopen("BACKING_STORE.bin","r");
    TLB = g_hash_table_new_full(g_int_hash, g_int_equal, g_free, NULL); 
    pageTable = g_hash_table_new_full(g_int_hash, g_int_equal, g_free, NULL); 
    
    while (!feof(addressPointer)){
        int* lookUpResult = tlbLookUP(addressPointer);
        int pageNumber = get_vAddrInfo(addressPointer)[0];
        int pageOffset = get_vAddrInfo(addressPointer)[1];
        int pAddr;

        if (lookUpResult == NULL){//tlb miss            
            lookUpResult = pageLookUp(addressPointer);
            if (lookUpResult == NULL){//page fault
                //load physical frame from backup
                printf("page fault -> physical address: %d", pAddr);
            }
            else{//page table hit
                int frameNumber = *lookUpResult;
                pAddr = frameNumber + pageOffset;
                //load tlb
                int loadTLB(fp, pageNumber, pageOffset);
                printf("page hit -> physical address: %d", pAddr);
            }
            seekCounter++;
        }
        else{//tlb load
            frameNumber = *lookUpResult;
            int pAddr = frameNumber + pageOffset;
            seekCounter++;
            printf("tlb hit -> physical address: %d", pAddr);
        }
    }
    
    fclose(fp);
    return 0;
}
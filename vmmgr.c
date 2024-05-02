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
// #include <glib.h>
#include <limits.h>
#include <stdbool.h>


// GHashTable* TLB;
// GHashTable* pageTable;
char* physicalMemory;
int seekCounter;




unsigned int readLine(FILE* fp){
    fseek( fp, seekCounter*sizeof(int), SEEK_SET );
    char* str = malloc(10 * sizeof(char*));
    printf("%s", fgets(str, 10, fp));
    int result = atoi(str);
    seekCounter++;
    return result;
}

int* lookPageTable(FILE* fp){
    unsigned int vAddr = readLine(fp);
    int pageNumber = (vAddr >> 8);
    int offset = vAddr & 0x000000ff;
    printf("vaddr: %d\n", vAddr);
    printf("pageNumber: %d\n", pageNumber);
    printf("offset: %d\n", offset);
    int result[2];
    result[0] = pageNumber;
    result[1] = offset;
    return result;
}

int main( int argc, char* agrv[]){
    FILE *fp;
    seekCounter = 0;
    fp = fopen("addresses.txt","r");
    
    
    fclose(fp);
    return 0;
}
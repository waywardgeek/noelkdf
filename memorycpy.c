// This file simply fills 2GB of RAM with no hashing.  It's a useful benchmark for
// noelkdf, which hopefully should aproach this speed.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define MEM_SIZE (1LLU << 31)
#define MEM_LENGTH (MEM_SIZE/sizeof(unsigned int))
#define NUM_THREADS 1
#define PAGE_SIZE (1024*16)
#define PAGE_LENGTH (PAGE_SIZE/sizeof(unsigned int))

static inline unsigned int H(unsigned int v) {
    return v*1103515245 + 12345;
}

static void *moveMem(void *memPtr) {
    unsigned int *mem = (unsigned int *)memPtr;
    //memmove(mem, mem + 8, (MEM_LENGTH-1)*sizeof(unsigned int)/NUM_THREADS);
    mem[0] = 0xdeadbeef;
    unsigned int hash = 1;
    unsigned int i;
    unsigned int prevAddr = 0;
    for(i = 1; i < MEM_LENGTH/NUM_THREADS;) {
        prevAddr = H(i) % i;
        unsigned int j;
        for(j = 0; j < PAGE_LENGTH; j++) {
            hash = hash*(mem[prevAddr++] | 1) + 12345;
            mem[i++] = hash;
        }
    }
    pthread_exit(NULL);
}

int main() {
    pthread_t threads[NUM_THREADS];
    unsigned int *mem = (unsigned int *)malloc(MEM_SIZE);
    int rc;
    long t;
    for(t = 0; t < NUM_THREADS; t++) {
        rc = pthread_create(&threads[t], NULL, moveMem, (void *)(mem + t*MEM_LENGTH/NUM_THREADS));
        if (rc){
            fprintf(stderr, "Unable to start threads\n");
            return 1;
        }
    }
    // Wait for threads to finish
    for(t = 0; t < NUM_THREADS; t++) {
        (void)pthread_join(threads[t], NULL);
    }
    printf("%u\n", ((unsigned int *)mem)[rand() % MEM_LENGTH]);
    return 0;
}

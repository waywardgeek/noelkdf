// This file simply fills 2GB of RAM with no hashing.  It's a useful benchmark for
// keystretch, which hopefully should aproach this speed.
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <immintrin.h>

#define MEM_SIZE (1LL << 31)
#define MEM_LENGTH (MEM_SIZE/sizeof(__m128i))
#define NUM_THREADS 2
#define BLOCK_SIZE 16384
#define BLOCK_LENGTH (BLOCK_SIZE/sizeof(__m128i))
#define NUM_BLOCKS (MEM_LENGTH/(NUM_THREADS*BLOCK_LENGTH))

volatile bool quit = false;

static void *multHash(void *hashPtr) {
    uint32_t *hash = hashPtr;
    uint64_t i;
    for(i = 0; i < 8; i++) {
        hash[i] = i;
    }
    uint32_t value;
    //for(i = 0; !quit; i++) {
    for(i = 0; i < 240711928/8; i++) {
        uint32_t j;
        for(j = 0; j < 8; j++) {
            value = (value*(hash[j] | 1)) ^ (hash[7-j] >> 1);
            hash[j] = value;
        }
    }
    printf("Completed %lu multiplications\n", i*8);
    for(i = 0; i < 8; i++) {
        printf("%u\n", hash[i]);
    }
    pthread_exit(NULL);
}

static void *hashMem(void *memPtr) {
    __m128i *mem = (__m128i *)memPtr;
    uint32_t i;
    for(i = 0; i < BLOCK_LENGTH*2; i++) {
        __m128i v = _mm_set_epi32(i, i, i, i);
        mem[i] = v;
    }
    __m128i *prev1 = mem + BLOCK_LENGTH;
    __m128i *prev2 = mem;
    __m128i *to = mem + BLOCK_LENGTH;
    for(i = 2; i < NUM_BLOCKS; i++) {
        __m128i *from = mem + BLOCK_LENGTH*(0xdeadbeef % i);
        uint32_t j;
        for(j = 0; j < BLOCK_LENGTH; j++) {
            __m128i value = _mm_add_epi64(*prev1++, *prev2++);
            value = _mm_xor_si128(value, *from++);
            value = _mm_shuffle_epi32(value, _MM_SHUFFLE(2, 3, 0, 1));
            *to++ = value;
        }
        prev2 = prev1 - BLOCK_LENGTH;
    }
    pthread_exit(NULL);
}

int main() {
    pthread_t memThreads[NUM_THREADS], multThread;
    __m128i *mem = (__m128i *)aligned_alloc(32, MEM_SIZE);
    uint32_t hash[8];
    uint32_t r;
    for(r = 0; r < 1; r++) {
        int rc;
        rc = pthread_create(&multThread, NULL, multHash, hash);
        if (rc){
            fprintf(stderr, "Unable to start threads\n");
            return 1;
        }
        uint32_t t;
        for(t = 0; t < NUM_THREADS; t++) {
            rc = pthread_create(&memThreads[t], NULL, hashMem, (void *)(mem + t*MEM_LENGTH/NUM_THREADS));
            if (rc){
                fprintf(stderr, "Unable to start threads\n");
                return 1;
            }
        }
        // Wait for threads to finish
        for(t = 0; t < NUM_THREADS; t++) {
            (void)pthread_join(memThreads[t], NULL);
        }
        quit = true;
        (void)pthread_join(multThread, NULL);
    }
    printf("%u\n", ((unsigned int *)mem)[rand() % MEM_LENGTH]);
    free(mem);
    return 0;
}

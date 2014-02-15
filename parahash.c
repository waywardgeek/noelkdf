// This file simply fills 2GB of RAM with no hashing.  It's a useful benchmark for
// keystretch, which hopefully should aproach this speed.
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <immintrin.h>
#include <x86intrin.h>

#define MEM_SIZE (1LL << 31)
#define MEM_LENGTH (MEM_SIZE/sizeof(__m128i))
#define NUM_THREADS 2
#define BLOCK_SIZE 16384
//#define BLOCK_SIZE 1024
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

static void printM128(__m128i value) {
    uint32_t *p = (uint32_t *)&value;
    uint32_t i;
    for(i = 0; i < 4; i++) {
        printf("%u ", *p++);
    }
    printf("\n");
}

static void *hashMem(void *memPtr) {
    __m128i *mem = (__m128i *)memPtr;
    uint32_t i;
    // This needs to be replaced with crypto-strength initialization
    for(i = 0; i < BLOCK_LENGTH*2; i++) {
        __m128i v = _mm_set_epi32(i, i, i, i);
        mem[i] = v;
    }
    __m128i *prev1 = mem + BLOCK_LENGTH;
    __m128i *prev2 = mem;
    __m128i *to = mem + 2*BLOCK_LENGTH;
    __m128i shiftRightVal = _mm_set_epi32(25, 25, 25, 25);
    __m128i shiftLeftVal = _mm_set_epi32(7, 7, 7, 7);
    __m128i value1 = shiftRightVal;
    __m128i value2 = shiftRightVal;
    for(i = 2; i < NUM_BLOCKS; i++) {
        uint32_t randAddr = *(((uint32_t *)to) - 1);
        __m128i *from = mem + BLOCK_LENGTH*(randAddr % i);
        uint32_t j;
        for(j = 0; j < BLOCK_LENGTH/2; j++) {
            value1 = _mm_add_epi32(value1, *prev1++);
            value1 = _mm_add_epi32(value1, *prev2++);
            value1 = _mm_xor_si128(value1, *from++);
            // Rotate right 7
            value1 = _mm_or_si128(_mm_srl_epi32(value1, shiftRightVal), _mm_sll_epi32(value1, shiftLeftVal));
            *to++ = value1;
            value2 = _mm_add_epi32(value2, *prev1++);
            value2 = _mm_add_epi32(value2, *prev2++);
            value2 = _mm_xor_si128(value2, *from++);
            // Rotate right 7
            value2 = _mm_or_si128(_mm_srl_epi32(value2, shiftRightVal), _mm_sll_epi32(value2, shiftLeftVal));
            *to++ = value2;
            //printM128(value1);
            //printM128(value2);
        }
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

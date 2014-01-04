// This file simply fills 2GB of RAM with no hashing.  It's a useful benchmark for
// noelkdf, which hopefully should aproach this speed.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include "sha256.h"
#include "noelkdf.h"

struct ContextStruct {
    uint32 *mem;
    uint32 *threadKey;
    const uint8 *salt;
    uint32 saltSize;
    uint32 numPages;
};

// Hash the next page.
static inline void hashPage(uint32 *toPage, uint32 *prevPage, uint32 *fromPage) {
    uint32 i;
    *toPage++ = *prevPage + ((*fromPage * *(prevPage + 1)) ^ *(fromPage - 1 + PAGE_LENGTH));
    prevPage++;
    fromPage++;
    for(i = 1; i < PAGE_LENGTH - 1; i++) {
        *toPage++ = *prevPage + ((*fromPage * *(prevPage + 1)) ^ *(fromPage - 1));
        prevPage++;
        fromPage++;
    }
    *toPage = *prevPage + ((*fromPage * *(prevPage + 1 - PAGE_LENGTH)) ^ *(fromPage - 1));
}

static void *hashMem(void *contextPtr) {
    struct ContextStruct *c = (struct ContextStruct *)contextPtr;
    uint32 *mem = c->mem;

    // Initialize first page
    PBKDF2_SHA256((uint8 *)(void *)c->threadKey, THREAD_KEY_SIZE, c->salt, c->saltSize, 1,
        (uint8 *)(void *)mem, PAGE_LENGTH*sizeof(uint32));

    // Create pages sequentially by hashing the previous page with a random page.
    uint32 i;
    uint32 *toPage = mem + PAGE_LENGTH, *fromPage, *prevPage = mem;
    uint32 fromAddress = mem[1];
    for(i = 1; i < c->numPages; i++) {
        // Select a random from page
        fromAddress ^= prevPage[fromAddress % PAGE_LENGTH];
        fromPage = mem + PAGE_LENGTH*(fromAddress % i);
        hashPage(toPage, prevPage, fromPage);
        prevPage = toPage;
        toPage += PAGE_LENGTH;
    }

    // Hash the last page over the thread key
    PBKDF2_SHA256((uint8 *)(void *)prevPage, PAGE_LENGTH*sizeof(uint32), c->salt, c->saltSize, 1,
        (uint8 *)(void *)c->threadKey, THREAD_KEY_SIZE);
    pthread_exit(NULL);
}

// This is the prototype required for the password hashing competition.
// t_cost is an integer multiplier on CPU work.  m_cost is an integer number of MB of memory to hash.
int PHS(void *out, size_t outlen, const void *in, size_t inlen, const void *salt, size_t saltlen,
        unsigned int t_cost, unsigned int m_cost) {
    uint32 numPages = m_cost*(1LL << 20)/(NUM_THREADS*PAGE_LENGTH*sizeof(uint32));
    uint32 *mem = (uint32 *)malloc(numPages*PAGE_LENGTH*NUM_THREADS*sizeof(uint32));
    PBKDF2_SHA256(in, inlen, salt, saltlen, 2048, out, outlen);
    // Note: here is where we should clear the password, but the const qualifier forbids it
    // memset(in, '\0', inlen); // It's a good idea to clear the password ASAP!
    uint32 *threadKeys = (uint32 *)malloc(NUM_THREADS*THREAD_KEY_SIZE);
    int i;
    for(i = 0; i < t_cost; i++) {
        PBKDF2_SHA256(out, outlen, salt, saltlen, 1, (uint8 *)(void *)threadKeys, NUM_THREADS*THREAD_KEY_SIZE);
        pthread_t threads[NUM_THREADS];
        struct ContextStruct c[NUM_THREADS];
        int rc;
        long t;
        for(t = 0; t < NUM_THREADS; t++) {
            c[t].mem = mem + t*numPages*PAGE_LENGTH;
            c[t].numPages = numPages;
            c[t].salt = salt;
            c[t].saltSize = saltlen;
            c[t].threadKey = threadKeys + t*THREAD_KEY_LENGTH;
            rc = pthread_create(&threads[t], NULL, hashMem, (void *)(c + t));
            if (rc){
                fprintf(stderr, "Unable to start threads\n");
                return 1;
            }
        }
        // Wait for threads to finish
        for(t = 0; t < NUM_THREADS; t++) {
            (void)pthread_join(threads[t], NULL);
        }
        PBKDF2_SHA256((uint8 *)(void *)threadKeys, NUM_THREADS*THREAD_KEY_SIZE, salt, saltlen, 1, out, outlen);
    }
    return 0;
}

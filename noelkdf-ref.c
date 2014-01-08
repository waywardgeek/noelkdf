// This file simply fills 2GB of RAM with no hashing.  It's a useful benchmark for
// noelkdf, which hopefully should aproach this speed.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "sha256.h"
#include "noelkdf.h"

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;

#define THREAD_KEY_SIZE 64 // In bytes
#define THREAD_KEY_LENGTH (THREAD_KEY_SIZE/sizeof(uint32)) // In bytes
#define MEGA_BLOCK_SIZE ((1 << 20)/sizeof(uint32)) // 1MB in 32-bit words

struct ContextStruct {
    uint32 *mem;
    uint32 *threadKey;
    const uint8 *salt;
    uint32 saltSize;
    uint32 pageLength;
    uint32 numPages;
    uint32 parallelism;
};

// Hash the next page.
static inline void hashPage(uint32 *toPage, uint32 *prevPage, uint32 *fromPage,
        uint32 pageLength, uint32 parallelism, uint32 toPageNum) {
    uint32 numSequences = pageLength/parallelism;
    *toPage = 0; // In case parallism == 1, and we try to read *toPage before it's written
    uint32 hash = 0;
    uint32 i;
    for(i = 0; i < numSequences; i++) {
        // Do one sequential operation involving hash that can't be done in parallel
        hash += *fromPage*1103515245 + 12345;
        *toPage++ = *prevPage + ((*fromPage * *(prevPage + 1)) ^ *(fromPage + 1)) + hash;
        prevPage++;
        fromPage++;

        // Now do the rest all in parallel
        uint32 j;
        for(j = 1; j < parallelism; j++) {
            *toPage++ = *prevPage + ((*fromPage * *(prevPage + 1)) ^ *(fromPage + 1));
            prevPage++;
            fromPage++;
        }
    }
}

// To eliminate timing attacks, pick an address less than i that depends only on i.
// The mask parameter is the largest sequence of 1's less than i, so any value AND-ed with
// it will also be less than i.  The constants are from glibc's rand() function.
static inline uint32 hashAddress(uint32 i, uint32 mask) {
    return i - 1 - ((i*1103515245 + 12345) & mask);
}

// This is the function called by each thread.  It hashes a single continuous block of memory.
static void *hashMem(void *contextPtr) {
    struct ContextStruct *c = (struct ContextStruct *)contextPtr;

    // Initialize first page from the thread key
    PBKDF2_SHA256((uint8 *)(void *)c->threadKey, THREAD_KEY_SIZE, c->salt, c->saltSize, 1,
        (uint8 *)(void *)c->mem, c->pageLength*sizeof(uint32));

    // Create pages sequentially by hashing the previous page with a random page.
    uint32 *toPage = c->mem + c->pageLength, *fromPage, *prevPage = c->mem;
    uint32 mask = 0;
    uint32 i;
    for(i = 1; i < c->numPages; i++) {
        if(i > ((mask << 1) | 1)) {
            mask = (mask << 1) | 1;
        }
        // Select a random-ish from page that depends only on i
        fromPage = c->mem + c->pageLength*(hashAddress(i, mask));
        hashPage(toPage, prevPage, fromPage, c->pageLength, c->parallelism, i);
        prevPage = toPage;
        toPage += c->pageLength;
    }

    // Hash the last page over the thread key
    PBKDF2_SHA256((uint8 *)(void *)prevPage, c->pageLength*sizeof(uint32), c->salt, c->saltSize, 1,
        (uint8 *)(void *)c->threadKey, THREAD_KEY_SIZE);
    pthread_exit(NULL);
}

// This version allows for some more options than PHS.  They are:
//  num_threads     - the number of threads to run in parallel
//  page_size     - length of memory blocks hashed at a time
//  num_hash_rounds - number of SHA256 rounds to compute the intermediate key
//  parallelism     - number of inner loops allowed to run in parallel - should match
//                    user's machine for best protection
//  clear_in        - when true, overwrite the in buffer with 0's early on
//  return_memory   - when true, the hash data stored in memory is returned without being
//                    freed in the memPtr variable
int NoelKDF(void *out, size_t outlen, void *in, size_t inlen, const void *salt, size_t saltlen,
        unsigned int t_cost, unsigned int m_cost, unsigned int num_hash_rounds, unsigned int parallelism,
        unsigned int num_threads, unsigned int page_size, int clear_in, int return_memory, uint32 **memPtr) {

    // Allocate memory
    uint32 pageLength = (page_size << 10)/sizeof(uint32);
    uint32 numPages = m_cost*(1LL << 20)/(num_threads*pageLength*sizeof(uint32)) + 1;
    uint32 *mem = (uint32 *)malloc((uint64)numPages*pageLength*num_threads*sizeof(uint32) << t_cost);
    pthread_t *threads = (pthread_t *)malloc(num_threads*sizeof(pthread_t));
    uint32 *threadKeys = (uint32 *)malloc(num_threads*THREAD_KEY_SIZE);
    struct ContextStruct *c = (struct ContextStruct *)malloc(num_threads*sizeof(struct ContextStruct));
    if(mem == NULL || threads == NULL || threadKeys == NULL || c == NULL) {
        fprintf(stderr, "Unable to allocate memory\n");
        return 0;
    }

    // Compute intermediate key which is used to hash memory
    PBKDF2_SHA256(in, inlen, salt, saltlen, 2048, out, outlen);
    if(clear_in) {
        // Note that an optimizer may drop this, and that data may leak through registers
        // or elsewhere.  The hand optimized version should try to deal with these issues
        memset(in, '\0', inlen);
    }

    // Using t_cost in this outer loop enables a hashed password to be strenthened in the
    // future by increasing the t_cost parameter, without knowing the password.
    uint32 i;
    for(i = 0; i <= t_cost; i++) {

        // Initialize the thread keys from the intermediate key
        PBKDF2_SHA256(out, outlen, salt, saltlen, 1, (uint8 *)(void *)threadKeys, num_threads*THREAD_KEY_SIZE);

        // Launch threads.  Each hashes it's own separate block of memory, which improves performance.
        uint32 t;
        for(t = 0; t < num_threads; t++) {
            c[t].mem = mem + (uint64)t*numPages*pageLength;
            c[t].numPages = numPages;
            c[t].salt = salt;
            c[t].saltSize = saltlen;
            c[t].threadKey = threadKeys + t*THREAD_KEY_LENGTH;
            c[t].pageLength = pageLength;
            c[t].parallelism = parallelism;
            int rc = pthread_create(&threads[t], NULL, hashMem, (void *)(c + t));
            if (rc){
                fprintf(stderr, "Unable to start threads\n");
                return 1;
            }
        }
        // Wait for threads to finish
        for(t = 0; t < num_threads; t++) {
            (void)pthread_join(threads[t], NULL);
        }
        PBKDF2_SHA256((uint8 *)(void *)threadKeys, num_threads*THREAD_KEY_SIZE, salt, saltlen, 1, out, outlen);

        // Double memory usage for the next loop.
        numPages <<= 1;
    }

    // Free memory.  The optimized version should try to insure that memory is cleared.
    free(threads);
    free(threadKeys);
    free(c);
    if(return_memory) {
        *memPtr = mem;
    } else {
        free(mem);
    }
    return 0;
}

// This is the prototype required for the password hashing competition.
// t_cost is an integer multiplier on CPU work.  m_cost is an integer number of MB of memory to hash.
int PHS(void *out, size_t outlen, const void *in, size_t inlen, const void *salt, size_t saltlen,
        unsigned int t_cost, unsigned int m_cost) {
    return NoelKDF(out, outlen, (void *)in, inlen, salt, saltlen, t_cost, m_cost, 2048, 64, 16, 4096, 0, 0, NULL);
}

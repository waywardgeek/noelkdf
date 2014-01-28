

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

struct ContextStruct {
    uint32 *mem;
    uint8 *hash;
    pthread_mutex_t *countMutex;
    pthread_cond_t *countVar;
    uint32 *combinedValue;
    uint32 *threadCount;
    uint32 hashlen;
    uint32 id;
    uint32 blockLength;
    uint32 numBlocks;
};

// Simple rand() function.
//     from: http://www.codeproject.com/Articles/25172/Simple-Random-Number-Generation
static inline uint32 fastRand(uint32 *z, uint32 *w) {
    *z = 36969 * (*z & 65535) + (*z >> 16);
    *w = 18000 * (*w & 65535) + (*w >> 16);
    return (*z << 16) + *w;
}

// This function waits until every thread has reached the half-way point, and adds
// together all the 'value' variables frome each thread and returns the result.
static uint32 findHalfWayCombineThreadValue(struct ContextStruct *c, uint32 value) {
    pthread_mutex_lock(c->countMutex);
    *(c->combinedValue) += value;
    *(c->threadCount) -= 1;
    if(*(c->threadCount) == 0) {
        pthread_cond_broadcast(c->countVar);
    } else {
        pthread_cond_wait(c->countVar, c->countMutex);
    }
    pthread_mutex_unlock(c->countMutex);
    return *(c->combinedValue);
}

// This is the function called by each thread.  It hashes a single continuous block of memory.
static void *hashMem(void *contextPtr) {
    struct ContextStruct *c = (struct ContextStruct *)contextPtr;

    // Initialize the thread key from the intermediate key
    uint8 threadKey[c->blockLength*sizeof(uint32)];
    uint8 salt[sizeof(uint32)];
    be32enc(salt, c->id);
    PBKDF2_SHA256(c->hash, c->hashlen, salt, sizeof(uint32), 1, threadKey, c->blockLength*sizeof(uint32));
    be32dec_vect(c->mem, threadKey, c->blockLength*sizeof(uint32));

    uint32 z = 1, w = 1;
    uint32 *prevBlock = c->mem;
    uint32 *toBlock = c->mem + c->blockLength;
    uint32 *fromBlock;
    uint32 value = 1;
    uint32 i;
    for(i = 1; i < c->numBlocks; i++) {
        uint64 distance = value;
        if(i == c->numBlocks >> 1) {
            value ^= findHalfWayCombineThreadValue(c, value);
        }
        if(i < c->numBlocks >> 1) {
            distance = fastRand(&z, &w);
        } else {
            distance = value;
        }
        uint64 distanceSquared = (distance*distance) >> 32;
        uint64 distanceCubed = (distanceSquared*distance) >> 32;
        distance = ((i-1)*distanceCubed) >> 32;
        fromBlock = c->mem + (uint64)c->blockLength*(i - 1 - distance);
        uint32 j;
        for(j = 0; j < c->blockLength; j++) {
            value = value*(*prevBlock++ | 3) + *fromBlock++;
            *toBlock++ = value;
        }
    }
    pthread_exit(NULL);
}

// Use the resulting output hash, which does depend on the password, to hash a small
// percentage of the total memory.  This forces all the threads' memory to be loaded at
// once.
static void xorIntoHash(uint32 parallelism, uint32 *mem, uint8 *hash, uint32 hashSize,
        uint32 blockLength, uint32 numBlocks) {
    uint32 value = UINT32_MAX;
    uint32 hashPos = 0;
    uint32 memLength = parallelism*blockLength*numBlocks;
    uint32 i;
    for(i = 0; i < parallelism*100; i++) {
        value += mem[value % memLength] ^ i;
        uint32 j;
        for(j = 0; j < sizeof(uint32); j++) {
            if(hashPos == hashSize) {
                hashPos = 0;
            }
            hash[hashPos++] ^= (uint8)(value >> 24);
            value <<= 8;
        }
    }
}

// This version allows for some more options than PHS.  They are:
//  parallelism     - the number of threads to run in parallel
//  block_size      - length of memory blocks hashed at a time
//  num_hash_rounds - number of SHA256 rounds to compute the intermediate key
//  killer_factor   - 1 means hash m_cost locations in the cheat killer round.  K means
//                    hash m_cost/K locations.
//  parallelism     - number of inner loops allowed to run in parallel - should match
//                    user's machine for best protection
//  clear_in        - when true, overwrite the in buffer with 0's early on
//  return_memory   - when true, the hash data stored in memory is returned without being
//                    freed in the memPtr variable
int NoelKDF(void *out, size_t outlen, void *in, size_t inlen, const void *salt, size_t saltlen,
        void *data, size_t datalen, unsigned int t_cost, unsigned int m_cost,
        unsigned int repeat_count, unsigned int parallelism, unsigned int block_size,
        int clear_in, int return_memory, unsigned int **mem_ptr) {

    // Allocate memory
    uint32 blockLength = block_size/sizeof(uint32);
    uint32 numBlocks = m_cost*(1LL << 20)/(parallelism*blockLength*sizeof(uint32)) + 1;
    uint64 memLength = (uint64)numBlocks*blockLength*parallelism << t_cost;
    uint32 *mem = (uint32 *)malloc(memLength*sizeof(uint32));
    pthread_t *threads = (pthread_t *)malloc(parallelism*sizeof(pthread_t));
    struct ContextStruct *c = (struct ContextStruct *)malloc(parallelism*sizeof(struct ContextStruct));
    if(mem == NULL || threads == NULL || c == NULL) {
        fprintf(stderr, "Unable to allocate memory\n");
        return 0;
    }

    //printf("memLength:%llu numThreads:%u t_cost:%u repeat_count:%u blockLength:%u numBlocks:%u\n",
        //memLength, parallelism, t_cost, repeat_count, blockLength, numBlocks);
    // Compute intermediate key which is used to hash memory
    uint8 derivedSalt[saltlen];
    if(data != NULL && datalen != 0) {
        // Hash data into salt.  Treat data as a secret.
        PBKDF2_SHA256(data, datalen, salt, saltlen, 1, derivedSalt, saltlen);
    } else {
        memcpy(derivedSalt, salt, saltlen);
    }
    PBKDF2_SHA256(in, inlen, derivedSalt, saltlen, 1, out, outlen);
    if(clear_in) {
        // Note that an optimizer may drop this, and that data may leak through registers
        // or elsewhere.  The hand optimized version should try to deal with these issues
        memset(in, 0, inlen);
        if(data != NULL && datalen != 0) {
            memset(data, 0, datalen);
        }
    }

    // Using t_cost in this outer loop enables a hashed password to be strenthened in the
    // future by increasing the t_cost parameter, without knowing the password.
    uint32 i;
    for(i = 0; i <= t_cost; i++) {
        uint32 j;
        for(j = 0; j < repeat_count; j++) {
            // Launch threads.  Each hashes it's own separate block of memory, which improves performance.
            pthread_mutex_t countMutex;
            pthread_cond_t countVar;
            pthread_mutex_init(&countMutex, NULL);
            pthread_cond_init (&countVar, NULL);
            uint32 threadCount = parallelism;
            uint32 combinedValue = 0;
            uint32 t;
            for(t = 0; t < parallelism; t++) {
                c[t].id = t;
                c[t].mem = mem + (uint64)t*numBlocks*blockLength;
                c[t].numBlocks = numBlocks;
                c[t].blockLength = blockLength;
                c[t].countMutex = &countMutex;
                c[t].countVar = &countVar;
                c[t].threadCount = &threadCount;
                c[t].combinedValue = &combinedValue;
                int rc = pthread_create(&threads[t], NULL, hashMem, (void *)(c + t));
                if (rc){
                    fprintf(stderr, "Unable to start threads\n");
                    return 1;
                }
            }
            // Wait for threads to finish
            for(t = 0; t < parallelism; t++) {
                (void)pthread_join(threads[t], NULL);
            }
            pthread_mutex_destroy(&countMutex);
            pthread_cond_destroy(&countVar);
            xorIntoHash(parallelism, mem, out, outlen, blockLength, numBlocks);
        }

        // Double memory usage for the next loop.
        numBlocks <<= 1;
    }

    // Free memory.  The optimized version should try to insure that memory is cleared.
    //free(threads);
    //free(threadKeys);
    //free(c);
    if(return_memory) {
        *mem_ptr = mem;
    } else {
       // free(mem);
    }
    return 0;
}

// This is the prototype required for the password hashing competition.
// t_cost is an integer multiplier on CPU work.  m_cost is an integer number of MB of memory to hash.
int PHS(void *out, size_t outlen, const void *in, size_t inlen, const void *salt, size_t saltlen,
        unsigned int t_cost, unsigned int m_cost) {
    return NoelKDF(out, outlen, (void *)in, inlen, salt, saltlen, NULL, 0, t_cost, m_cost, 2048, 2, 4096, 0, 0, NULL);
}

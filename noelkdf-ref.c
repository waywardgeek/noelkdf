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

struct ContextStruct {
    uint32 *mem;
    uint8 *threadKey;
    uint32 threadKeySize;
    const uint8 *salt;
    uint32 saltSize;
    uint32 blockLength;
    uint32 numBlocks;
};

struct RngState {
    uint32 z;
    uint32 w;
};

// Simple rand() function.
//     from: http://www.codeproject.com/Articles/25172/Simple-Random-Number-Generation
static inline uint32 Rand(struct RngState *s) {
    s->z = 36969 * (s->z & 65535) + (s->z >> 16);
    s->w = 18000 * (s->w & 65535) + (s->w >> 16);
    return (s->z << 16) + s->w;
}

// This is the function called by each thread.  It hashes a single continuous block of memory.
static void *hashMem(void *contextPtr) {
    struct ContextStruct *c = (struct ContextStruct *)contextPtr;
    struct RngState s = {c->mem[0], c->mem[1]};

    // Copy the thread key to the first block
    be32dec_vect(c->mem, c->threadKey, c->threadKeySize);
    uint32 i;
    for(i = c->threadKeySize; i < c->blockLength; i++) {
        c->mem[i] = Rand(&s);
    }

    uint32 *prevBlock = c->mem;
    uint32 *toBlock = c->mem + c->blockLength;
    uint32 *fromBlock;
    uint32 hash = 1;
    for(i = 1; i < c->numBlocks; i++) {
        uint64 dist = *prevBlock;
        uint64 distSquared = (dist*dist) >> 32;
        uint64 distCubed = (distSquared*dist) >> 32;
        dist = ((i-1)*distCubed) >> 32;
        //printf("dist:%llu\n", dist);
        fromBlock = c->mem + (uint64)c->blockLength*(i - 1 - (dist%i));
        uint32 j;
        for(j = 0; j < c->blockLength; j++) {
            hash = hash*(*prevBlock++ | 3) + *fromBlock++;
            *toBlock++ = hash;
            //printf("hash:%u\n", hash);
        }
    }
    pthread_exit(NULL);
}

// Use the resulting output hash, which does depend on the password, to hash a small
// percentage of the total memory.  This forces all the threads' memory to be loaded at
// once, and if there was an attack that worked based on pre-computation of addresses,
// this pass should kill it.
static void cheatKillerPass(uint8 *out, uint32 outSize, uint32 *mem, uint32 numBlocks,
        uint32 blockLength, uint32 killerFactor) {
    uint32 outPos = 0;
    uint32 hash = UINT32_MAX;
    uint32 i;
    for(i = 0; i < numBlocks/killerFactor; i++) {
        uint32 address = blockLength*(hash % numBlocks);
        uint32 j;
        for(j = 0; j < blockLength; j++) {
            uint32 value = mem[address++];
            hash += value;
            uint32 k;
            for(k = 0; k < sizeof(uint32); k++) {
                if(outPos == outSize) {
                    outPos = 0;
                }
                out[outPos++] ^= (uint8)(value >> 24);
                value <<= 8;
            }
        }
    }
}

// This version allows for some more options than PHS.  They are:
//  num_threads     - the number of threads to run in parallel
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
        unsigned int t_cost, unsigned int m_cost, unsigned int num_hash_rounds, unsigned int killer_factor,
        unsigned int repeat_count, unsigned int num_threads, unsigned int block_size, int clear_in,
        int return_memory, unsigned int **mem_ptr) {

    // Allocate memory
    uint32 blockLength = block_size/sizeof(uint32);
    uint32 numBlocks = m_cost*(1LL << 20)/(num_threads*blockLength*sizeof(uint32)) + 1;
    uint64 memLength = (uint64)numBlocks*blockLength*num_threads << t_cost;
    uint32 *mem = (uint32 *)malloc(memLength*sizeof(uint32));
    pthread_t *threads = (pthread_t *)malloc(num_threads*sizeof(pthread_t));
    uint8 *threadKeys = (uint8 *)malloc(num_threads*outlen);
    struct ContextStruct *c = (struct ContextStruct *)malloc(num_threads*sizeof(struct ContextStruct));
    if(mem == NULL || threads == NULL || threadKeys == NULL || c == NULL) {
        fprintf(stderr, "Unable to allocate memory\n");
        return 0;
    }

    //printf("memLength:%llu numThreads:%u t_cost:%u repeat_count:%u blockLength:%u numBlocks:%u\n",
        //memLength, num_threads, t_cost, repeat_count, blockLength, numBlocks);
    // Compute intermediate key which is used to hash memory
    PBKDF2_SHA256(in, inlen, salt, saltlen, num_hash_rounds, out, outlen);
    if(clear_in) {
        // Note that an optimizer may drop this, and that data may leak through registers
        // or elsewhere.  The hand optimized version should try to deal with these issues
        memset(in, '\0', inlen);
    }

    // Using t_cost in this outer loop enables a hashed password to be strenthened in the
    // future by increasing the t_cost parameter, without knowing the password.
    uint32 i;
    for(i = 0; i <= t_cost; i++) {
        uint32 j;
        for(j = 0; j < repeat_count; j++) {

            // Initialize the thread keys from the intermediate key
            PBKDF2_SHA256(out, outlen, salt, saltlen, 1, threadKeys, num_threads*outlen);

            // Launch threads.  Each hashes it's own separate block of memory, which improves performance.
            uint32 t;
            for(t = 0; t < num_threads; t++) {
                c[t].mem = mem + (uint64)t*numBlocks*blockLength;
                c[t].numBlocks = numBlocks;
                c[t].salt = salt;
                c[t].saltSize = saltlen;
                c[t].threadKey = threadKeys + t*outlen;
                c[t].threadKeySize = outlen;
                c[t].blockLength = blockLength;
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
            if(num_threads > 1) {
                cheatKillerPass(out, outlen, mem, numBlocks, blockLength, killer_factor);
            }
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
    return NoelKDF(out, outlen, (void *)in, inlen, salt, saltlen, t_cost, m_cost, 2048, 1000, 1, 2, 4096, 0, 0, NULL);
}

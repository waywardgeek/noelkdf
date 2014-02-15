#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <immintrin.h>
#include "sha256.h"
#include "tigerkdf.h"

struct TigerKDFContextStruct {
    uint32_t *mem;
    uint8_t *hash;
    uint32_t hashSize;
    uint32_t parallelism;
    uint32_t p;
    uint32_t blocklen;
    uint32_t numblocks;
    uint32_t repetitions;
};

// Print the state.
static void printState(uint32_t state[8]) {
    uint32_t i;
    for(i = 0; i < 8; i++) {
        printf("%u ", state[i]);
    }
    printf("\n");
}

// XOR the last hashed data from each parallel process into the result.
static void xorIntoHash(uint8_t *hash, uint32_t hashSize, uint32_t *mem, uint32_t blocklen,
        uint32_t numblocks, uint32_t parallelism) {
    uint8_t data[hashSize];
    uint32_t p;
    for(p = 0; p < parallelism; p++) {
        uint64_t pos = 2*(p+1)*numblocks*(uint64_t)blocklen - hashSize/sizeof(uint32_t);
        be32enc_vect(data, mem + pos, hashSize);
        uint32_t i;
        for(i = 0; i < hashSize; i++) {
            hash[i] ^= data[i];
        }
    }
}

// Compute the bit reversal of value.
static uint32_t bitReverse(uint32_t value, uint32_t mask) {
    uint32_t result = 0;
    while(mask != 1) {
        result = (result << 1) | (value & 1);
        value >>= 1;
        mask >>= 1;
    }
    return result;
}

// Convert a uint32_t[8] to two __m128i values.
static void convStateFromUint32ToM128i(uint32_t state[8], __m128i *v1, __m128i *v2) {
    *v1 = _mm_set_epi32(state[3], state[2], state[1], state[0]);
    *v2 = _mm_set_epi32(state[7], state[6], state[5], state[4]);
}

// Convert two __m128i to uint32_t[8].
static void convStateFromM128iToUint32(__m128i *v1, __m128i *v2, uint32_t state[8]) {
    state[0] = _mm_extract_epi32(*v1, 0);
    state[1] = _mm_extract_epi32(*v1, 1);
    state[2] = _mm_extract_epi32(*v1, 2);
    state[3] = _mm_extract_epi32(*v1, 3);
    state[4] = _mm_extract_epi32(*v2, 0);
    state[5] = _mm_extract_epi32(*v2, 1);
    state[6] = _mm_extract_epi32(*v2, 2);
    state[7] = _mm_extract_epi32(*v2, 3);
}

// Hash three blocks together with fast SSE friendly hash function optimized for high memory bandwidth.
static inline void hashBlocks(uint32_t state[8], uint32_t *mem, uint32_t blocklen, uint64_t fromAddr,
        uint64_t toAddr, uint32_t repetitions) {
    __m128i s1, s2;
    convStateFromUint32ToM128i(state, &s1, &s2);
    uint64_t prevAddr = toAddr - blocklen;
    __m128i *m = (__m128i *)mem;
    __m128i shiftRightVal = _mm_set_epi32(25, 25, 25, 25);
    __m128i shiftLeftVal = _mm_set_epi32(7, 7, 7, 7);
    uint32_t i;
    uint32_t r;
    for(r = 0; r < repetitions; r++) {
        for(i = 0; i < blocklen/8;) {
            s1 = _mm_add_epi32(s1, m[prevAddr/4+i]);
            s1 = _mm_xor_si128(s1, m[fromAddr/4+i]);
            // Rotate right 7
            s1 = _mm_or_si128(_mm_srl_epi32(s1, shiftRightVal), _mm_sll_epi32(s1, shiftLeftVal));
            m[toAddr/4+i] = s1;
            i++;
            s2 = _mm_add_epi32(s2, m[prevAddr/4+i]);
            s2 = _mm_xor_si128(s2, m[fromAddr/4+i]);
            // Rotate right 7
            s2 = _mm_or_si128(_mm_srl_epi32(s2, shiftRightVal), _mm_sll_epi32(s2, shiftLeftVal));
            m[toAddr/4+i] = s2;
            i++;
        }
    }
    convStateFromM128iToUint32(&s1, &s2, state);
    printState(state);
}

// Hash memory without doing any password dependent memory addressing to thwart cache-timing-attacks.
static void *hashWithoutPassword(void *contextPtr) {
    struct TigerKDFContextStruct *c = (struct TigerKDFContextStruct *)contextPtr;

    uint32_t *mem = c->mem;
    uint8_t *hash = c->hash;
    uint32_t hashSize = c->hashSize;
    uint32_t p = c->p;
    uint32_t blocklen = c->blocklen;
    uint32_t numblocks = c->numblocks;
    uint32_t repetitions = c->repetitions;

    uint64_t start = 2*p*(uint64_t)numblocks*blocklen;
    uint8_t threadKey[blocklen*sizeof(uint32_t)];
    uint8_t s[sizeof(uint32_t)];
    be32enc(s, p);
    H(threadKey, blocklen*sizeof(uint32_t), hash, hashSize, s, sizeof(uint32_t));
    be32dec_vect(mem + start, threadKey, blocklen*sizeof(uint32_t));
    uint32_t state[8] = {1, 1, 1, 1, 1, 1, 1, 1};
    uint32_t mask = 1;
    uint64_t toAddr = start + blocklen;
    uint32_t i;
    for(i = 1; i < numblocks; i++) {
        if(mask << 1 <= i) {
            mask = mask << 1;
        }
        uint32_t reversePos = bitReverse(i, mask);
        if(reversePos + mask < i) {
            reversePos += mask;
        }
        uint64_t fromAddr = start + (uint64_t)blocklen*reversePos;
        hashBlocks(state, mem, blocklen, fromAddr, toAddr, repetitions);
        toAddr += blocklen;
    }
    pthread_exit(NULL);
}

// Hash memory with dependent memory addressing to thwart TMTO attacks.
static void *hashWithPassword(void *contextPtr) {
    struct TigerKDFContextStruct *c = (struct TigerKDFContextStruct *)contextPtr;

    uint32_t *mem = c->mem;
    uint32_t parallelism = c->parallelism;
    uint32_t p = c->p;
    uint32_t blocklen = c->blocklen;
    uint32_t numblocks = c->numblocks;
    uint32_t repetitions = c->repetitions;

    uint64_t start = (2*p + 1)*(uint64_t)numblocks*blocklen;
    uint32_t state[8] = {1, 1, 1, 1, 1, 1, 1, 1};
    uint64_t toAddr = start;
    uint32_t i;
    for(i = 0; i < numblocks; i++) {
        uint64_t v = state[0];
        uint64_t v2 = v*v >> 32;
        uint64_t v3 = v*v2 >> 32;
        uint32_t distance = (i + numblocks - 1)*v3 >> 32;
        uint64_t fromAddr;
        if(distance < i) {
            fromAddr = start + (i - 1 - distance)*(uint64_t)blocklen;
        } else {
            uint32_t q = (p + i) % parallelism;
            uint32_t b = numblocks - 1 - (distance - i);
            fromAddr = (2*numblocks*q + b)*(uint64_t)blocklen;
        }
        hashBlocks(state, mem, blocklen, fromAddr, toAddr, repetitions);
        toAddr += blocklen;
    }
    pthread_exit(NULL);
}

// The TigerKDF password hashing function.  MemSize is in KiB.
bool TigerKDF(uint8_t *hash, uint32_t hashSize, uint32_t memSize, uint8_t startGarlic, uint8_t stopGarlic,
        uint32_t blockSize, uint32_t parallelism, uint32_t repetitions, bool skipLastHash) {
    uint64_t memlen = (1 << 10)*(uint64_t)memSize/sizeof(uint32_t);
    uint32_t blocklen = blockSize/sizeof(uint32_t);
    uint32_t numblocks = (memlen/(2*parallelism*blocklen)) << startGarlic;
    memlen = (2*parallelism*(uint64_t)numblocks*blocklen) << (stopGarlic - startGarlic);
    uint32_t *mem = (uint32_t *)aligned_alloc(32, memlen*sizeof(uint32_t));
    if(mem == NULL) {
        return false;
    }
    pthread_t *threads = (pthread_t *)malloc(parallelism*sizeof(pthread_t));
    if(threads == NULL) {
        return false;
    }
    struct TigerKDFContextStruct *c = (struct TigerKDFContextStruct *)malloc(
            parallelism*sizeof(struct TigerKDFContextStruct));
    if(c == NULL) {
        return false;
    }
    uint8_t i;
    for(i = startGarlic; i <= stopGarlic; i++) {
        uint32_t p;
        for(p = 0; p < parallelism; p++) {
            c[p].p = p;
            c[p].hash = hash;
            c[p].hashSize = hashSize;
            c[p].mem = mem;
            c[p].numblocks = numblocks;
            c[p].blocklen = blocklen;
            c[p].parallelism = parallelism;
            c[p].repetitions = repetitions;
            int rc = pthread_create(&threads[p], NULL, hashWithoutPassword, (void *)(c + p));
            if (rc){
                fprintf(stderr, "Unable to start threads\n");
                return false;
            }
        }
        for(p = 0; p < parallelism; p++) {
            (void)pthread_join(threads[p], NULL);
        }
        for(p = 0; p < parallelism; p++) {
            int rc = pthread_create(&threads[p], NULL, hashWithPassword, (void *)(c + p));
            if (rc){
                fprintf(stderr, "Unable to start threads\n");
                return false;
            }
        }
        for(p = 0; p < parallelism; p++) {
            (void)pthread_join(threads[p], NULL);
        }
        xorIntoHash(hash, hashSize, mem, blocklen, numblocks, parallelism);
        numblocks *= 2;
        if(i < stopGarlic || !skipLastHash) {
            H(hash, hashSize, hash, hashSize, &i, 1);
        }
    }
    free(mem);
    free(threads);
    return true;
}

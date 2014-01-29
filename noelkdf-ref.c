#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "sha256.h"
#include "noelkdf.h"


// Forward declarations
static bool NoelKDF(uint8 *hash, uint32 hashSize, uint32 memSize, uint32 startGarlic, uint32 stopGarlic,
        uint32 blockSize, uint32 parallelism, uint32 repetitions, bool printDieharderData);
static void hashWithoutPassword(uint32 p, uint32 *wordHash, uint32 hashlen, uint32 *mem,
        uint32 blocklen, uint32 numblocks);
static uint32 bitReverse(uint32 value, uint32 mask);
static void hashWithPassword(uint32 p, uint32 *mem, uint32 blocklen, uint32 numblocks,
        uint32 parallelism, uint32 value);
static void xorIntoHash(uint32 *wordHash, uint32 hashlen, uint32 *mem, uint32 blocklen,
        uint32 numblocks, uint32 parallelism);
static void dumpMemory(uint32 *mem, uint32 memorySize);


// This is the crytographically strong hash function.  Feel free to use any other hash here.
static void H(uint8 *hash, uint32 hashSize, uint8 *password, uint32 passwordSize, uint8 *salt,
        uint32 saltSize) {
    PBKDF2_SHA256(password, passwordSize, salt, saltSize, 1, hash, hashSize);
}

// A simple password hashing interface.  MemSize is in MB.
bool NoelKDF_SimpleHashPassword(uint8 *hash, uint32 hashSize, uint8 *password, uint32 passwordSize,
        uint8 *salt, uint32 saltSize, uint32 memSize) {
    H(hash, hashSize, password, passwordSize, salt, saltSize);
    return NoelKDF(hash, hashSize, memSize, 0, 0, 4096, 1, 1, false);
}

// The full password hashing interface.  MemSize is in MB.
bool NoelKDF_HashPassword(uint8 *hash, uint32 hashSize, uint8 *password, uint8 passwordSize,
        uint8 *salt, uint32 saltSize, uint32 memSize, uint32 garlic, uint8 *data, uint32 dataSize,
        uint32 blockSize, uint32 parallelism, uint32 repetitions, bool printDieharderData) {
    uint8 derivedSalt[hashSize];
    H(derivedSalt, saltSize, data, dataSize, salt, saltSize);
    H(hash, hashSize, password, passwordSize, derivedSalt, saltSize);
    return NoelKDF(hash, hashSize, memSize, 0, garlic, blockSize, parallelism, repetitions,
        printDieharderData);
}

// Update an existing password hash to a more difficult level of garlic.
bool NoelKDF_UpdatePasswordHash(uint8 *hash, uint32 hashSize, uint32 memSize, uint32 oldGarlic,
        uint32 newGarlic, uint32 blockSize, uint32 parallelism, uint32 repetitions) {
    return NoelKDF(hash, hashSize, memSize, oldGarlic, newGarlic, blockSize, parallelism,
            repetitions, false);
}

// The NoelKDF password hashing function.  MemSize is in MB.
static bool NoelKDF(uint8 *hash, uint32 hashSize, uint32 memSize, uint32 startGarlic, uint32 stopGarlic,
        uint32 blockSize, uint32 parallelism, uint32 repetitions, bool printDieharderData) {
    uint32 hashlen = hashSize/sizeof(uint32);
    uint32 wordHash[hashlen];
    be32dec_vect(wordHash, hash, hashSize);
    uint32 memlen = (1 << 20)*memSize/sizeof(uint32);
    uint32 blocklen = blockSize/sizeof(uint32);
    uint32 numblocks = memlen/(2*parallelism*blocklen);
    memlen = 2*parallelism*numblocks*blocklen;
    uint32 *mem = (uint32 *)malloc(memlen*sizeof(uint32));
    if(mem == NULL) {
        return false;
    }
    uint32 i;
    for(i = startGarlic; i <= stopGarlic; i++) {
        uint32 j;
        for(j = 0; i < repetitions; j++) {
            uint32 value = 0;
            uint32 p;
            for(p = 0; p < parallelism; p++) {
                hashWithoutPassword(p, wordHash, hashlen, mem, blocklen, numblocks);
                value += mem[2*p*numblocks*blocklen + blocklen-1];
            }
            for(p = 0; p < parallelism; p++) {
                hashWithPassword(p, mem, blocklen, numblocks, parallelism, value);
            }
            xorIntoHash(wordHash, hashlen, mem, blocklen, numblocks, parallelism);
        }
        numblocks *= 2;
    }
    be32enc_vect(hash, wordHash, hashSize);
    if(printDieharderData) {
        dumpMemory(mem, memSize);
    }
    free(mem);
    return true;
}

// Hash memory without doing any password dependent memory addressing to thwart cache-timing-attacks.
static void hashWithoutPassword(uint32 p, uint32 *wordHash, uint32 hashlen, uint32 *mem,
        uint32 blocklen, uint32 numblocks) {
    uint64 start = 2*p*(uint64)numblocks*blocklen;
    uint32 hashSize = hashlen*sizeof(uint32);
    uint8 threadKey[blocklen*sizeof(uint32)];
    uint8 hash[hashSize];
    be32enc_vect(hash, wordHash, hashSize);
    uint8 salt[sizeof(uint32)];
    be32enc(salt, p);
    H(threadKey, blocklen*sizeof(uint32), hash, hashSize, salt, sizeof(uint32));
    be32dec_vect(mem, threadKey, blocklen*sizeof(uint32));
    uint32 value = 1;
    uint32 mask = 1;
    uint64 prevAddr = start;
    uint64 toAddr = start + blocklen;
    uint32 i;
    for(i = 0; i < numblocks; i++) {
        if(mask << 1 <= i) {
            mask = mask << 1;
        }
        uint32 reversePos = bitReverse(i, mask);
        if(reversePos + mask < i) {
            reversePos += mask;
        }
        uint64 fromAddr = start + blocklen*reversePos;
        uint32 j;
        for(j = 0; j < blocklen; j++) {
            value = value*(mem[prevAddr++] | 3) + mem[fromAddr++];
            mem[toAddr++] = value;
        }
    }
}

// Compute the bit reversal of value.
static uint32 bitReverse(uint32 value, uint32 mask) {
    uint32 result = 0;
    while(mask != 1) {
        result = (result << 1) | (value & 1);
        value >>= 1;
        mask >>= 1;
    }
    return result;
}

// Hash memory with dependent memory addressing to thwart TMTO attacks.
static void hashWithPassword(uint32 p, uint32 *mem, uint32 blocklen, uint32 numblocks,
        uint32 parallelism, uint32 value) {
    uint32 startBlock = 2*p*numblocks + numblocks;
    uint64 start = (uint64)startBlock*blocklen;
    uint64 prevAddr = start - blocklen;
    uint64 toAddr = start;
    uint32 i;
    for(i = 0; i < numblocks; i++) {
        uint64 v = value;
        uint64 v2 = v*v >> 32;
        uint64 v3 = v*v2 >> 32;
        uint32 distance = (i + numblocks - 1)*v3 >> 32;
        uint64 fromAddr;
        if(distance < i) {
            fromAddr = start + (i - 1 - distance)*blocklen;
        } else {
            uint32 q = (p + i) % parallelism;
            uint32 b = numblocks - 1 - (distance - i);
            fromAddr = (2*numblocks*q + b)*(uint64)blocklen;
        }
        uint32 j;
        for(j = 0; j < blocklen; j++) {
            value = value*(mem[prevAddr++] | 3) + mem[fromAddr++];
            mem[toAddr++] = value;
        }
    }
}

// XOR the last hashed data from each parallel process into the result.
static void xorIntoHash(uint32 *wordHash, uint32 hashlen, uint32 *mem, uint32 blocklen,
        uint32 numblocks, uint32 parallelism) {
    uint32 p;
    for(p = 0; p < parallelism; p++) {
        uint64 pos = 2*(p+1)*numblocks*(uint64)blocklen - hashlen;
        uint32 i;
        for(i = 0; i < hashlen; i++) {
            wordHash[i] ^= mem[pos+i];
        }
    }
}

// This is the prototype required for the password hashing competition.
// t_cost is an integer multiplier on CPU work.  m_cost is an integer number of MB of memory to hash.
int PHS(void *out, size_t outlen, const void *in, size_t inlen, const void *salt, size_t saltlen,
        unsigned int t_cost, unsigned int m_cost) {
    return !NoelKDF_HashPassword(out, outlen, (void *)in, inlen, (void *)salt, saltlen, m_cost, 0,
        NULL, 0, 4096, 1, t_cost, false);
}

// Dump memory in dieharder input format.  We skip numToSkip KB of initial memory since it
// may not be very random yet.
static void dumpMemory(uint32 *mem, uint32 memorySize) {
    uint64 memoryLength = (1LL << 20)*memorySize/sizeof(uint32);
    uint64 numToSkip = memoryLength >> 3;
    printf("type: d\n"
        "count: %llu\n"
        "numbit: 32\n", (memoryLength - numToSkip));
    uint64 i;
    for(i = numToSkip; i < memoryLength; i++) {
        printf("%u\n", mem[i]);
    }
}


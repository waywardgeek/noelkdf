#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#include "noelkdf.h"

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;

static void usage(char *format, ...) {
    va_list ap;
    va_start(ap, format);
    vfprintf(stderr, (char *)format, ap);
    va_end(ap);
    fprintf(stderr, "\nUsage: noelkdf outlen password salt t_cost m_cost num_hash_rounds +\n"
        "        repeat_count num_threads page_size free_memory dump\n"
        "    outlen is the output derived key in bytes\n"
        "    t_cost is \"garlic\" that multiplies memory and CPU work by 2^t_cost\n"
        "        it is used for client independent hash updates\n"
        "    m_cost is the ammount of memory to use in MB\n"
        "    num_hash_rounds is the number of SHA256 hashes used to derive the intermediate key\n"
        "    repeat_count is a multiplier on the total number of times we hash\n"
        "    num_threads is the number of threads used in hashing\n"
        "    page_size is the length of memory hashed in the inner loop, in KB\n"
        "    when free_memory is non-zero, we free hashed memory before exiting\n"
        "    when dump is non-zero, write to stdout dieharder input.  Use like:\n"
        "        dieharder -a -g 202 -f foo\n");
    exit(1);
}

static uint32 readUint32(char **argv, uint32 xArg) {
    char *endPtr;
    char *p = argv[xArg];
    uint32 value = strtol(p, &endPtr, 0);
    if(*p == '\0' || *endPtr != '\0') {
        usage("Invalid integer for parameter %u", xArg);
    }
    return value;
}

// Read a 2-character hex byte.
static bool readHexByte(uint8 *dest, char *value) {
    char c = toupper((uint8)*value++);
    uint8 byte;
    if(c >= '0' && c <= '9') {
        byte = c - '0';
    } else if(c >= 'A' && c <= 'F') {
        byte = c - 'A' + 10;
    } else {
        return false;
    }
    byte <<= 4;
    c = toupper((uint8)*value);
    if(c >= '0' && c <= '9') {
        byte |= c - '0';
    } else if(c >= 'A' && c <= 'F') {
        byte |= c - 'A' + 10;
    } else {
        return false;
    }
    *dest = byte;
    return true;
}

static uint8 *readHexSalt(char *p, uint32 *saltLength) {
    uint32 length = strlen(p);
    if(length & 1) {
        usage("hex salt string must have an even number of digits.\n");
    }
    *saltLength = strlen(p) >> 1;
    uint8 *salt = malloc(*saltLength*sizeof(uint8));
    if(salt == NULL) {
        usage("Unable to allocate salt");
    }
    uint8 *dest = salt;
    while(*p != '\0' && readHexByte(dest++, p)) {
        p += 2;
    }
    return salt;
}

static char findHexDigit(
    uint8 value)
{
    if(value <= 9) {
        return '0' + value;
    }
    return 'A' + value - 10;
}

static void printHex(
    uint8 *values,
    uint32 size)
{
    uint8 value;
    while(size-- != 0) {
        value = *values++;
        putchar(findHexDigit((uint8)(0xf & (value >> 4))));
        putchar((uint8)findHexDigit(0xf & value));
    }
}

static void readArguments(int argc, char **argv, uint32 *derivedKeySize, char **password, uint32 *passwordSize,
        uint8 **salt, uint32 *saltSize, uint32 *garlic, uint32 *memorySize, uint32 *numHashRounds,
        uint32 *repeatCount, uint32 *numThreads, uint32 *pageSize,
        bool *freeMemory, bool *dump) {
    if(argc != 12) {
        usage("Incorrect number of arguments");
    }
    *derivedKeySize = readUint32(argv, 1);
    *password = argv[2];
    *passwordSize = strlen(*password);
    *salt = readHexSalt(argv[3], saltSize);
    *garlic = readUint32(argv, 4);
    *memorySize = readUint32(argv, 5); // Number of MB
    *numHashRounds = readUint32(argv, 6);
    *repeatCount = readUint32(argv, 7);
    *numThreads = readUint32(argv, 8);
    *pageSize = readUint32(argv, 9);
    *freeMemory = readUint32(argv, 10);
    *dump = readUint32(argv, 11);
}

// Verify the input parameters are reasonalble.
static void verifyParameters(uint32 garlic, uint64 memorySize, uint32
        derivedKeySize, uint32 saltSize, uint32 passwordSize,
        uint32 numHashRounds, uint32 repeatCount, uint32 numThreads, uint32 pageSize) {
    if(garlic > 16) {
        usage("Invalid hashing multipler");
    }
    if(memorySize > (1 << 20) || memorySize < 1) {
        usage("Invalid memory size");
    }
    if(derivedKeySize < 8 || derivedKeySize > (1 << 20)) {
        usage("Invalid derived key size");
    }
    if(saltSize > (1 << 16) || saltSize < 4) {
        usage("Invalid salt size");
    }
    if(passwordSize == 0) {
        usage("Invalid password size");
    }
    if(numHashRounds < 1) {
        usage("num_hash_rounds must be >= 1");
    }
    if(pageSize < 1) {
        usage("page_size must be >= 1");
    }
    if(repeatCount < 1) {
        usage("repeatCount must be >= 1");
    }
    if(numThreads < 1) {
        usage("num_threads must be >= 1");
    }
    while((pageSize & 1) == 0) {
        pageSize >>= 1;
    }
    if(pageSize != 1) {
        usage("page_size must be a power of 2");
    }
}

// Dump memory in dieharder input format.  We skip numToSkip KB of initial memory since it
// may not be very random yet.
static void dumpMemory(uint32 *mem, uint32 memorySize) {
    uint64 memoryLength = (1LL << 20)*memorySize/sizeof(uint32);
    uint32 numToSkip = memoryLength >> 3;
    printf("type: d\n"
        "count: %llu\n"
        "numbit: 32\n", (memoryLength - numToSkip));
    uint32 i;
    for(i = numToSkip; i < memoryLength; i++) {
        printf("%u\n", mem[i]);
    }
}

int main(int argc, char **argv) {
    uint32 memorySize;
    uint32 garlic, derivedKeySize, saltSize, passwordSize;
    uint32 numHashRounds, repeatCount, numThreads, pageSize;
    uint8 *salt;
    char *password;
    bool freeMemory, dump;
    readArguments(argc, argv, &derivedKeySize, &password, &passwordSize, &salt, &saltSize, &garlic, &memorySize,
        &numHashRounds, &repeatCount, &numThreads, &pageSize, &freeMemory, &dump);
    verifyParameters(garlic, memorySize, derivedKeySize, saltSize, passwordSize,
        numHashRounds, repeatCount, numThreads, pageSize);
    uint8 *derivedKey = (uint8 *)calloc(derivedKeySize, sizeof(uint8));
    uint32 *mem;
    if(!dump) {
        printf("garlic:%u memorySize:%u numHashRounds:%u repeatCount:%u numThreads:%u pageSize:%u\n", 
            garlic, memorySize, numHashRounds, repeatCount, numThreads, pageSize);
    }
    if(NoelKDF(derivedKey, derivedKeySize, password, passwordSize, salt, saltSize, garlic, memorySize,
            numHashRounds, repeatCount, numThreads, pageSize, true, dump || !freeMemory, &mem)) {
        fprintf(stderr, "Key stretching failed.\n");
        return 1;
    }
    if(!dump) {
        printHex(derivedKey, derivedKeySize);
        printf("\n");
    } else {
        dumpMemory(mem, memorySize);
    }
    memset(derivedKey, '\0', derivedKeySize*sizeof(uint8));
    free(derivedKey);
    return 0;
}

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
    fprintf(stderr, "\nUsage: noelkdf outlen password salt t_cost m_cost num_hash_rounds parallelism num_threads page_size\n"
        "    outlen is the output derived key in bytes\n"
        "    t_cost is an integer multiplier CPU work\n"
        "    m_cost is the ammount of memory to use in MB\n"
        "    num_hash_rounds is the number of SHA256 hashes used to derive the intermediate key\n"
        "    parallelism is the number of inner loops that can be run in parallel\n"
        "    num_threads is the number of threads used in hashing\n"
        "    page_size is the length of memory hashed in the inner loop, in KB\n");
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
        uint8 **salt, uint32 *saltSize, uint32 *cpuWorkMultiplier, uint32 *memorySize,
        uint32 *numHashRounds, uint32 *parallelism, uint32 *numThreads, uint32 *pageSize) {
    if(argc != 10) {
        usage("Incorrect number of arguments");
    }
    *derivedKeySize = readUint32(argv, 1);
    *password = argv[2];
    *passwordSize = strlen(*password);
    *salt = readHexSalt(argv[3], saltSize);
    *cpuWorkMultiplier = readUint32(argv, 4);
    *memorySize = readUint32(argv, 5); // Number of MB
    *numHashRounds = readUint32(argv, 6);
    *parallelism = readUint32(argv, 7);
    *numThreads = readUint32(argv, 8);
    *pageSize = readUint32(argv, 9);
}

// Verify the input parameters are reasonalble.
static void verifyParameters(uint32 cpuWorkMultiplier, uint64 memorySize, uint32
        derivedKeySize, uint32 saltSize, uint32 passwordSize,
        uint32 numHashRounds, uint32 parallelism, uint32 numThreads, uint32 pageSize) {
    if(cpuWorkMultiplier < 1 || cpuWorkMultiplier > (1 << 20)) {
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
    if(parallelism < 1 || ((1024*pageSize)/parallelism)*parallelism != 1024*pageSize) {
        usage("parallelism must divide page_size evenly");
    }
    if(numThreads < 1) {
        usage("num_threads must be >= 1");
    }
}

int main(int argc, char **argv) {
    uint32 memorySize;
    uint32 cpuWorkMultiplier, derivedKeySize, saltSize, passwordSize;
    uint32 numHashRounds, parallelism, numThreads, pageSize;
    uint8 *salt;
    char *password;
    readArguments(argc, argv, &derivedKeySize, &password, &passwordSize, &salt, &saltSize, &cpuWorkMultiplier, &memorySize,
        &numHashRounds, &parallelism, &numThreads, &pageSize);
    verifyParameters(cpuWorkMultiplier, memorySize, derivedKeySize, saltSize, passwordSize,
        numHashRounds, parallelism, numThreads, pageSize);
    uint8 *derivedKey = (uint8 *)calloc(derivedKeySize, sizeof(uint8));
    if(NoelKDF(derivedKey, derivedKeySize, password, passwordSize, salt, saltSize, cpuWorkMultiplier, memorySize,
            numHashRounds, parallelism, numThreads, pageSize, true)) {
        fprintf(stderr, "Key stretching failed.\n");
        return 1;
    }
    printHex(derivedKey, derivedKeySize);
    printf("\n");
    memset(derivedKey, '\0', derivedKeySize*sizeof(uint8));
    free(derivedKey);
    return 0;
}

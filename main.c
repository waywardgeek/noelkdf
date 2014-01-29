#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#include <getopt.h>
#include "noelkdf.h"

static void usage(char *format, ...) {
    va_list ap;
    va_start(ap, format);
    vfprintf(stderr, (char *)format, ap);
    va_end(ap);
    fprintf(stderr, "\nUsage: noelkdf-test [OPTIONS]\n"
        "    -h hashSize     -- The output derived key length in bytes\n"
        "    -p password     -- Set the password to hash\n"
        "    -s salt         -- Set the salt.  Salt must be in hexidecimal\n"
        "    -g garlic       -- Multiplies memory and CPU work by 2^garlic\n"
        "    -m memorySize   -- The ammount of memory to use in MB\n"
        "    -r repetitions  -- A multiplier on the total number of times we hash\n"
        "    -t parallelism  -- Parallelism parameter, typically the number of threads\n"
        "    -b blockSize    -- Memory hashed in the inner loop at once, in bytes\n"
        "    -d              -- Write to stdout dieharder input.  Use like:\n"
        "                       dieharder -a -g 202 -f foo\n");
    exit(1);
}

static uint32 readUint32(char flag, char *arg) {
    char *endPtr;
    char *p = arg;
    uint32 value = strtol(p, &endPtr, 0);
    if(*p == '\0' || *endPtr != '\0') {
        usage("Invalid integer for parameter -%c", flag);
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

int main(int argc, char **argv) {
    uint32 memorySize = 1024, garlic = 0, derivedKeySize = 32;
    uint32 repetitions = 1, parallelism = 1, blockSize = 4096;
    uint8 *salt = (uint8 *)"salt";
    uint32 saltSize = sizeof(salt);
    uint8 *password = (uint8 *)"password";
    uint32 passwordSize = sizeof(password);
    bool dump = false;

    char c;
    while((c = getopt(argc, argv, "h:p:s:g:m:r:t:b:d")) != -1) {
        switch (c) {
        case 'h':
            derivedKeySize = readUint32(c, optarg);
            break;
        case 'p':
            password = (uint8 *)optarg;
            passwordSize = strlen(optarg);
            break;
        case 's':
            salt = readHexSalt(optarg, &saltSize);
            break;
        case 'g':
            garlic = readUint32(c, optarg);
            break;
        case 'm':
            memorySize = readUint32(c, optarg);
            break;
        case 'r':
            repetitions = readUint32(c, optarg);
            break;
        case 't':
            parallelism = readUint32(c, optarg);
            break;
        case 'b':
            blockSize = readUint32(c, optarg);
            break;
        case 'd':
            dump = true;
            break;
        default:
            usage("Invalid argumet");
        }
    }
    if(optind != argc) {
        usage("Extra parameters not recognised\n");
    }

    if(!dump) {
        printf("garlic:%u memorySize:%u repetitions:%u numThreads:%u blockSize:%u\n", 
            garlic, memorySize, repetitions, parallelism, blockSize);
    }
    uint8 *derivedKey = (uint8 *)calloc(derivedKeySize, sizeof(uint8));
    if(!NoelKDF_HashPassword(derivedKey, derivedKeySize, password, passwordSize, salt, saltSize,
            memorySize, garlic, NULL, 0, blockSize, parallelism, repetitions, dump)) {
        fprintf(stderr, "Key stretching failed.\n");
        return 1;
    }
    if(!dump) {
        printHex(derivedKey, derivedKeySize);
        printf("\n");
    }
    return 0;
}

#include <stdbool.h>

// This is the prototype required for the password hashing competition.
// t_cost is an integer multiplier on CPU work.  m_cost is an integer number of MB of memory to hash.
int PHS(void *out, size_t outlen, const void *in, size_t inlen, const void *salt, size_t saltlen,
    unsigned int t_cost, unsigned int m_cost);

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;

// A simple password hashing interface.  MemSize is in MB.
bool NoelKDF_SimpleHashPassword(uint8 *hash, uint32 hashSize, uint8 *password, uint32 passwordSize,
        uint8 *salt, uint32 saltSize, uint32 memSize);

// The full password hashing interface.  MemSize is in MB.
bool NoelKDF_HashPassword(uint8 *hash, uint32 hashSize, uint8 *password, uint8 passwordSize,
        uint8 *salt, uint32 saltSize, uint32 memSize, uint8 garlic, uint8 *data, uint32 dataSize,
        uint32 blockSize, uint32 parallelism, uint32 repetitions, bool printDieharderData);

// Update an existing password hash to a more difficult level of garlic.
bool NoelKDF_UpdatePasswordHash(uint8 *hash, uint32 hashSize, uint32 memSize, uint8 oldGarlic,
        uint8 newGarlic, uint32 blockSize, uint32 parallelism, uint32 repetitions);

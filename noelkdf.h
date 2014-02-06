#include <stdint.h>
#include <stdbool.h>

// This is the prototype required for the password hashing competition.
// t_cost is an integer multiplier on CPU work.  m_cost is an integer number of MiB of memory to hash.
int PHS(void *out, size_t outlen, const void *in, size_t inlen, const void *salt, size_t saltlen,
    unsigned int t_cost, unsigned int m_cost);

// A simple password hashing interface.  MemSize is in MiB.
bool NoelKDF_SimpleHashPassword(uint8_t *hash, uint32_t hashSize, uint8_t *password, uint32_t passwordSize,
        uint8_t *salt, uint32_t saltSize, uint32_t memSize);

// The full password hashing interface.  MemSize is in MiB.
bool NoelKDF_HashPassword(uint8_t *hash, uint32_t hashSize, uint8_t *password, uint8_t passwordSize,
        uint8_t *salt, uint32_t saltSize, uint32_t memSize, uint8_t garlic, uint8_t *data, uint32_t dataSize,
        uint32_t blockSize, uint32_t parallelism, uint32_t repetitions);

// Update an existing password hash to a more difficult level of garlic.
bool NoelKDF_UpdatePasswordHash(uint8_t *hash, uint32_t hashSize, uint32_t memSize, uint8_t oldGarlic,
        uint8_t newGarlic, uint32_t blockSize, uint32_t parallelism, uint32_t repetitions);

// Client-side portion of work for server-relief mode.
bool NoelKDF_ClientHashPassword(uint8_t *hash, uint32_t hashSize, uint8_t *password, uint8_t passwordSize,
        uint8_t *salt, uint32_t saltSize, uint32_t memSize, uint8_t garlic, uint8_t *data, uint32_t dataSize,
        uint32_t blockSize, uint32_t parallelism, uint32_t repetitions);

// Server portion of work for server-relief mode.
void NoelKDF_ServerHashPassword(uint8_t *hash, uint32_t hashSize, uint8_t garlic);

// Raw interface to NoelKDF.
bool NoelKDF(uint8_t *hash, uint32_t hashSize, uint32_t memSize, uint8_t startGarlic, uint8_t stopGarlic,
    uint32_t blockSize, uint32_t parallelism, uint32_t repetitions, bool skipLastHash);

// PBKDF2 based hash function.  Uses PBKDF2-SHA256 by default.
void H(uint8_t *hash, uint32_t hashSize, uint8_t *password, uint32_t passwordSize, uint8_t *salt, uint32_t saltSize);

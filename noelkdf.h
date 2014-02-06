#include <stdint.h>
#include <stdbool.h>

// PBKDF2 based hash function.  Uses PBKDF2-SHA256 by default.
void H(uint8_t *hash, uint32_t hashSize, uint8_t *password, uint32_t passwordSize, uint8_t *salt, uint32_t saltSize);

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

// Hash the password as a Halting Password Puzzle.  Memory is initially 1MiB, and
// increased by 2X until we see the expected hash result.  QuitGarlic enables us to give
// up without the user halting the process manually.  Return the matching garlic level if
// we succeed, and -1 - last garlic tried if we fail.  If the return value >= 0, the
// password is correct.  Otherwise last garlic level successfully tried is subtracted from
// -1 is returned.
int NoelKDF_HaltingPasswordPuzzle(uint8_t *expectedHash, uint32_t hashSize, uint8_t *password, uint8_t passwordSize,
        uint8_t *salt, uint32_t saltSize, uint8_t quitGarlic, uint8_t *data, uint32_t dataSize);

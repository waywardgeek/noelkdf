#include <stdbool.h>

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;

#define NUM_THREADS 2
#define PAGE_LENGTH ((16*1024)/sizeof(uint64)) // 16KB
#define THREAD_KEY_SIZE 256 // In bytes

// This is the prototype required for the password hashing competition.
// t_cost is an integer multiplier on CPU work.  m_cost is an integer number of MB of memory to hash.
int PHS(void *out, size_t outlen, const void *in, size_t inlen, const void *salt, size_t saltlen,
    unsigned int t_cost, unsigned int m_cost);

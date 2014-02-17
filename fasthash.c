#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <immintrin.h>
#include "pbkdf2.h"

void SHA256_Uint8(const uint8_t input[32], uint8_t hash[32]) {
  SHA256_CTX ctx;
  SHA256_Init(&ctx);
  SHA256_Update(&ctx, input, 32);
  SHA256_Final(hash, &ctx);
}

void SHA256_Uint32(uint32_t hash[8]) {
    uint8_t in[32], out[32];
    be32enc_vect(in, hash, 32);
    SHA256_Uint8(in, out);
    be32dec_vect(hash, out, 32);
}

int main(int argc, char **argv) {
    uint64_t memlen = (1u << 31)/sizeof(uint32_t);
    uint32_t blocklen = 32768/sizeof(uint32_t);
    uint32_t parallelism = blocklen/8;
    uint32_t repetitions = 1;
    char c;
    while((c = getopt(argc, argv, "p:m:r:b:")) != -1) {
        switch (c) {
        case 'p':
            parallelism = atol(optarg);
            break;
        case 'm':
            memlen = atol(optarg)/sizeof(uint32_t);
            break;
        case 'r':
            repetitions = atol(optarg);
            break;
        case 'b':
            blocklen = atol(optarg)/sizeof(uint32_t);
            break;
        default:
            printf("Invalid argumet");
            return 1;
        }
    }
    uint32_t numblocks = memlen/blocklen;
    if(parallelism*8 > blocklen) {
        printf("parallelism must be <= blocksize/32\n");
        return 1;
    }

    uint32_t *mem = aligned_alloc(32, memlen*sizeof(uint32_t));
    uint32_t i;
    for(i = 0; i < blocklen; i++) {
        mem[i] = i;
    }
    uint32_t vs = 1;
    uint32_t mask = (blocklen-1) & ~(4*parallelism-1);
    __m128i vm1 = _mm_set_epi32(vs, vs, vs, vs);
    __m128i vm2 = _mm_set_epi32(vs, vs, vs, vs);
    uint32_t r;
    for(r = 0; r < repetitions; r++) {
        __m128i *tm = (__m128i *)(mem + blocklen);
        uint32_t *ps = mem;
        for(i = 1; i < numblocks; i++) {
            uint32_t fromAddr = vs % i;
            uint32_t *fs = mem + fromAddr*blocklen;
            __m128i *fm = (__m128i *)fs;
            uint32_t j;
            for(j = 0; j < blocklen; j += (parallelism << 3)) {
                uint32_t *psl = ps + (vs & mask);
                //uint32_t *psl = ps + j;
                __m128i *pml = (__m128i *)psl;
                uint32_t k;
                for(k = 0; k < parallelism; k++) {
                    __m128i p = _mm_load_si128(pml++);
                    __m128i f = _mm_load_si128(fm++);
                    vm1 = _mm_add_epi32(vm1, p);
                    vm1 = _mm_xor_si128(vm1, f);
                    _mm_store_si128(tm++, vm1);
                    vs = vs * (*fs | 3) + *psl;
                    vs = vs * (*psl | 3) + *fs;
                    psl += 4;
                    fs += 4;
                    pml = (__m128i *)psl;
                    p = _mm_load_si128(pml++);
                    f = _mm_load_si128(fm++);
                    vm2= _mm_add_epi32(vm2, p);
                    vm2= _mm_xor_si128(vm2, f);
                    _mm_store_si128(tm++, vm2);
                    vs = vs * (*fs | 3) + *psl;
                    vs = vs * (*psl | 3) + *fs;
                    psl += 4;
                    fs += 4;
                }
            }
            ps += blocklen;
            if((i & 0x3f) == 0) {
                uint32_t *data = ps - 8;
                SHA256_Uint32(data);
                vm1 = _mm_load_si128(tm - 2);
                vm2 = _mm_load_si128(tm - 1);
            }
            __m128i t = _mm_set_epi32(vs, vs, vs, vs);
            vm1 = _mm_xor_si128(vm1, t);
            vm2 = _mm_xor_si128(vm2, t);
        }
    }
    printf("%u\n", mem[rand() % memlen]);
    return 0;
}

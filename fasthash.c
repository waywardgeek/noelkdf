#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <immintrin.h>

#define memlen ((1u << 31)/sizeof(uint32_t))
#define blocklen (4096/sizeof(uint32_t))
#define numblocks (memlen/blocklen)

int main() {
    uint32_t *mem = aligned_alloc(32, memlen*sizeof(uint32_t));
    uint32_t i;
    for(i = 0; i < blocklen; i++) {
        mem[i] = i;
    }
    uint32_t vs = 1;
    uint32_t mask = (blocklen-1) & ~7;
    __m128i vm = _mm_set_epi32(vs, vs, vs, vs);
    uint32_t r;
    for(r = 0; r < 1; r++) {
        __m128i *tm = (__m128i *)(mem + blocklen);
        uint32_t *ps = mem;
        for(i = 1; i < numblocks; i++) {
            uint32_t fromAddr = vs % i;
            uint32_t *fs = mem + fromAddr*blocklen;
            __m128i *fm = (__m128i *)fs;
            uint32_t j;
            for(j = 0; j < blocklen; j += 8) {
                uint32_t *psl = ps + (vs & mask);
                //uint32_t *psl = ps + j;
                __m128i *pml = (__m128i *)psl;
                __m128i p = _mm_load_si128(pml++);
                __m128i f = _mm_load_si128(fm++);
                vm = _mm_add_epi32(vm, p);
                vm = _mm_xor_si128(vm, f);
                _mm_store_si128(tm++, vm);
                vs = vs * (*fs | 3) + *psl;
                vs = vs * (*psl | 3) + *fs;
                psl += 4;
                fs += 4;
                p = _mm_load_si128(pml);
                f = _mm_load_si128(fm++);
                vm = _mm_add_epi32(vm, p);
                vm = _mm_xor_si128(vm, f);
                _mm_store_si128(tm++, vm);
                vs = vs * (*ps | 3) + *fs;
                vs = vs * (*fs | 3) + *ps;
                fs += 4;
            }
            __m128i t = _mm_set_epi32(vs, vs, vs, vs);
            vm = _mm_xor_si128(vm, t);
        }
    }
    printf("%u\n", mem[rand() % memlen]);
    return 0;
}

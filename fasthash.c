#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <immintrin.h>

int main(int argc, char **argv) {
    uint32_t parallelism = 2;
    uint64_t memlen = (1u << 31)/sizeof(uint32_t);
    uint32_t blocklen = 4096/sizeof(uint32_t);
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
    if(parallelism*4 > blocklen) {
        printf("parallelism must be <= blocksize/16\n");
        return 1;
    }

    uint32_t *mem = aligned_alloc(32, memlen*sizeof(uint32_t));
    uint32_t i;
    for(i = 0; i < blocklen; i++) {
        mem[i] = i;
    }
    uint32_t vs = 1;
    uint32_t mask = (blocklen-1) & ~(4*parallelism-1);
    __m128i vm = _mm_set_epi32(vs, vs, vs, vs);
    uint32_t r;
    for(r = 0; r < repetitions; r++) {
        __m128i *tm = (__m128i *)(mem + blocklen);
        uint32_t *ps = mem;
        for(i = 1; i < numblocks; i++) {
            uint32_t fromAddr = vs % i;
            uint32_t *fs = mem + fromAddr*blocklen;
            __m128i *fm = (__m128i *)fs;
            uint32_t j;
            for(j = 0; j < blocklen; j += (parallelism << 2)) {
                uint32_t *psl = ps + (vs & mask);
                //uint32_t *psl = ps + j;
                uint32_t k;
                for(k = 0; k < parallelism; k++) {
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
                }
            }
            ps += blocklen;
            __m128i t = _mm_set_epi32(vs, vs, vs, vs);
            vm = _mm_xor_si128(vm, t);
        }
    }
    printf("%u\n", mem[rand() % memlen]);
    return 0;
}

#include <stdlib.h>
#include <math.h>
#include "pedatabase.h"

#define MEM_LENGTH 10000000LL
#define NUM_PEBBLES (MEM_LENGTH/4)
#define NUM_SAMPLES 100LL

peRoot peTheRoot;
peLocationArray peVisitedLocations;

/*
// Find the previous position along the logarithmic chains.  Basically, let n = the number
// of 1's from the LSB until we see the first 0, and return 2^(n+1).
static uint32 findPrevLogarithmicChainPos(uint32 pos) {
    uint32 row = 0;
    uint32 t = pos;
    while(t != 1) {
        t >>= 1;
        row++;
    }
    uint32 dist = 2 << (row/4);
    t = pos;
    do {
        t >>= 1;
        dist <<= 1;
    } while(t & 1);
    if(dist < pos) {
        return pos - dist;
    }
    return 0;
}

// Find the previous position in the safe-dating tree.  If the root is labeled 1, and then
// left edge of the tree is 2^r. For row r starting with 0, for a node at position x,
// we have row = floor(log2(x)).  Let r = 2^row.  If x < 3*r/2, then prevPos = r/2 + x-r = x - r/2.
// Otherwise, prevPos = r/2 + x - 3*r/2 = x - r.  However, we actually use addr = 0 for
// the root node, and the left edge is 0, 2, 6, 14..., where x == addr/2 + 1.
static uint32 findPrevSafeDatingPos(uint32 pos) {
    uint32 x = pos/2 + 1;
    uint32 row = 0;
    uint32 t = x;
    while(t != 1) {
        t >>= 1;
        row++;
    }
    uint32 r = 1 << row;
    uint32 prevPos;
    if(x < 3*r/2) {
        prevPos = x - r/2;
    } else {
        prevPos = x - r;
    }
    return (prevPos - 1)*2;
}
*/

// Find the previous position to point to.
static uint32 findPrevPos(uint32 pos) {
    if(pos <= 2) {
        return 0;
    }
    double dist = (rand()/(double)RAND_MAX);
    dist = dist*dist*dist;
    return pos - 1 - (uint32)((pos-1)*dist);
    /*
    if(pos & 1) {
        // logarithmic chains
        return findPrevLogarithmicChainPos(pos);
    }
    return pos - (rand() % pos)/2 - 1;
    // Safe dating tree
    return findPrevSafeDatingPos(pos);
    */
}

// Find the size of the uncovered DAG starting at pos.
static uint32 findDAGSize(uint32 pos) {
    peLocation location = peRootGetiLocation(peTheRoot, pos);
    pePebble pebble = peLocationGetPebble(location);
    if(pebble != pePebbleNull) {
        return 0;
    }
    if(peLocationVisited(location)) {
        return 0;
    }
    uint32 numPebbles = 1;
    peLocationSetVisited(location, true);
    peLocationArrayAppendLocation(peVisitedLocations, location);
    if(pos > 0) {
        // Previous guy pointer
        numPebbles += findDAGSize(pos - 1);
        uint32 prevPos = findPrevPos(pos);
        // printf("%u\n", prevPos);
        numPebbles += findDAGSize(prevPos);
    }
    return numPebbles;
}

// Find the average size of the sub-DAG that has to be recomputed when I randomly ask for
// values from memory.
static void computeAveragePenalty(void) {
    uint64 total = 0;
    uint32 i;
    for(i = 0; i < NUM_SAMPLES; i++) {
        uint32 pos = rand() % MEM_LENGTH;
        total += findDAGSize(pos);
        peLocation location;
        peForeachLocationArrayLocation(peVisitedLocations, location) {
            peLocationSetVisited(location, false);
        } peEndLocationArrayLocation;
        peLocationArraySetUsedLocation(peVisitedLocations, 0);
    }
    printf("Average recalculation is %.4f%%\n", total*100.0/(MEM_LENGTH*NUM_SAMPLES));
}

// Set the number of pointers to each memory location.
static void setNumPointers(void) {
    uint32 i;
    for(i = 0; i < MEM_LENGTH; i++) {
        uint32 prevPos = findPrevPos(i);
        peLocation location = peRootGetiLocation(peTheRoot, prevPos);
        peLocationSetNumPointers(location, peLocationGetNumPointers(location) + 1);
    }
}

// Compute the cut size at the middle.
static void computeCut(void) {
    uint32 cutSize = 0;
    uint32 i;
    for(i = MEM_LENGTH/2; i < MEM_LENGTH-1; i++) {
        if(findPrevPos(i) < MEM_LENGTH/2) {
            cutSize++;
        }
    }
    printf("Mid-cut size: %u (%.1f%%)\n", cutSize, (cutSize*100.0)/MEM_LENGTH);
}

// For now, just randomly distribute pebbles.
static void randomlyDistributePebbles(void) {
    uint32 total = 0;
    uint32 xPebble;
    for(xPebble = 0; xPebble < MEM_LENGTH; xPebble++) {
        //peLocation location = peRootGetiLocation(peTheRoot, xPebble);
        //if((rand() % (MEM_LENGTH/NUM_PEBBLES)) == 0) {
        //if((xPebble % (MEM_LENGTH/NUM_PEBBLES)) == 0) {
        //if(peLocationGetNumPointers(location) >= 3 || (xPebble % 8) == 0) {
        uint32 prevPos = findPrevPos(xPebble);
        if(xPebble - prevPos < 20000) {
            peLocation location = peRootGetiLocation(peTheRoot, prevPos);
            if(peLocationGetPebble(location) == pePebbleNull) {
                pePebble pebble = pePebbleAlloc();
                peLocationInsertPebble(location, pebble);
                total++;
            }
        }
        if(xPebble % 8 == 0) {
            peLocation location = peRootGetiLocation(peTheRoot, xPebble);
            pePebble pebble = pePebbleAlloc();
            peLocationInsertPebble(location, pebble);
        }
    }
    printf("Total pebbles: %u out of %llu (%.1f%%)\n", total, MEM_LENGTH, total*100.0/MEM_LENGTH);
}

// Dump the graph.
static void dumpGraph(void) {
    uint32 pos;
    for(pos = 0; pos < MEM_LENGTH; pos++) {
        peLocation location = peRootGetiLocation(peTheRoot, pos);
        char c = ' ';
        if(peLocationGetPebble(location) != pePebbleNull) {
            c = '*';
        }
        printf("%c n%u -> n%u\t%u pointers\n", c, pos, findPrevPos(pos), peLocationGetNumPointers(location));
    }
}

int main(int argc, char **argv) {
    utStart();
    peDatabaseStart();
    peTheRoot = peRootAlloc();
    peVisitedLocations = peLocationArrayAlloc();
    peRootAllocLocations(peTheRoot, MEM_LENGTH);
    uint32 i;
    for(i = 0; i < MEM_LENGTH; i++) {
        peLocation location = peLocationAlloc();
        peRootSetiLocation(peTheRoot, i, location);
    }
    setNumPointers();
    randomlyDistributePebbles();
    if(argc == 2 && !strcmp(argv[1], "-d")) {
        dumpGraph();
    }
    computeAveragePenalty();
    computeCut();
    peDatabaseStop();
    utStop(true);
    return 0;
}

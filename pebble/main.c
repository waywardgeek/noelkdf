#include <stdlib.h>
#include "pedatabase.h"

#define MEM_LENGTH 400000LL
#define NUM_PEBBLES (MEM_LENGTH/8)
#define NUM_SAMPLES 100

peRoot peTheRoot;
peLocationArray peVisitedLocations;

// For now, just randomly distribute pebbles.
static void randomlyDistributePebbles(void) {
    uint32 xPebble;

    for(xPebble = 0; xPebble < MEM_LENGTH; xPebble++) {
        if((rand() % (MEM_LENGTH/NUM_PEBBLES)) == 0) {
            pePebble pebble = pePebbleAlloc();
            peLocation location = peRootGetiLocation(peTheRoot, xPebble);
            peLocationInsertPebble(location, pebble);
        }
    }
}

// Find the previous position along the logarithmic chains.  Basically, let n = the number
// of 1's from the LSB until we see the first 0, and return 2^(n+1).
static uint32 findPrevLogarithmicChainPos(uint32 pos) {
    uint32 dist = 2;
    uint32 t = pos;
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

// Find the previous position to point to.
static uint32 findPrevPos(uint32 pos) {
    if(pos & 1) {
        // logarithmic chains
        return findPrevLogarithmicChainPos(pos);
    }
    // Safe dating tree
    return findPrevSafeDatingPos(pos);
}

// Find the size of the uncovered DAG starting at pos.
static uint32 findDAGSize(uint32 pos) {
    peLocation location = peRootGetiLocation(peTheRoot, pos);
    pePebble pebble = peLocationGetPebble(location);
    if(pebble != pePebbleNull || peLocationVisited(location)) {
        return 0;
    }
    uint32 numPebbles = 1;
    peLocationSetVisited(location, true);
    peLocationArrayAppendLocation(peVisitedLocations, location);
    if(pos > 0) {
        // Previous guy pointer
        numPebbles += findDAGSize(pos - 1);
        uint32 prevPos = findPrevPos(pos);
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
    printf("Average recalculation is %.2f%%\n", total*100.0/(MEM_LENGTH*NUM_SAMPLES));
}

// Dump the graph.
static void dumpGraph(void) {
    uint32 pos;
    printf("n0\n");
    for(pos = 1; pos < MEM_LENGTH; pos++) {
        char c = ' ';
        if(peLocationGetPebble(peRootGetiLocation(peTheRoot, pos)) != pePebbleNull) {
            c = '*';
        }
        printf("%c n%u -> n%u\n", c, pos, findPrevPos(pos));
    }
}

int main() {
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
    randomlyDistributePebbles();
    //dumpGraph();
    computeAveragePenalty();
    peDatabaseStop();
    utStop(true);
    return 0;
}

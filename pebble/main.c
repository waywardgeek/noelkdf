#include <stdlib.h>
#include <math.h>
#include "pedatabase.h"

#define MEM_LENGTH 1000LL

peRoot peTheRoot;
peLocationArray peVisitedLocations;

typedef enum {
    SLIDING_WINDOW,
    RAND_CUBED
} peGraphType;

// Find the previous position using Alexander's sliding power-of-two window.
static uint32 findSlidingWindowPos(uint32 pos) {
    // This is a sliding window which is the largest power of 2 < i.
    uint32 mask = 1;
    while(mask < pos) {
        mask <<= 1;
    }
    mask = (mask >> 1) - 1;
    return pos - (rand() & mask);
}

// Find the previous position using a uniform random value between 0..1, cube it, and go
// that * position back.
static uint32 findRandCubedPos(uint32 pos) {
    double dist = (rand()/(double)RAND_MAX);
    dist = dist*dist*dist;
    return pos - 1 - (uint32)((pos-1)*dist);
}

// Find the previous position to point to.
static void setPrevLocation(uint32 pos, peGraphType type) {
    peLocation location = peRootGetiLocation(peTheRoot, pos);
    if(pos <= 1) {
        return;
    }
    uint32 prevPos;
    switch(type) {
    case SLIDING_WINDOW: prevPos = findSlidingWindowPos(pos); break;
    case RAND_CUBED: prevPos = findRandCubedPos(pos); break;
    default:
        printf("Unknown graph type\n");
        exit(1);
    }
    peLocation prevLocation = peRootGetiLocation(peTheRoot, prevPos);
    peLocationInsertLocation(prevLocation, location);
}

// Find the previous position to point to.
static uint32 findPrevPos(uint32 pos) {
    if(pos <= 1) {
        return 0;
    }
    return peLocationGetRootIndex(peLocationGetLocation(peRootGetiLocation(peTheRoot, pos)));
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
        // printf("%u\n", prevPos);
        uint32 prevPos = findPrevPos(pos);
        numPebbles += findDAGSize(prevPos);
        // Previous guy pointer
        numPebbles += findDAGSize(pos - 1);
    }
    return numPebbles;
}

// Find the average size of the sub-DAG that has to be recomputed when I randomly ask for
// values from memory.
static void computeAveragePenalty(void) {
    uint64 total = 0;
    uint32 mask = 1;
    uint32 nextMarker = MEM_LENGTH/2;
    uint32 i;
    for(i = 0; i < MEM_LENGTH; i++) {
        if(i > 1) {
            if(i == nextMarker) {
                mask++;
                nextMarker = i + (MEM_LENGTH - i)/2;
                printf("At %u, increased spacing to %u\n", i, mask);
            }
            uint32 prevPos = findPrevPos(i);
            total += findDAGSize(prevPos);
            peLocation location;
            peForeachLocationArrayLocation(peVisitedLocations, location) {
                peLocationSetVisited(location, false);
            } peEndLocationArrayLocation;
            peLocationArraySetUsedLocation(peVisitedLocations, 0);
        }
        //if(i < 50*MEM_LENGTH/100 || i % (1 << MEM_LENGTH/(MEM_LENGTH - i) == 0) {
        if(i % mask == 0) {
            peLocation location = peRootGetiLocation(peTheRoot, i);
            pePebble pebble = pePebbleAlloc();
            peLocationInsertPebble(location, pebble);
        }
    }
    uint32 numPebbles = 0;
    for(i = 0; i < MEM_LENGTH; i++) {
        if(peLocationGetPebble(peRootGetiLocation(peTheRoot, i)) != pePebbleNull) {
            numPebbles++;
        }
    }
    printf("Total pebbles: %f%%\n", numPebbles*100.0/MEM_LENGTH);
    printf("Recalculation penalty is %fX\n", (total + MEM_LENGTH)/(double)MEM_LENGTH);
}

// Set the number of pointers to each memory location.
static void setNumPointers(void) {
    uint32 i;
    for(i = 1; i < MEM_LENGTH; i++) {
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
        peLocation location = peRootGetiLocation(peTheRoot, xPebble);
        if(xPebble < 99*MEM_LENGTH/100 || peLocationGetNumPointers(location) >= 100 || (xPebble % 2) == 0) {
            pePebble pebble = pePebbleAlloc();
            peLocationInsertPebble(location, pebble);
            total++;
        }
        // Distribution covering nodes pointed to by short edges
        uint32 prevPos = findPrevPos(xPebble);
        if(xPebble - prevPos < 200) {
            peLocation location = peRootGetiLocation(peTheRoot, prevPos);
            if(peLocationGetPebble(location) == pePebbleNull) {
                pePebble pebble = pePebbleAlloc();
                peLocationInsertPebble(location, pebble);
                total++;
            }
        }
        /*
        // Simple uniform distribution
        if(xPebble % 8 == 0) {
            peLocation location = peRootGetiLocation(peTheRoot, xPebble);
            pePebble pebble = pePebbleAlloc();
            peLocationInsertPebble(location, pebble);
            total++;
        }
        */
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

// Parse the type of graph we want to test.
static peGraphType parseType(char *name) {
    if(!strcmp(name, "rand_cubed")) {
        return RAND_CUBED;
    }
    if(!strcmp(name, "sliding_window")) {
        return SLIDING_WINDOW;
    }
    printf("usage: pebble [-d] [graph-type]\n"
        "    graph type can be: rand_cubed, sliding_window\n");
    exit(1);
}

int main(int argc, char **argv) {
    utStart();
    peDatabaseStart();
    peTheRoot = peRootAlloc();
    peVisitedLocations = peLocationArrayAlloc();
    peRootAllocLocations(peTheRoot, MEM_LENGTH);
    peGraphType type = RAND_CUBED;
    if(argc > 1 && *(argv[argc-1]) != '-') {
        type = parseType(argv[argc-1]);
    }
    uint32 i;
    for(i = 0; i < MEM_LENGTH; i++) {
        peLocation location = peLocationAlloc();
        peRootInsertLocation(peTheRoot, i, location);
        setPrevLocation(i, type);
    }
    setNumPointers();
    //randomlyDistributePebbles();
    computeAveragePenalty();
    computeCut();
    if(argc >= 2 && !strcmp(argv[1], "-d")) {
        dumpGraph();
    }
    peDatabaseStop();
    utStop(true);
    return 0;
}

#include <stdlib.h>
#include <math.h>
#include "pedatabase.h"

//#define MEM_LENGTH 10000000
#define MEM_LENGTH 2000
#define NUM_PEBBLES (MEM_LENGTH/2)

peRoot peTheRoot;
peLocationArray peVisitedLocations;
peGroup peTheGroup;
bool peVerbose;
uint32 peMaxDegree;
uint32 peCurrentPos;

// Find the previous position to point to.  Only call this once, then store the result.
static uint32 computePrevPosOnce(uint32 pos) {
    if(pos <= 2) {
        return 0;
    }
    double dist = rand()/(double)RAND_MAX;
    dist = dist*dist*dist;
    return pos - 2 - (uint32)((pos-2)*dist);
}

// Find the previous position to point to.
static inline uint32 findPrevPos(uint32 pos) {
    if(pos == 0) {
        return 0;
    }
    peLocation prevLocation = peLocationGetLocation(peRootGetiLocation(peTheRoot, pos));
    return peLocationGetRootIndex(prevLocation);
}

// Dump the graph.
static void dumpGraph(void) {
    uint32 pos;
    for(pos = 0; pos < MEM_LENGTH; pos++) {
        peLocation location = peRootGetiLocation(peTheRoot, pos);
        pePebble pebble = peLocationGetPebble(location);
        char c = ' ';
        if(pebble != pePebbleNull) {
            c = '*';
            if(pePebbleGetGroup(pebble) != peGroupNull) {
                c = '-';
            } else if(pePebbleInUse(pebble)) {
                c = '!';
            }
        }
        if(peVerbose) {
            printf("%c n%u -> n%u\t%u pointers\n", c, pos, findPrevPos(pos), peLocationGetNumPointers(location));
        }
    }
}

// Find the number of pointers that come from the beyond the max pebble placement.
static uint32 findFuturePointers(peLocation location) {
    uint32 numPointers = 0;
    peLocation nextLocation;
    peForeachLocationLocation(location, nextLocation) {
        if(peLocationGetRootIndex(nextLocation) >= peCurrentPos) {
            numPointers++;
        }
    } peEndLocationLocation;
    if(peLocationGetRootIndex(location) + 1 == peCurrentPos) {
        return numPointers + 1;
    }
    return numPointers;
}

// Create a new lazy pebble group.
static void addPebblesToGroup(uint32 maxPointers) {
    uint32 numPebbles = 0;
    peLocation location;
    peForeachRootLocation(peTheRoot, location) {
        if(findFuturePointers(location) <= maxPointers) {
            pePebble pebble = peLocationGetPebble(location);
            if(pebble != pePebbleNull && !pePebbleInUse(pebble)) {
                peGroupAppendPebble(peTheGroup, pebble);
                numPebbles++;
            }
        }
    } peEndRootLocation;
    peGroupSetAvailablePebbles(peTheGroup, numPebbles);
    if(peVerbose) {
        printf("Rebuilt group with %u pebbles\n", numPebbles);
        dumpGraph();
    }
}

// The group is empty, so delete all the pebbles since they have been used.  Then create a
// new group with the pebbles in the graph.  Try to build a group using the minimum degree
// possible.
static void rebuildGroup(void) {
    pePebble pebble;
    peSafeForeachGroupPebble(peTheGroup, pebble) {
        pePebbleDestroy(pebble);
    } peEndSafeGroupPebble;
    uint32 i = 0;
    do {
        addPebblesToGroup(i);
    } while(++i <= peMaxDegree && peGroupGetAvailablePebbles(peTheGroup) == 0);
}

// Pull a pebble from the group.  Just decrement the available counter, and return a new pebble.
static pePebble pullPebbleFromGroup(void) {
    uint32 numAvailable = peGroupGetAvailablePebbles(peTheGroup);
    if(numAvailable == 0) {
        rebuildGroup();
        if(peGroupGetAvailablePebbles(peTheGroup) == 0) {
            printf("Failed to pebble with %u pebbles.\n", NUM_PEBBLES);
            if(peVerbose) {
                dumpGraph();
            }
            exit(1);
        }
    } else {
        peGroupSetAvailablePebbles(peTheGroup, numAvailable-1);
    }
    return pePebbleAlloc();
}

// Remove the pebble from the group, since it's needed in it's current location.
static void removePebbleFromGroup(pePebble pebble) {
    uint32 numAvailable = peGroupGetAvailablePebbles(peTheGroup);
    utAssert(numAvailable > 0 && pePebbleInUse(pebble));
    peGroupRemovePebble(peTheGroup, pebble);
    if(numAvailable == 1) {
        rebuildGroup();
    } else {
        peGroupSetAvailablePebbles(peTheGroup, numAvailable-1);
    }
}

// Find a pebble that will be used as far as possible in the future, and pick it up.
static inline pePebble pickUpPebble(void) {
    pePebble pebble = pullPebbleFromGroup();
    if(peVerbose) {
        printf("Picking up pebble, %u remain available\n", peGroupGetAvailablePebbles(peTheGroup));
    }
    return pebble;
}

// Place a pebble.  This causes any pebble on the fanin that had this pebble location as
// it's needed position to have to recompute it's needed position.
static void placePebble(pePebble pebble, uint32 pos) {
    peLocation location = peRootGetiLocation(peTheRoot, pos);
    peLocationInsertPebble(location, pebble);
    if(peVerbose) {
        printf("Placing %u\n", pos);
    }
}

// Pebble the location and return the total number of pebbles moved.
static uint32 pebbleLocation(uint32 pos) {
    peLocation location = peRootGetiLocation(peTheRoot, pos);
    pePebble pebble = peLocationGetPebble(location);
    if(pebble != pePebbleNull) {
        pePebbleSetInUse(pebble, true); // Must come before call to remove from group
        if(pePebbleGetGroup(pebble) != peGroupNull) {
            removePebbleFromGroup(pebble);
        }
        return 0;
    }
    if(peVerbose) {
        printf("Trying to cover %u\n", pos);
    }
    uint32 total = 0;
    uint32 prevPos = findPrevPos(pos);
    if(pos > 0) {
        total += pebbleLocation(pos-1);
        total += pebbleLocation(prevPos);
    }
    pebble = pickUpPebble();
    placePebble(pebble, pos);
    if(peVerbose) {
        dumpGraph();
    }
    if(pos > 0) {
        pePebble prevPebble = peLocationGetPebble(peRootGetiLocation(peTheRoot, pos-1));
        pePebbleSetInUse(prevPebble, false);
        prevPebble = peLocationGetPebble(peRootGetiLocation(peTheRoot, prevPos));
        pePebbleSetInUse(prevPebble, false);
    }
    pePebbleSetInUse(pebble, true);
    if(peGroupGetAvailablePebbles(peTheGroup) == 0) {
        rebuildGroup();
    }
    return total + 1;
}

// Pebble the graph with a fixed number of pebbles.  When a pebble is needed, pick up a
// pebble that currently is not needed until the maximum time in the future.
static void pebbleGraph(void) {
    peTheGroup = peGroupAlloc();
    peCurrentPos = NUM_PEBBLES;
    rebuildGroup();
    uint64 total = NUM_PEBBLES;
    for(; peCurrentPos < MEM_LENGTH; peCurrentPos++) {
        total += pebbleLocation(peCurrentPos);
    }
    printf("Recalculation penalty is %.4fX\n", total/(double)MEM_LENGTH - 1.0);
}

// Set the number of pointers to each memory location.
static void setNumPointers(void) {
    peMaxDegree = 0;
    uint32 i;
    for(i = 1; i < MEM_LENGTH; i++) {
        peLocation location = peRootGetiLocation(peTheRoot, i);
        uint32 prevPos = computePrevPosOnce(i);
        peLocation prevLocation = peRootGetiLocation(peTheRoot, prevPos);
        uint32 numPointers = peLocationGetNumPointers(prevLocation) + 1;
        peLocationSetNumPointers(prevLocation, numPointers);
        peLocationInsertLocation(prevLocation, location);
        if(numPointers > peMaxDegree) {
            peMaxDegree = numPointers;
        }
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

// Distribute pebbles in the first memory locations.
static void distributePebbles(void) {
    uint32 total = 0;
    uint32 xPebble;
    for(xPebble = 0; xPebble < NUM_PEBBLES; xPebble++) {
        peLocation location = peRootGetiLocation(peTheRoot, xPebble);
        pePebble pebble = pePebbleAlloc();
        peLocationInsertPebble(location, pebble);
        total++;
    }
    printf("Total pebbles: %u out of %u (%.1f%%)\n", total, MEM_LENGTH, total*100.0/MEM_LENGTH);
}

int main(int argc, char **argv) {
    peVerbose = false;
    utStart();
    peDatabaseStart();
    peTheRoot = peRootAlloc();
    peVisitedLocations = peLocationArrayAlloc();
    peRootAllocLocations(peTheRoot, MEM_LENGTH);
    uint32 i;
    for(i = 0; i < MEM_LENGTH; i++) {
        peLocation location = peLocationAlloc();
        peLocationSetDepth(location, UINT32_MAX);
        peRootInsertLocation(peTheRoot, i, location);
    }
    setNumPointers();
    distributePebbles();
    if(argc >= 2 && !strcmp(argv[1], "-d")) {
        dumpGraph();
    }
    if(argc >= 2 && !strcmp(argv[1], "-v")) {
        dumpGraph();
        peVerbose = true;
    }
    pebbleGraph();
    computeCut();
    peDatabaseStop();
    utStop(true);
    return 0;
}

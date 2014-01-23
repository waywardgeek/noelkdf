#include <stdlib.h>
#include <math.h>
#include "pedatabase.h"

#define MEM_LENGTH 2048
#define NUM_PEBBLES (MEM_LENGTH/5)
#define SPACING 16
#define SPACING_START 11

peRoot peTheRoot;
peLocationArray peVisitedLocations;
peGroup peTheGroup;
bool peVerbose;
uint32 peMaxDegree;
uint32 peCurrentPos;

typedef enum {
    SLIDING_WINDOW,
    RAND_CUBED,
    RAND,
    CATENA3
} peGraphType;

peGraphType peCurrentType;

// Return the name of the type.
char *peTypeGetName(peGraphType type) {
    switch(type) {
    case SLIDING_WINDOW: return "sliding_window";
    case RAND_CUBED: return "rand_cubed";
    case RAND: return "rand";
    case CATENA3: return "catena3";
    default:
        utExit("Unknown graph type");
    }
    return NULL; // Dummy return
}

// Return the type given the name.
static peGraphType parseType(char *name) {
    if(!strcmp(name, "sliding_window")) {
        return SLIDING_WINDOW;
    }
    if(!strcmp(name, "rand_cubed")) {
        return RAND_CUBED;
    }
    if(!strcmp(name, "rand")) {
        return RAND;
    }
    if(!strcmp(name, "catena3")) {
        return CATENA3;
    }
    utExit("Unknown graph type %s", name);
    return RAND_CUBED;
}

// Find the previous position using Alexander's sliding power-of-two window.
static uint32 findSlidingWindowPos(uint32 pos) {
    // This is a sliding window which is the largest power of 2 < i.
    if(pos < 2) {
        return UINT32_MAX; // No edge
    }
    uint32 mask = 1;
    while(mask < pos) {
        mask <<= 1;
    }
    mask = (mask >> 1) - 1;
    return pos - 1 - (rand() & mask);
}

// Find the previous position using a uniform random value between 0..1, cube it, and go
// that * position back.
static uint32 findRandCubedPos(uint32 pos) {
    if(pos < 2) {
        return UINT32_MAX; // No edge
    }
    double dist = (rand()/(double)RAND_MAX);
    dist = dist*dist*dist;
    return pos - 2 - (uint32)((pos-2)*dist);
}

// Find the previous position using a uniform random value.
static uint32 findRandPos(uint32 pos) {
    if(pos < 2) {
        return UINT32_MAX; // No edge
    }
    double dist = rand()/(double)RAND_MAX;
    return pos - 2 - (uint32)((pos-2)*dist);
}

// Reverse the bits.
static uint32 reverse(uint32 value, uint32 rowLength) {
    //printf("rowLength:%u %x -> ", rowLength, value);
    uint32 result = 0;
    while(rowLength != 1) {
        result = (result << 1) | (value & 1);
        value >>= 1;
        rowLength >>= 1;
    }
    //printf("%x\n", result);
    return result;
}

// Find the previous position using a uniform random value between 0..1, cube it, and go
// that * position back.
static uint32 findCatena3Pos(uint32 pos) {
    uint32 rowLength = MEM_LENGTH/4; // Note: MEM_LENGTH must be power of 2
    uint32 row = pos/rowLength;
    if(row == 0) {
        return UINT32_MAX;
    }
    uint32 rowPos = pos - row*rowLength;
    return (row-1)*rowLength + reverse(rowPos, rowLength);
}

// Find the previous position to point to.
static void setPrevLocation(uint32 pos, peGraphType type) {
    if(pos == 0) {
        return;
    }
    peLocation location = peRootGetiLocation(peTheRoot, pos);
    uint32 prevPos;
    switch(type) {
    case SLIDING_WINDOW: prevPos = findSlidingWindowPos(pos); break;
    case RAND_CUBED: prevPos = findRandCubedPos(pos); break;
    case RAND: prevPos = findRandPos(pos); break;
    case CATENA3: prevPos = findCatena3Pos(pos); break;
    default:
        utExit("Unknown graph type\n");
    }
    if(prevPos == UINT32_MAX) {
        return; // No edge
    }
    peLocation prevLocation = peRootGetiLocation(peTheRoot, prevPos);
    peLocationInsertLocation(prevLocation, location);
}

// Find the previous position to point to.
static inline uint32 findPrevPos(uint32 pos) {
    peLocation prevLocation = peLocationGetLocation(peRootGetiLocation(peTheRoot, pos));
    if(prevLocation == peLocationNull) {
        return 0;
    }
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
        if(peLocationGetLocation(location) == peLocationNull) {
            printf("%c n%-17u %u pointers\n", c, pos, peLocationGetNumPointers(location));
        } else {
            printf("%c n%-6u -> n%-6u %u pointers\n", c, pos, findPrevPos(pos), peLocationGetNumPointers(location));
        }
    }
}

// Find the number of pointers that come from the beyond the max pebble placement.
static uint32 findFuturePointers(peLocation location) {
    uint32 numPointers = 0;
    uint32 currentPos = peCurrentPos;
    if(peLocationGetPebble(peRootGetiLocation(peTheRoot, peCurrentPos)) != pePebbleNull) {
        currentPos++;
    }
    peLocation nextLocation;
    peForeachLocationLocation(location, nextLocation) {
        if(peLocationGetRootIndex(nextLocation) >= currentPos) {
            numPointers++;
        }
    } peEndLocationLocation;
    if(peLocationGetRootIndex(location) + 1 == currentPos) {
        return numPointers + 1;
    }
    return numPointers;
}

// Create a new lazy pebble group.
static void addPebblesToGroup(void) {
    uint32 numPebbles = 0;
    peLocation location;
    peForeachRootLocation(peTheRoot, location) {
        if(findFuturePointers(location) == 0) {
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

// We are in the position of having to pick up a pebble we know we will need in the
// future.  Try to find a low-pain choice, and add it to the group.  This that make a
// choice low pain are low initial computation cost and being needed further in the
// future.
static pePebble findLeastBadPebble(void) {
    peLocation location;
    peForeachRootLocation(peTheRoot, location) {
        pePebble pebble = peLocationGetPebble(location);
        if(pebble != pePebbleNull && !pePebbleInUse(pebble)) {
            return pebble;
        }
    } peEndRootLocation;
    utExit("Where did all the pebbles go?");
    return pePebbleNull;
}

// The group is empty, so delete all the pebbles since they have been used.  Then create a
// new group with the pebbles in the graph.  Try to build a group using the minimum degree
// possible.
static void rebuildGroup(void) {
    pePebble pebble;
    peSafeForeachGroupPebble(peTheGroup, pebble) {
        pePebbleDestroy(pebble);
    } peEndSafeGroupPebble;
    addPebblesToGroup();
    if(peGroupGetAvailablePebbles(peTheGroup) == 0) {
        pebble = findLeastBadPebble();
        peGroupAppendPebble(peTheGroup, pebble);
        peGroupSetAvailablePebbles(peTheGroup, 1);
    }
}

// Pull a pebble from the group.  Just decrement the available counter, and return a new pebble.
static pePebble pullPebbleFromGroup(void) {
    uint32 numAvailable = peGroupGetAvailablePebbles(peTheGroup);
    if(numAvailable == 0) {
        rebuildGroup();
        if(peGroupGetAvailablePebbles(peTheGroup) == 0) {
            if(peVerbose) {
                dumpGraph();
            }
            utExit("Failed to pebble with %u pebbles.\n", NUM_PEBBLES);
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

// Mark the pebble in use.
static inline void markInUse(pePebble pebble) {
    pePebbleSetInUse(pebble, true); // Must come before call to remove from group
    if(pePebbleGetGroup(pebble) != peGroupNull) {
        removePebbleFromGroup(pebble);
    }
}

// Pebble the location and return the total number of pebbles moved.
static uint32 pebbleLocation(uint32 pos) {
    peLocation location = peRootGetiLocation(peTheRoot, pos);
    pePebble pebble = peLocationGetPebble(location);
    if(pebble != pePebbleNull) {
        markInUse(pebble);
        return 0;
    }
    if(peVerbose) {
        printf("Trying to cover %u\n", pos);
    }
    peLocation prevLocation = peLocationGetLocation(location);
    pePebble prevPebble;
    uint32 total = 0;
    if(pos > 0) {
        pePebble prevPebble = peLocationGetPebble(peRootGetiLocation(peTheRoot, pos-1));
        if(prevPebble != pePebbleNull) {
            markInUse(prevPebble);
        }
    }
    if(prevLocation != peLocationNull) {
        prevPebble = peLocationGetPebble(prevLocation);
        if(prevPebble != pePebbleNull) {
            markInUse(prevPebble);
        }
    }
    if(pos > 0) {
        total += pebbleLocation(pos-1);
    }
    if(prevLocation != peLocationNull) {
        total += pebbleLocation(peLocationGetRootIndex(prevLocation));
    }
    pebble = pickUpPebble();
    placePebble(pebble, pos);
    if(peVerbose) {
        dumpGraph();
    }
    if(pos > 0) {
        pePebble prevPebble = peLocationGetPebble(peRootGetiLocation(peTheRoot, pos-1));
        if(!pePebbleFixed(prevPebble)) {
            pePebbleSetInUse(prevPebble, false);
        }
    }
    if(prevLocation != peLocationNull) {
        prevPebble = peLocationGetPebble(peRootGetiLocation(peTheRoot, peLocationGetRootIndex(prevLocation)));
        if(!pePebbleFixed(prevPebble)) {
            pePebbleSetInUse(prevPebble, false);
        }
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
    peRootAppendGroup(peTheRoot, peTheGroup);
    peCurrentPos = NUM_PEBBLES;
    rebuildGroup();
    uint64 total = NUM_PEBBLES;
    for(; peCurrentPos < MEM_LENGTH; peCurrentPos++) {
        total += pebbleLocation(peCurrentPos);
        if(peCurrentPos >= SPACING_START && peCurrentPos % SPACING == SPACING_START) {
            // Fix the position of pebbles every so often to see if it helps.
            pePebbleSetFixed(peLocationGetPebble(peRootGetiLocation(peTheRoot, peCurrentPos)), true);
        }
    }
    printf("Recalculation penalty is %.4fX\n", total/(double)MEM_LENGTH - 1.0);
}

// Set the number of pointers to each memory location.
static void setNumPointers(peGraphType type) {
    peMaxDegree = 0;
    uint32 i;
    for(i = 1; i < MEM_LENGTH; i++) {
        setPrevLocation(i, type);
        peLocation prevLocation = peLocationGetLocation(peRootGetiLocation(peTheRoot, i));
        if(prevLocation != peLocationNull) {
            uint32 numPointers = peLocationGetNumPointers(prevLocation) + 1;
            peLocationSetNumPointers(prevLocation, numPointers);
            if(numPointers > peMaxDegree) {
                peMaxDegree = numPointers;
            }
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

// Test the type of graph.
static void runTest(peGraphType type, bool dumpGraphs) {
    peTheRoot = peRootAlloc();
    peVisitedLocations = peLocationArrayAlloc();
    peRootAllocLocations(peTheRoot, MEM_LENGTH);
    uint32 i;
    for(i = 0; i < MEM_LENGTH; i++) {
        peLocation location = peLocationAlloc();
        peLocationSetDepth(location, UINT32_MAX);
        peRootInsertLocation(peTheRoot, i, location);
    }
    printf("========= Testing %s\n", peTypeGetName(type));
    peCurrentType = type;
    setNumPointers(type);
    distributePebbles();
    if(dumpGraphs) {
        dumpGraph();
    }
    pebbleGraph();
    computeCut();
    peRootDestroy(peTheRoot);
    printf("\n");
}

int main(int argc, char **argv) {
    bool dumpGraphs = false;
    peVerbose = false;
    while(argc >= 2 && *argv[1] == '-') {
        if(!strcmp(argv[1], "-d")) {
            dumpGraphs = true;
        } else if(argc >= 2 && !strcmp(argv[1], "-v")) {
            dumpGraphs = true;
            peVerbose = true;
        }
        argc--;
        argv++;
    }
    utStart();
    peDatabaseStart();
    if(argc == 2) {
        runTest(parseType(argv[1]), dumpGraphs);
    } else {
        uint32 type;
        for(type = 0; type <= CATENA3; type++) {
            runTest(type, dumpGraphs);
        }
    }
    peDatabaseStop();
    utStop(true);
    return 0;
}

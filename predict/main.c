#include <stdlib.h>
#include <math.h>
#include "pedatabase.h"

uint32 peMemLength = 512;
uint32 peNumPebbles = 127;
uint32 peSpacingStart = UINT32_MAX;
uint32 peSpacing = 12;

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
    if(pos < 3) {
        return UINT32_MAX; // No edge
    }
    uint32 mask = 1;
    while(mask < pos - 1) {
        mask <<= 1;
    }
    mask = (mask >> 1) - 1;
    return pos - 2 - (rand() & mask);
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
    uint32 rowLength = peMemLength/4; // Note: peMemLength must be power of 2
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
    uint32 prevPos = 0;
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
    utAssert(prevPos + 1 < pos);
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
    for(pos = 0; pos < peMemLength; pos++) {
        peLocation location = peRootGetiLocation(peTheRoot, pos);
        pePebble pebble = peLocationGetPebble(location);
        char c = ' ';
        if(pebble != pePebbleNull) {
            c = '*';
            if(pePebbleGetGroup(pebble) != peGroupNull) {
                c = '-';
            } else if(pePebbleFixed(pebble)) {
                c = '!';
            } else if(pePebbleGetUseCount(pebble) != 0) {
                c = '0' + pePebbleGetUseCount(pebble);
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
            if(pebble != pePebbleNull && pePebbleGetUseCount(pebble) == 0) {
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
        if(pebble != pePebbleNull && pePebbleGetUseCount(pebble) == 0) {
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
            utExit("Failed to pebble with %u pebbles.\n", peNumPebbles);
        }
    } else {
        peGroupSetAvailablePebbles(peTheGroup, numAvailable-1);
    }
    return pePebbleAlloc();
}

// Remove the pebble from the group, since it's needed in it's current location.
static void removePebbleFromGroup(pePebble pebble) {
    uint32 numAvailable = peGroupGetAvailablePebbles(peTheGroup);
    utAssert(numAvailable > 0 && pePebbleGetUseCount(pebble) == 1);
    peGroupRemovePebble(peTheGroup, pebble);
    if(numAvailable == 1) {
        rebuildGroup();
    } else {
        peGroupSetAvailablePebbles(peTheGroup, numAvailable-1);
    }
}

// Find a pebble that will be used as far as possible in the future, and pick it up.
static inline pePebble pickUpPebble(uint32 pos) {
    pePebble pebble = pePebbleNull;
    if(peGroupGetAvailablePebbles(peTheGroup) == 0) {
        // Allow the previous pebble to be used, but only if not locked down in another calculation.
        peLocation location = peRootGetiLocation(peTheRoot, pos);
        peLocation prevLocation = peLocationGetLocation(location);
        if(prevLocation != peLocationNull) {
            pePebble prevPebble = peLocationGetPebble(prevLocation);
            if(prevPebble != pePebbleNull && pePebbleGetUseCount(prevPebble) == 1 && !pePebbleFixed(prevPebble)) {
                pebble = prevPebble;
                peLocationRemovePebble(prevLocation, pebble);
            }
        }
        if(pebble == pePebbleNull && pos > 0) {
            prevLocation = peRootGetiLocation(peTheRoot, pos-1);
            pePebble prevPebble = peLocationGetPebble(prevLocation);
            if(prevPebble != pePebbleNull && pePebbleGetUseCount(prevPebble) == 1 && !pePebbleFixed(prevPebble)) {
                pebble = prevPebble;
                peLocationRemovePebble(prevLocation, pebble);
            }
        }
    }
    if(pebble == pePebbleNull) {
        if(peGroupGetAvailablePebbles(peTheGroup) == 0) {
            pebble = findLeastBadPebble();
            peGroupAppendPebble(peTheGroup, pebble);
            peGroupSetAvailablePebbles(peTheGroup, 1);
        }
        pebble = pullPebbleFromGroup();
    }
    if(peVerbose) {
        printf("Picking up pebble, %u remain available\n", peGroupGetAvailablePebbles(peTheGroup));
    }
    return pebble;
}

// Place a pebble.  This causes any pebble on the fanin that had this pebble location as
// it's needed position to have to recompute it's needed position.
static void placePebble(pePebble pebble, uint32 pos) {
    utAssert(pePebbleGetGroup(pebble) == peGroupNull);
    peLocation location = peRootGetiLocation(peTheRoot, pos);
    utAssert(peLocationGetPebble(location) == pePebbleNull);
    peLocationInsertPebble(location, pebble);
    if(peVerbose) {
        printf("Placing %u\n", pos);
    }
}

// Mark the pebble in use.
static inline void markInUse(pePebble pebble) {
    if(!pePebbleFixed(pebble)) {
        pePebbleSetUseCount(pebble, pePebbleGetUseCount(pebble) + 1); // Must come before call to remove from group
        if(pePebbleGetGroup(pebble) != peGroupNull) {
            removePebbleFromGroup(pebble);
        }
    }
}

// Unmark a pabble as in use.
static inline void unmarkInUse(pePebble pebble) {
    if(!pePebbleFixed(pebble)) {
        uint32 useCount = pePebbleGetUseCount(pebble);
        utAssert(useCount != 0);
        pePebbleSetUseCount(pebble, useCount - 1); // Must come before call to remove from group
    }
}

// Pebble the location and return the total number of pebbles moved.
static uint32 pebbleLocation(uint32 pos) {
    peLocation location = peRootGetiLocation(peTheRoot, pos);
    pePebble pebble = peLocationGetPebble(location);
    if(peVerbose) {
        printf("Trying to cover %u\n", pos);
    }
    peLocation prevLocation = peLocationGetLocation(location);
    pePebble prev1 = pePebbleNull;
    pePebble prev2 = pePebbleNull;
    uint32 total = 0;
    if(pos > 0) {
        prev1 = peLocationGetPebble(peRootGetiLocation(peTheRoot, pos-1));
        if(prev1 != pePebbleNull) {
            markInUse(prev1);
        }
    }
    if(prevLocation != peLocationNull) {
        prev2 = peLocationGetPebble(prevLocation);
        if(prev2 != pePebbleNull) {
            markInUse(prev2);
        }
    }
    if(pos > 0 && prev1 == pePebbleNull) {
        total += pebbleLocation(pos-1);
        prev1 = peLocationGetPebble(peRootGetiLocation(peTheRoot, pos-1));
    }
    if(prevLocation != peLocationNull && prev2 == pePebbleNull) {
        prev2 = peLocationGetPebble(prevLocation);
        if(prev2 == pePebbleNull) {
            total += pebbleLocation(peLocationGetRootIndex(prevLocation));
            prev2 = peLocationGetPebble(prevLocation);
        } else {
            // There is an odd case where pebbling prev1 creates a pebble in prev2's spot
            markInUse(prev2);
        }
    }
    pebble = pickUpPebble(pos);
    placePebble(pebble, pos);
    if(peVerbose) {
        dumpGraph();
    }
    pePebbleSetUseCount(pebble, 1);
    if(prev1 != pePebbleNull && prev1 != pebble) {
        unmarkInUse(prev1);
    }
    if(prev2 != pePebbleNull && prev2 != pebble) {
        unmarkInUse(prev2);
    }
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
    peCurrentPos = peNumPebbles;
    uint64 total = peNumPebbles;
    for(; peCurrentPos < peMemLength; peCurrentPos++) {
        total += pebbleLocation(peCurrentPos);
        pePebble pebble = peLocationGetPebble(peRootGetiLocation(peTheRoot, peCurrentPos));
        unmarkInUse(pebble);
        if(peCurrentPos >= peSpacingStart && peCurrentPos % peSpacing == peSpacingStart) {
            // Fix the position of pebbles every so often to see if it helps.
            pePebbleSetFixed(pebble, true);
            pePebbleSetUseCount(pebble, 1);
        }
    }
    printf("Recalculation penalty is %.4fX\n", total/(double)peMemLength - 1.0);
}

// Set the number of pointers to each memory location.
static void setNumPointers(peGraphType type) {
    peMaxDegree = 0;
    uint32 i;
    for(i = 1; i < peMemLength; i++) {
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
    for(i = peMemLength/2; i < peMemLength-1; i++) {
        if(findPrevPos(i) < peMemLength/2) {
            cutSize++;
        }
    }
    printf("Mid-cut size: %u (%.1f%%)\n", cutSize, (cutSize*100.0)/peMemLength);
}

// Distribute pebbles in the first memory locations.
static void distributePebbles(void) {
    uint32 total = 0;
    uint32 xPebble;
    for(xPebble = 0; xPebble < peNumPebbles; xPebble++) {
        peLocation location = peRootGetiLocation(peTheRoot, xPebble);
        pePebble pebble = pePebbleAlloc();
        peLocationInsertPebble(location, pebble);
        total++;
    }
    printf("Total pebbles: %u out of %u (%.1f%%)\n", total, peMemLength, total*100.0/peMemLength);
}

// Test the type of graph.
static void runTest(peGraphType type, bool dumpGraphs) {
    peTheRoot = peRootAlloc();
    peVisitedLocations = peLocationArrayAlloc();
    peRootAllocLocations(peTheRoot, peMemLength);
    uint32 i;
    for(i = 0; i < peMemLength; i++) {
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
        } else if(argc >= 3 && !strcmp(argv[1], "-m")) {
            peMemLength = atoi(argv[2]);
            argc--;
            argv++;
        } else if(argc >= 3 && !strcmp(argv[1], "-p")) {
            peNumPebbles = atoi(argv[2]);
            argc--;
            argv++;
        } else if(argc >= 3 && !strcmp(argv[1], "-s")) {
            peSpacing = atoi(argv[2]);
            if(peSpacingStart == UINT32_MAX) {
                peSpacingStart = peSpacing - 1;
            }
            argc--;
            argv++;
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

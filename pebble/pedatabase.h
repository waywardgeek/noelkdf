/*----------------------------------------------------------------------------------------
  Module header file for: pe module
----------------------------------------------------------------------------------------*/
#ifndef PEDATABASE_H

#define PEDATABASE_H

#if defined __cplusplus
extern "C" {
#endif

#ifndef DD_UTIL_H
#include "ddutil.h"
#endif

extern uint8 peModuleID;
/* Class reference definitions */
#if (defined(DD_DEBUG) && !defined(DD_NOSTRICT)) || defined(DD_STRICT)
typedef struct _struct_peRoot{char val;} *peRoot;
#define peRootNull ((peRoot)0)
typedef struct _struct_pePebble{char val;} *pePebble;
#define pePebbleNull ((pePebble)0)
typedef struct _struct_peLocation{char val;} *peLocation;
#define peLocationNull ((peLocation)0)
typedef struct _struct_peLocationArray{char val;} *peLocationArray;
#define peLocationArrayNull ((peLocationArray)0)
#else
typedef uint32 peRoot;
#define peRootNull 0
typedef uint32 pePebble;
#define pePebbleNull 0
typedef uint32 peLocation;
#define peLocationNull 0
typedef uint32 peLocationArray;
#define peLocationArrayNull 0
#endif

/* Constructor/Destructor hooks. */
typedef void (*peRootCallbackType)(peRoot);
extern peRootCallbackType peRootConstructorCallback;
typedef void (*pePebbleCallbackType)(pePebble);
extern pePebbleCallbackType pePebbleConstructorCallback;
typedef void (*peLocationCallbackType)(peLocation);
extern peLocationCallbackType peLocationConstructorCallback;
typedef void (*peLocationArrayCallbackType)(peLocationArray);
extern peLocationArrayCallbackType peLocationArrayConstructorCallback;
extern peLocationArrayCallbackType peLocationArrayDestructorCallback;

/*----------------------------------------------------------------------------------------
  Root structure
----------------------------------------------------------------------------------------*/
struct peRootType_ {
    uint32 hash; /* This depends only on the structure of the database */
    uint32 usedRoot, allocatedRoot;
    uint32 usedRootLocation, allocatedRootLocation, freeRootLocation;
    uint32 usedPebble, allocatedPebble;
    uint32 usedLocation, allocatedLocation;
    peLocationArray firstFreeLocationArray;
    uint32 usedLocationArray, allocatedLocationArray;
    uint32 usedLocationArrayLocation, allocatedLocationArrayLocation, freeLocationArrayLocation;
};
extern struct peRootType_ peRootData;

utInlineC uint32 peHash(void) {return peRootData.hash;}
utInlineC uint32 peUsedRoot(void) {return peRootData.usedRoot;}
utInlineC uint32 peAllocatedRoot(void) {return peRootData.allocatedRoot;}
utInlineC void peSetUsedRoot(uint32 value) {peRootData.usedRoot = value;}
utInlineC void peSetAllocatedRoot(uint32 value) {peRootData.allocatedRoot = value;}
utInlineC uint32 peUsedRootLocation(void) {return peRootData.usedRootLocation;}
utInlineC uint32 peAllocatedRootLocation(void) {return peRootData.allocatedRootLocation;}
utInlineC uint32 peFreeRootLocation(void) {return peRootData.freeRootLocation;}
utInlineC void peSetUsedRootLocation(uint32 value) {peRootData.usedRootLocation = value;}
utInlineC void peSetAllocatedRootLocation(uint32 value) {peRootData.allocatedRootLocation = value;}
utInlineC void peSetFreeRootLocation(int32 value) {peRootData.freeRootLocation = value;}
utInlineC uint32 peUsedPebble(void) {return peRootData.usedPebble;}
utInlineC uint32 peAllocatedPebble(void) {return peRootData.allocatedPebble;}
utInlineC void peSetUsedPebble(uint32 value) {peRootData.usedPebble = value;}
utInlineC void peSetAllocatedPebble(uint32 value) {peRootData.allocatedPebble = value;}
utInlineC uint32 peUsedLocation(void) {return peRootData.usedLocation;}
utInlineC uint32 peAllocatedLocation(void) {return peRootData.allocatedLocation;}
utInlineC void peSetUsedLocation(uint32 value) {peRootData.usedLocation = value;}
utInlineC void peSetAllocatedLocation(uint32 value) {peRootData.allocatedLocation = value;}
utInlineC peLocationArray peFirstFreeLocationArray(void) {return peRootData.firstFreeLocationArray;}
utInlineC void peSetFirstFreeLocationArray(peLocationArray value) {peRootData.firstFreeLocationArray = (value);}
utInlineC uint32 peUsedLocationArray(void) {return peRootData.usedLocationArray;}
utInlineC uint32 peAllocatedLocationArray(void) {return peRootData.allocatedLocationArray;}
utInlineC void peSetUsedLocationArray(uint32 value) {peRootData.usedLocationArray = value;}
utInlineC void peSetAllocatedLocationArray(uint32 value) {peRootData.allocatedLocationArray = value;}
utInlineC uint32 peUsedLocationArrayLocation(void) {return peRootData.usedLocationArrayLocation;}
utInlineC uint32 peAllocatedLocationArrayLocation(void) {return peRootData.allocatedLocationArrayLocation;}
utInlineC uint32 peFreeLocationArrayLocation(void) {return peRootData.freeLocationArrayLocation;}
utInlineC void peSetUsedLocationArrayLocation(uint32 value) {peRootData.usedLocationArrayLocation = value;}
utInlineC void peSetAllocatedLocationArrayLocation(uint32 value) {peRootData.allocatedLocationArrayLocation = value;}
utInlineC void peSetFreeLocationArrayLocation(int32 value) {peRootData.freeLocationArrayLocation = value;}

/* Validate macros */
#if defined(DD_DEBUG)
utInlineC peRoot peValidRoot(peRoot Root) {
    utAssert(utLikely(Root != peRootNull && (uint32)(Root - (peRoot)0) < peRootData.usedRoot));
    return Root;}
utInlineC pePebble peValidPebble(pePebble Pebble) {
    utAssert(utLikely(Pebble != pePebbleNull && (uint32)(Pebble - (pePebble)0) < peRootData.usedPebble));
    return Pebble;}
utInlineC peLocation peValidLocation(peLocation Location) {
    utAssert(utLikely(Location != peLocationNull && (uint32)(Location - (peLocation)0) < peRootData.usedLocation));
    return Location;}
utInlineC peLocationArray peValidLocationArray(peLocationArray LocationArray) {
    utAssert(utLikely(LocationArray != peLocationArrayNull && (uint32)(LocationArray - (peLocationArray)0) < peRootData.usedLocationArray));
    return LocationArray;}
#else
utInlineC peRoot peValidRoot(peRoot Root) {return Root;}
utInlineC pePebble peValidPebble(pePebble Pebble) {return Pebble;}
utInlineC peLocation peValidLocation(peLocation Location) {return Location;}
utInlineC peLocationArray peValidLocationArray(peLocationArray LocationArray) {return LocationArray;}
#endif

/* Object ref to integer conversions */
#if (defined(DD_DEBUG) && !defined(DD_NOSTRICT)) || defined(DD_STRICT)
utInlineC uint32 peRoot2Index(peRoot Root) {return Root - (peRoot)0;}
utInlineC uint32 peRoot2ValidIndex(peRoot Root) {return peValidRoot(Root) - (peRoot)0;}
utInlineC peRoot peIndex2Root(uint32 xRoot) {return (peRoot)(xRoot + (peRoot)(0));}
utInlineC uint32 pePebble2Index(pePebble Pebble) {return Pebble - (pePebble)0;}
utInlineC uint32 pePebble2ValidIndex(pePebble Pebble) {return peValidPebble(Pebble) - (pePebble)0;}
utInlineC pePebble peIndex2Pebble(uint32 xPebble) {return (pePebble)(xPebble + (pePebble)(0));}
utInlineC uint32 peLocation2Index(peLocation Location) {return Location - (peLocation)0;}
utInlineC uint32 peLocation2ValidIndex(peLocation Location) {return peValidLocation(Location) - (peLocation)0;}
utInlineC peLocation peIndex2Location(uint32 xLocation) {return (peLocation)(xLocation + (peLocation)(0));}
utInlineC uint32 peLocationArray2Index(peLocationArray LocationArray) {return LocationArray - (peLocationArray)0;}
utInlineC uint32 peLocationArray2ValidIndex(peLocationArray LocationArray) {return peValidLocationArray(LocationArray) - (peLocationArray)0;}
utInlineC peLocationArray peIndex2LocationArray(uint32 xLocationArray) {return (peLocationArray)(xLocationArray + (peLocationArray)(0));}
#else
utInlineC uint32 peRoot2Index(peRoot Root) {return Root;}
utInlineC uint32 peRoot2ValidIndex(peRoot Root) {return peValidRoot(Root);}
utInlineC peRoot peIndex2Root(uint32 xRoot) {return xRoot;}
utInlineC uint32 pePebble2Index(pePebble Pebble) {return Pebble;}
utInlineC uint32 pePebble2ValidIndex(pePebble Pebble) {return peValidPebble(Pebble);}
utInlineC pePebble peIndex2Pebble(uint32 xPebble) {return xPebble;}
utInlineC uint32 peLocation2Index(peLocation Location) {return Location;}
utInlineC uint32 peLocation2ValidIndex(peLocation Location) {return peValidLocation(Location);}
utInlineC peLocation peIndex2Location(uint32 xLocation) {return xLocation;}
utInlineC uint32 peLocationArray2Index(peLocationArray LocationArray) {return LocationArray;}
utInlineC uint32 peLocationArray2ValidIndex(peLocationArray LocationArray) {return peValidLocationArray(LocationArray);}
utInlineC peLocationArray peIndex2LocationArray(uint32 xLocationArray) {return xLocationArray;}
#endif

/*----------------------------------------------------------------------------------------
  Fields for class Root.
----------------------------------------------------------------------------------------*/
struct peRootFields {
    uint32 *LocationIndex_;
    uint32 *NumLocation;
    peLocation *Location;
    uint32 *UsedLocation;
};
extern struct peRootFields peRoots;

void peRootAllocMore(void);
void peRootCopyProps(peRoot peOldRoot, peRoot peNewRoot);
void peRootAllocLocations(peRoot Root, uint32 numLocations);
void peRootResizeLocations(peRoot Root, uint32 numLocations);
void peRootFreeLocations(peRoot Root);
void peCompactRootLocations(void);
utInlineC uint32 peRootGetLocationIndex_(peRoot Root) {return peRoots.LocationIndex_[peRoot2ValidIndex(Root)];}
utInlineC void peRootSetLocationIndex_(peRoot Root, uint32 value) {peRoots.LocationIndex_[peRoot2ValidIndex(Root)] = value;}
utInlineC uint32 peRootGetNumLocation(peRoot Root) {return peRoots.NumLocation[peRoot2ValidIndex(Root)];}
utInlineC void peRootSetNumLocation(peRoot Root, uint32 value) {peRoots.NumLocation[peRoot2ValidIndex(Root)] = value;}
#if defined(DD_DEBUG)
utInlineC uint32 peRootCheckLocationIndex(peRoot Root, uint32 x) {utAssert(x < peRootGetNumLocation(Root)); return x;}
#else
utInlineC uint32 peRootCheckLocationIndex(peRoot Root, uint32 x) {return x;}
#endif
utInlineC peLocation peRootGetiLocation(peRoot Root, uint32 x) {return peRoots.Location[
    peRootGetLocationIndex_(Root) + peRootCheckLocationIndex(Root, x)];}
utInlineC peLocation *peRootGetLocation(peRoot Root) {return peRoots.Location + peRootGetLocationIndex_(Root);}
#define peRootGetLocations peRootGetLocation
utInlineC void peRootSetLocation(peRoot Root, peLocation *valuePtr, uint32 numLocation) {
    peRootResizeLocations(Root, numLocation);
    memcpy(peRootGetLocations(Root), valuePtr, numLocation*sizeof(peLocation));}
utInlineC void peRootSetiLocation(peRoot Root, uint32 x, peLocation value) {
    peRoots.Location[peRootGetLocationIndex_(Root) + peRootCheckLocationIndex(Root, (x))] = value;}
utInlineC uint32 peRootGetUsedLocation(peRoot Root) {return peRoots.UsedLocation[peRoot2ValidIndex(Root)];}
utInlineC void peRootSetUsedLocation(peRoot Root, uint32 value) {peRoots.UsedLocation[peRoot2ValidIndex(Root)] = value;}
utInlineC void peRootSetConstructorCallback(void(*func)(peRoot)) {peRootConstructorCallback = func;}
utInlineC peRootCallbackType peRootGetConstructorCallback(void) {return peRootConstructorCallback;}
utInlineC peRoot peFirstRoot(void) {return peRootData.usedRoot == 1? peRootNull : peIndex2Root(1);}
utInlineC peRoot peLastRoot(void) {return peRootData.usedRoot == 1? peRootNull :
    peIndex2Root(peRootData.usedRoot - 1);}
utInlineC peRoot peNextRoot(peRoot Root) {return peRoot2ValidIndex(Root) + 1 == peRootData.usedRoot? peRootNull :
    Root + 1;}
utInlineC peRoot pePrevRoot(peRoot Root) {return peRoot2ValidIndex(Root) == 1? peRootNull : Root - 1;}
#define peForeachRoot(var) \
    for(var = peIndex2Root(1); peRoot2Index(var) != peRootData.usedRoot; var++)
#define peEndRoot
utInlineC void peRootFreeAll(void) {peSetUsedRoot(1); peSetUsedRootLocation(0);}
utInlineC peRoot peRootAllocRaw(void) {
    peRoot Root;
    if(peRootData.usedRoot == peRootData.allocatedRoot) {
        peRootAllocMore();
    }
    Root = peIndex2Root(peRootData.usedRoot);
    peSetUsedRoot(peUsedRoot() + 1);
    return Root;}
utInlineC peRoot peRootAlloc(void) {
    peRoot Root = peRootAllocRaw();
    peRootSetLocationIndex_(Root, 0);
    peRootSetNumLocation(Root, 0);
    peRootSetNumLocation(Root, 0);
    peRootSetUsedLocation(Root, 0);
    if(peRootConstructorCallback != NULL) {
        peRootConstructorCallback(Root);
    }
    return Root;}

/*----------------------------------------------------------------------------------------
  Fields for class Pebble.
----------------------------------------------------------------------------------------*/
struct pePebbleFields {
    uint32 *Position;
    peLocation *Location;
};
extern struct pePebbleFields pePebbles;

void pePebbleAllocMore(void);
void pePebbleCopyProps(pePebble peOldPebble, pePebble peNewPebble);
utInlineC uint32 pePebbleGetPosition(pePebble Pebble) {return pePebbles.Position[pePebble2ValidIndex(Pebble)];}
utInlineC void pePebbleSetPosition(pePebble Pebble, uint32 value) {pePebbles.Position[pePebble2ValidIndex(Pebble)] = value;}
utInlineC peLocation pePebbleGetLocation(pePebble Pebble) {return pePebbles.Location[pePebble2ValidIndex(Pebble)];}
utInlineC void pePebbleSetLocation(pePebble Pebble, peLocation value) {pePebbles.Location[pePebble2ValidIndex(Pebble)] = value;}
utInlineC void pePebbleSetConstructorCallback(void(*func)(pePebble)) {pePebbleConstructorCallback = func;}
utInlineC pePebbleCallbackType pePebbleGetConstructorCallback(void) {return pePebbleConstructorCallback;}
utInlineC pePebble peFirstPebble(void) {return peRootData.usedPebble == 1? pePebbleNull : peIndex2Pebble(1);}
utInlineC pePebble peLastPebble(void) {return peRootData.usedPebble == 1? pePebbleNull :
    peIndex2Pebble(peRootData.usedPebble - 1);}
utInlineC pePebble peNextPebble(pePebble Pebble) {return pePebble2ValidIndex(Pebble) + 1 == peRootData.usedPebble? pePebbleNull :
    Pebble + 1;}
utInlineC pePebble pePrevPebble(pePebble Pebble) {return pePebble2ValidIndex(Pebble) == 1? pePebbleNull : Pebble - 1;}
#define peForeachPebble(var) \
    for(var = peIndex2Pebble(1); pePebble2Index(var) != peRootData.usedPebble; var++)
#define peEndPebble
utInlineC void pePebbleFreeAll(void) {peSetUsedPebble(1);}
utInlineC pePebble pePebbleAllocRaw(void) {
    pePebble Pebble;
    if(peRootData.usedPebble == peRootData.allocatedPebble) {
        pePebbleAllocMore();
    }
    Pebble = peIndex2Pebble(peRootData.usedPebble);
    peSetUsedPebble(peUsedPebble() + 1);
    return Pebble;}
utInlineC pePebble pePebbleAlloc(void) {
    pePebble Pebble = pePebbleAllocRaw();
    pePebbleSetPosition(Pebble, 0);
    pePebbleSetLocation(Pebble, peLocationNull);
    if(pePebbleConstructorCallback != NULL) {
        pePebbleConstructorCallback(Pebble);
    }
    return Pebble;}

/*----------------------------------------------------------------------------------------
  Fields for class Location.
----------------------------------------------------------------------------------------*/
struct peLocationFields {
    uint32 *NumPointers;
    uint8 *Visited;
    pePebble *Pebble;
};
extern struct peLocationFields peLocations;

void peLocationAllocMore(void);
void peLocationCopyProps(peLocation peOldLocation, peLocation peNewLocation);
utInlineC uint32 peLocationGetNumPointers(peLocation Location) {return peLocations.NumPointers[peLocation2ValidIndex(Location)];}
utInlineC void peLocationSetNumPointers(peLocation Location, uint32 value) {peLocations.NumPointers[peLocation2ValidIndex(Location)] = value;}
utInlineC uint8 peLocationVisited(peLocation Location) {return peLocations.Visited[peLocation2ValidIndex(Location)];}
utInlineC void peLocationSetVisited(peLocation Location, uint8 value) {peLocations.Visited[peLocation2ValidIndex(Location)] = value;}
utInlineC pePebble peLocationGetPebble(peLocation Location) {return peLocations.Pebble[peLocation2ValidIndex(Location)];}
utInlineC void peLocationSetPebble(peLocation Location, pePebble value) {peLocations.Pebble[peLocation2ValidIndex(Location)] = value;}
utInlineC void peLocationSetConstructorCallback(void(*func)(peLocation)) {peLocationConstructorCallback = func;}
utInlineC peLocationCallbackType peLocationGetConstructorCallback(void) {return peLocationConstructorCallback;}
utInlineC peLocation peFirstLocation(void) {return peRootData.usedLocation == 1? peLocationNull : peIndex2Location(1);}
utInlineC peLocation peLastLocation(void) {return peRootData.usedLocation == 1? peLocationNull :
    peIndex2Location(peRootData.usedLocation - 1);}
utInlineC peLocation peNextLocation(peLocation Location) {return peLocation2ValidIndex(Location) + 1 == peRootData.usedLocation? peLocationNull :
    Location + 1;}
utInlineC peLocation pePrevLocation(peLocation Location) {return peLocation2ValidIndex(Location) == 1? peLocationNull : Location - 1;}
#define peForeachLocation(var) \
    for(var = peIndex2Location(1); peLocation2Index(var) != peRootData.usedLocation; var++)
#define peEndLocation
utInlineC void peLocationFreeAll(void) {peSetUsedLocation(1);}
utInlineC peLocation peLocationAllocRaw(void) {
    peLocation Location;
    if(peRootData.usedLocation == peRootData.allocatedLocation) {
        peLocationAllocMore();
    }
    Location = peIndex2Location(peRootData.usedLocation);
    peSetUsedLocation(peUsedLocation() + 1);
    return Location;}
utInlineC peLocation peLocationAlloc(void) {
    peLocation Location = peLocationAllocRaw();
    peLocationSetNumPointers(Location, 0);
    peLocationSetVisited(Location, 0);
    peLocationSetPebble(Location, pePebbleNull);
    if(peLocationConstructorCallback != NULL) {
        peLocationConstructorCallback(Location);
    }
    return Location;}

/*----------------------------------------------------------------------------------------
  Fields for class LocationArray.
----------------------------------------------------------------------------------------*/
struct peLocationArrayFields {
    uint32 *LocationIndex_;
    uint32 *NumLocation;
    peLocation *Location;
    uint32 *UsedLocation;
    peLocationArray *FreeList;
};
extern struct peLocationArrayFields peLocationArrays;

void peLocationArrayAllocMore(void);
void peLocationArrayCopyProps(peLocationArray peOldLocationArray, peLocationArray peNewLocationArray);
void peLocationArrayAllocLocations(peLocationArray LocationArray, uint32 numLocations);
void peLocationArrayResizeLocations(peLocationArray LocationArray, uint32 numLocations);
void peLocationArrayFreeLocations(peLocationArray LocationArray);
void peCompactLocationArrayLocations(void);
utInlineC uint32 peLocationArrayGetLocationIndex_(peLocationArray LocationArray) {return peLocationArrays.LocationIndex_[peLocationArray2ValidIndex(LocationArray)];}
utInlineC void peLocationArraySetLocationIndex_(peLocationArray LocationArray, uint32 value) {peLocationArrays.LocationIndex_[peLocationArray2ValidIndex(LocationArray)] = value;}
utInlineC uint32 peLocationArrayGetNumLocation(peLocationArray LocationArray) {return peLocationArrays.NumLocation[peLocationArray2ValidIndex(LocationArray)];}
utInlineC void peLocationArraySetNumLocation(peLocationArray LocationArray, uint32 value) {peLocationArrays.NumLocation[peLocationArray2ValidIndex(LocationArray)] = value;}
#if defined(DD_DEBUG)
utInlineC uint32 peLocationArrayCheckLocationIndex(peLocationArray LocationArray, uint32 x) {utAssert(x < peLocationArrayGetNumLocation(LocationArray)); return x;}
#else
utInlineC uint32 peLocationArrayCheckLocationIndex(peLocationArray LocationArray, uint32 x) {return x;}
#endif
utInlineC peLocation peLocationArrayGetiLocation(peLocationArray LocationArray, uint32 x) {return peLocationArrays.Location[
    peLocationArrayGetLocationIndex_(LocationArray) + peLocationArrayCheckLocationIndex(LocationArray, x)];}
utInlineC peLocation *peLocationArrayGetLocation(peLocationArray LocationArray) {return peLocationArrays.Location + peLocationArrayGetLocationIndex_(LocationArray);}
#define peLocationArrayGetLocations peLocationArrayGetLocation
utInlineC void peLocationArraySetLocation(peLocationArray LocationArray, peLocation *valuePtr, uint32 numLocation) {
    peLocationArrayResizeLocations(LocationArray, numLocation);
    memcpy(peLocationArrayGetLocations(LocationArray), valuePtr, numLocation*sizeof(peLocation));}
utInlineC void peLocationArraySetiLocation(peLocationArray LocationArray, uint32 x, peLocation value) {
    peLocationArrays.Location[peLocationArrayGetLocationIndex_(LocationArray) + peLocationArrayCheckLocationIndex(LocationArray, (x))] = value;}
utInlineC uint32 peLocationArrayGetUsedLocation(peLocationArray LocationArray) {return peLocationArrays.UsedLocation[peLocationArray2ValidIndex(LocationArray)];}
utInlineC void peLocationArraySetUsedLocation(peLocationArray LocationArray, uint32 value) {peLocationArrays.UsedLocation[peLocationArray2ValidIndex(LocationArray)] = value;}
utInlineC peLocationArray peLocationArrayGetFreeList(peLocationArray LocationArray) {return peLocationArrays.FreeList[peLocationArray2ValidIndex(LocationArray)];}
utInlineC void peLocationArraySetFreeList(peLocationArray LocationArray, peLocationArray value) {peLocationArrays.FreeList[peLocationArray2ValidIndex(LocationArray)] = value;}
utInlineC void peLocationArraySetConstructorCallback(void(*func)(peLocationArray)) {peLocationArrayConstructorCallback = func;}
utInlineC peLocationArrayCallbackType peLocationArrayGetConstructorCallback(void) {return peLocationArrayConstructorCallback;}
utInlineC void peLocationArraySetDestructorCallback(void(*func)(peLocationArray)) {peLocationArrayDestructorCallback = func;}
utInlineC peLocationArrayCallbackType peLocationArrayGetDestructorCallback(void) {return peLocationArrayDestructorCallback;}
utInlineC peLocationArray peLocationArrayNextFree(peLocationArray LocationArray) {return ((peLocationArray *)(void *)(peLocationArrays.FreeList))[peLocationArray2ValidIndex(LocationArray)];}
utInlineC void peLocationArraySetNextFree(peLocationArray LocationArray, peLocationArray value) {
    ((peLocationArray *)(void *)(peLocationArrays.FreeList))[peLocationArray2ValidIndex(LocationArray)] = value;}
utInlineC void peLocationArrayFree(peLocationArray LocationArray) {
    peLocationArrayFreeLocations(LocationArray);
    peLocationArraySetNextFree(LocationArray, peRootData.firstFreeLocationArray);
    peSetFirstFreeLocationArray(LocationArray);}
void peLocationArrayDestroy(peLocationArray LocationArray);
utInlineC peLocationArray peLocationArrayAllocRaw(void) {
    peLocationArray LocationArray;
    if(peRootData.firstFreeLocationArray != peLocationArrayNull) {
        LocationArray = peRootData.firstFreeLocationArray;
        peSetFirstFreeLocationArray(peLocationArrayNextFree(LocationArray));
    } else {
        if(peRootData.usedLocationArray == peRootData.allocatedLocationArray) {
            peLocationArrayAllocMore();
        }
        LocationArray = peIndex2LocationArray(peRootData.usedLocationArray);
        peSetUsedLocationArray(peUsedLocationArray() + 1);
    }
    return LocationArray;}
utInlineC peLocationArray peLocationArrayAlloc(void) {
    peLocationArray LocationArray = peLocationArrayAllocRaw();
    peLocationArraySetLocationIndex_(LocationArray, 0);
    peLocationArraySetNumLocation(LocationArray, 0);
    peLocationArraySetNumLocation(LocationArray, 0);
    peLocationArraySetUsedLocation(LocationArray, 0);
    peLocationArraySetFreeList(LocationArray, peLocationArrayNull);
    if(peLocationArrayConstructorCallback != NULL) {
        peLocationArrayConstructorCallback(LocationArray);
    }
    return LocationArray;}

/*----------------------------------------------------------------------------------------
  Relationship macros between classes.
----------------------------------------------------------------------------------------*/
#define peForeachRootLocation(pVar, cVar) { \
    uint32 _xLocation; \
    for(_xLocation = 0; _xLocation < peRootGetUsedLocation(pVar); _xLocation++) { \
        cVar = peRootGetiLocation(pVar, _xLocation); \
        if(cVar != peLocationNull) {
#define peEndRootLocation }}}
void peRootInsertLocation(peRoot Root, uint32 x, peLocation _Location);
void peRootAppendLocation(peRoot Root, peLocation _Location);
utInlineC void peLocationInsertPebble(peLocation Location, pePebble _Pebble) {peLocationSetPebble(Location, _Pebble); pePebbleSetLocation(_Pebble, Location);}
utInlineC void peLocationRemovePebble(peLocation Location, pePebble _Pebble) {peLocationSetPebble(Location, pePebbleNull); pePebbleSetLocation(_Pebble, peLocationNull);}
#define peForeachLocationArrayLocation(pVar, cVar) { \
    uint32 _xLocation; \
    for(_xLocation = 0; _xLocation < peLocationArrayGetUsedLocation(pVar); _xLocation++) { \
        cVar = peLocationArrayGetiLocation(pVar, _xLocation); \
        if(cVar != peLocationNull) {
#define peEndLocationArrayLocation }}}
void peLocationArrayInsertLocation(peLocationArray LocationArray, uint32 x, peLocation _Location);
void peLocationArrayAppendLocation(peLocationArray LocationArray, peLocation _Location);
void peDatabaseStart(void);
void peDatabaseStop(void);
utInlineC void peDatabaseSetSaved(bool value) {utModuleSetSaved(utModules + peModuleID, value);}
#if defined __cplusplus
}
#endif

#endif

/*----------------------------------------------------------------------------------------
  Database pe
----------------------------------------------------------------------------------------*/

#include "pedatabase.h"

struct peRootType_ peRootData;
uint8 peModuleID;
struct peRootFields peRoots;
struct pePebbleFields pePebbles;
struct peLocationFields peLocations;
struct peLocationArrayFields peLocationArrays;

/*----------------------------------------------------------------------------------------
  Constructor/Destructor hooks.
----------------------------------------------------------------------------------------*/
peRootCallbackType peRootConstructorCallback;
pePebbleCallbackType pePebbleConstructorCallback;
peLocationCallbackType peLocationConstructorCallback;
peLocationArrayCallbackType peLocationArrayConstructorCallback;
peLocationArrayCallbackType peLocationArrayDestructorCallback;

/*----------------------------------------------------------------------------------------
  Default constructor wrapper for the database manager.
----------------------------------------------------------------------------------------*/
static uint64 allocRoot(void)
{
    peRoot Root = peRootAlloc();

    return peRoot2Index(Root);
}

/*----------------------------------------------------------------------------------------
  Allocate the field arrays of Root.
----------------------------------------------------------------------------------------*/
static void allocRoots(void)
{
    peSetAllocatedRoot(2);
    peSetUsedRoot(1);
    peRoots.LocationIndex_ = utNewAInitFirst(uint32, (peAllocatedRoot()));
    peRoots.NumLocation = utNewAInitFirst(uint32, (peAllocatedRoot()));
    peSetUsedRootLocation(0);
    peSetAllocatedRootLocation(2);
    peSetFreeRootLocation(0);
    peRoots.Location = utNewAInitFirst(peLocation, peAllocatedRootLocation());
    peRoots.UsedLocation = utNewAInitFirst(uint32, (peAllocatedRoot()));
}

/*----------------------------------------------------------------------------------------
  Realloc the arrays of properties for class Root.
----------------------------------------------------------------------------------------*/
static void reallocRoots(
    uint32 newSize)
{
    utResizeArray(peRoots.LocationIndex_, (newSize));
    utResizeArray(peRoots.NumLocation, (newSize));
    utResizeArray(peRoots.UsedLocation, (newSize));
    peSetAllocatedRoot(newSize);
}

/*----------------------------------------------------------------------------------------
  Allocate more Roots.
----------------------------------------------------------------------------------------*/
void peRootAllocMore(void)
{
    reallocRoots((uint32)(peAllocatedRoot() + (peAllocatedRoot() >> 1)));
}

/*----------------------------------------------------------------------------------------
  Compact the Root.Location heap to free memory.
----------------------------------------------------------------------------------------*/
void peCompactRootLocations(void)
{
    uint32 elementSize = sizeof(peLocation);
    uint32 usedHeaderSize = (sizeof(peRoot) + elementSize - 1)/elementSize;
    uint32 freeHeaderSize = (sizeof(peRoot) + sizeof(uint32) + elementSize - 1)/elementSize;
    peLocation *toPtr = peRoots.Location;
    peLocation *fromPtr = toPtr;
    peRoot Root;
    uint32 size;

    while(fromPtr < peRoots.Location + peUsedRootLocation()) {
        Root = *(peRoot *)(void *)fromPtr;
        if(Root != peRootNull) {
            /* Need to move it to toPtr */
            size = utMax(peRootGetNumLocation(Root) + usedHeaderSize, freeHeaderSize);
            memmove((void *)toPtr, (void *)fromPtr, size*elementSize);
            peRootSetLocationIndex_(Root, toPtr - peRoots.Location + usedHeaderSize);
            toPtr += size;
        } else {
            /* Just skip it */
            size = utMax(*(uint32 *)(void *)(((peRoot *)(void *)fromPtr) + 1), freeHeaderSize);
        }
        fromPtr += size;
    }
    peSetUsedRootLocation(toPtr - peRoots.Location);
    peSetFreeRootLocation(0);
}

/*----------------------------------------------------------------------------------------
  Allocate more memory for the Root.Location heap.
----------------------------------------------------------------------------------------*/
static void allocMoreRootLocations(
    uint32 spaceNeeded)
{
    uint32 freeSpace = peAllocatedRootLocation() - peUsedRootLocation();
    uint32 elementSize = sizeof(peLocation);
    uint32 usedHeaderSize = (sizeof(peRoot) + elementSize - 1)/elementSize;
    uint32 freeHeaderSize = (sizeof(peRoot) + sizeof(uint32) + elementSize - 1)/elementSize;
    peLocation *ptr = peRoots.Location;
    peRoot Root;
    uint32 size;

    while(ptr < peRoots.Location + peUsedRootLocation()) {
        Root = *(peRoot*)(void*)ptr;
        if(Root != peRootNull) {
            peValidRoot(Root);
            size = utMax(peRootGetNumLocation(Root) + usedHeaderSize, freeHeaderSize);
        } else {
            size = utMax(*(uint32 *)(void *)(((peRoot *)(void *)ptr) + 1), freeHeaderSize);
        }
        ptr += size;
    }
    if((peFreeRootLocation() << 2) > peUsedRootLocation()) {
        peCompactRootLocations();
        freeSpace = peAllocatedRootLocation() - peUsedRootLocation();
    }
    if(freeSpace < spaceNeeded) {
        peSetAllocatedRootLocation(peAllocatedRootLocation() + spaceNeeded - freeSpace +
            (peAllocatedRootLocation() >> 1));
        utResizeArray(peRoots.Location, peAllocatedRootLocation());
    }
}

/*----------------------------------------------------------------------------------------
  Allocate memory for a new Root.Location array.
----------------------------------------------------------------------------------------*/
void peRootAllocLocations(
    peRoot Root,
    uint32 numLocations)
{
    uint32 freeSpace = peAllocatedRootLocation() - peUsedRootLocation();
    uint32 elementSize = sizeof(peLocation);
    uint32 usedHeaderSize = (sizeof(peRoot) + elementSize - 1)/elementSize;
    uint32 freeHeaderSize = (sizeof(peRoot) + sizeof(uint32) + elementSize - 1)/elementSize;
    uint32 spaceNeeded = utMax(numLocations + usedHeaderSize, freeHeaderSize);

#if defined(DD_DEBUG)
    utAssert(peRootGetNumLocation(Root) == 0);
#endif
    if(numLocations == 0) {
        return;
    }
    if(freeSpace < spaceNeeded) {
        allocMoreRootLocations(spaceNeeded);
    }
    peRootSetLocationIndex_(Root, peUsedRootLocation() + usedHeaderSize);
    peRootSetNumLocation(Root, numLocations);
    *(peRoot *)(void *)(peRoots.Location + peUsedRootLocation()) = Root;
    {
        uint32 xValue;
        for(xValue = (uint32)(peRootGetLocationIndex_(Root)); xValue < peRootGetLocationIndex_(Root) + numLocations; xValue++) {
            peRoots.Location[xValue] = peLocationNull;
        }
    }
    peSetUsedRootLocation(peUsedRootLocation() + spaceNeeded);
}

/*----------------------------------------------------------------------------------------
  Wrapper around peRootGetLocations for the database manager.
----------------------------------------------------------------------------------------*/
static void *getRootLocations(
    uint64 objectNumber,
    uint32 *numValues)
{
    peRoot Root = peIndex2Root((uint32)objectNumber);

    *numValues = peRootGetNumLocation(Root);
    return peRootGetLocations(Root);
}

/*----------------------------------------------------------------------------------------
  Wrapper around peRootAllocLocations for the database manager.
----------------------------------------------------------------------------------------*/
static void *allocRootLocations(
    uint64 objectNumber,
    uint32 numValues)
{
    peRoot Root = peIndex2Root((uint32)objectNumber);

    peRootSetLocationIndex_(Root, 0);
    peRootSetNumLocation(Root, 0);
    if(numValues == 0) {
        return NULL;
    }
    peRootAllocLocations(Root, numValues);
    return peRootGetLocations(Root);
}

/*----------------------------------------------------------------------------------------
  Free memory used by the Root.Location array.
----------------------------------------------------------------------------------------*/
void peRootFreeLocations(
    peRoot Root)
{
    uint32 elementSize = sizeof(peLocation);
    uint32 usedHeaderSize = (sizeof(peRoot) + elementSize - 1)/elementSize;
    uint32 freeHeaderSize = (sizeof(peRoot) + sizeof(uint32) + elementSize - 1)/elementSize;
    uint32 size = utMax(peRootGetNumLocation(Root) + usedHeaderSize, freeHeaderSize);
    peLocation *dataPtr = peRootGetLocations(Root) - usedHeaderSize;

    if(peRootGetNumLocation(Root) == 0) {
        return;
    }
    *(peRoot *)(void *)(dataPtr) = peRootNull;
    *(uint32 *)(void *)(((peRoot *)(void *)dataPtr) + 1) = size;
    peRootSetNumLocation(Root, 0);
    peSetFreeRootLocation(peFreeRootLocation() + size);
}

/*----------------------------------------------------------------------------------------
  Resize the Root.Location array.
----------------------------------------------------------------------------------------*/
void peRootResizeLocations(
    peRoot Root,
    uint32 numLocations)
{
    uint32 freeSpace;
    uint32 elementSize = sizeof(peLocation);
    uint32 usedHeaderSize = (sizeof(peRoot) + elementSize - 1)/elementSize;
    uint32 freeHeaderSize = (sizeof(peRoot) + sizeof(uint32) + elementSize - 1)/elementSize;
    uint32 newSize = utMax(numLocations + usedHeaderSize, freeHeaderSize);
    uint32 oldSize = utMax(peRootGetNumLocation(Root) + usedHeaderSize, freeHeaderSize);
    peLocation *dataPtr;

    if(numLocations == 0) {
        if(peRootGetNumLocation(Root) != 0) {
            peRootFreeLocations(Root);
        }
        return;
    }
    if(peRootGetNumLocation(Root) == 0) {
        peRootAllocLocations(Root, numLocations);
        return;
    }
    freeSpace = peAllocatedRootLocation() - peUsedRootLocation();
    if(freeSpace < newSize) {
        allocMoreRootLocations(newSize);
    }
    dataPtr = peRootGetLocations(Root) - usedHeaderSize;
    memcpy((void *)(peRoots.Location + peUsedRootLocation()), dataPtr,
        elementSize*utMin(oldSize, newSize));
    if(newSize > oldSize) {
        {
            uint32 xValue;
            for(xValue = (uint32)(peUsedRootLocation() + oldSize); xValue < peUsedRootLocation() + oldSize + newSize - oldSize; xValue++) {
                peRoots.Location[xValue] = peLocationNull;
            }
        }
    }
    *(peRoot *)(void *)dataPtr = peRootNull;
    *(uint32 *)(void *)(((peRoot *)(void *)dataPtr) + 1) = oldSize;
    peSetFreeRootLocation(peFreeRootLocation() + oldSize);
    peRootSetLocationIndex_(Root, peUsedRootLocation() + usedHeaderSize);
    peRootSetNumLocation(Root, numLocations);
    peSetUsedRootLocation(peUsedRootLocation() + newSize);
}

/*----------------------------------------------------------------------------------------
  Copy the properties of Root.
----------------------------------------------------------------------------------------*/
void peRootCopyProps(
    peRoot oldRoot,
    peRoot newRoot)
{
}

/*----------------------------------------------------------------------------------------
  Add the indexed Location to the Root.
----------------------------------------------------------------------------------------*/
void peRootInsertLocation(
    peRoot Root,
    uint32 x,
    peLocation _Location)
{
#if defined(DD_DEBUG)
    if(Root == peRootNull) {
        utExit("Non existent Root");
    }
    if(peLocationGetRoot(_Location) != peRootNull) {
        utExit("Attempting to add Location to Root twice");
    }
#endif
    peRootSetiLocation(Root, x, _Location);
    peRootSetUsedLocation(Root, utMax(peRootGetUsedLocation(Root), x + 1));
    peLocationSetRootIndex(_Location, x);
    peLocationSetRoot(_Location, Root);
}

/*----------------------------------------------------------------------------------------
  Add the Location to the end of the RootLocation array.
----------------------------------------------------------------------------------------*/
void peRootAppendLocation(
    peRoot Root,
    peLocation _Location)
{
    uint32 usedLocation = peRootGetUsedLocation(Root);

#if defined(DD_DEBUG)
    if(Root == peRootNull) {
        utExit("Non existent Root");
    }
#endif
    if(usedLocation >= peRootGetNumLocation(Root)) {
        peRootResizeLocations(Root, usedLocation + (usedLocation << 1) + 1);
    }
    peRootSetiLocation(Root, usedLocation, _Location);
    peRootSetUsedLocation(Root, usedLocation + 1);
    peLocationSetRootIndex(_Location, usedLocation);
    peLocationSetRoot(_Location, Root);
}

/*----------------------------------------------------------------------------------------
  Remove the Location from the Root.
----------------------------------------------------------------------------------------*/
void peRootRemoveLocation(
    peRoot Root,
    peLocation _Location)
{
#if defined(DD_DEBUG)
    if(_Location == peLocationNull) {
        utExit("Non-existent Location");
    }
    if(peLocationGetRoot(_Location) != peRootNull && peLocationGetRoot(_Location) != Root) {
        utExit("Delete Location from non-owning Root");
    }
#endif
    peRootSetiLocation(Root, peLocationGetRootIndex(_Location), peLocationNull);
    peLocationSetRootIndex(_Location, UINT32_MAX);
    peLocationSetRoot(_Location, peRootNull);
}

#if defined(DD_DEBUG)
/*----------------------------------------------------------------------------------------
  Write out all the fields of an object.
----------------------------------------------------------------------------------------*/
void peShowRoot(
    peRoot Root)
{
    utDatabaseShowObject("pe", "Root", peRoot2Index(Root));
}
#endif

/*----------------------------------------------------------------------------------------
  Default constructor wrapper for the database manager.
----------------------------------------------------------------------------------------*/
static uint64 allocPebble(void)
{
    pePebble Pebble = pePebbleAlloc();

    return pePebble2Index(Pebble);
}

/*----------------------------------------------------------------------------------------
  Allocate the field arrays of Pebble.
----------------------------------------------------------------------------------------*/
static void allocPebbles(void)
{
    peSetAllocatedPebble(2);
    peSetUsedPebble(1);
    pePebbles.Location = utNewAInitFirst(peLocation, (peAllocatedPebble()));
}

/*----------------------------------------------------------------------------------------
  Realloc the arrays of properties for class Pebble.
----------------------------------------------------------------------------------------*/
static void reallocPebbles(
    uint32 newSize)
{
    utResizeArray(pePebbles.Location, (newSize));
    peSetAllocatedPebble(newSize);
}

/*----------------------------------------------------------------------------------------
  Allocate more Pebbles.
----------------------------------------------------------------------------------------*/
void pePebbleAllocMore(void)
{
    reallocPebbles((uint32)(peAllocatedPebble() + (peAllocatedPebble() >> 1)));
}

/*----------------------------------------------------------------------------------------
  Copy the properties of Pebble.
----------------------------------------------------------------------------------------*/
void pePebbleCopyProps(
    pePebble oldPebble,
    pePebble newPebble)
{
}

#if defined(DD_DEBUG)
/*----------------------------------------------------------------------------------------
  Write out all the fields of an object.
----------------------------------------------------------------------------------------*/
void peShowPebble(
    pePebble Pebble)
{
    utDatabaseShowObject("pe", "Pebble", pePebble2Index(Pebble));
}
#endif

/*----------------------------------------------------------------------------------------
  Default constructor wrapper for the database manager.
----------------------------------------------------------------------------------------*/
static uint64 allocLocation(void)
{
    peLocation Location = peLocationAlloc();

    return peLocation2Index(Location);
}

/*----------------------------------------------------------------------------------------
  Allocate the field arrays of Location.
----------------------------------------------------------------------------------------*/
static void allocLocations(void)
{
    peSetAllocatedLocation(2);
    peSetUsedLocation(1);
    peLocations.NumPointers = utNewAInitFirst(uint32, (peAllocatedLocation()));
    peLocations.Visited = utNewAInitFirst(uint8, (peAllocatedLocation()));
    peLocations.Root = utNewAInitFirst(peRoot, (peAllocatedLocation()));
    peLocations.RootIndex = utNewAInitFirst(uint32, (peAllocatedLocation()));
    peLocations.Pebble = utNewAInitFirst(pePebble, (peAllocatedLocation()));
    peLocations.Location = utNewAInitFirst(peLocation, (peAllocatedLocation()));
    peLocations.FirstLocation = utNewAInitFirst(peLocation, (peAllocatedLocation()));
    peLocations.NextLocationLocation = utNewAInitFirst(peLocation, (peAllocatedLocation()));
}

/*----------------------------------------------------------------------------------------
  Realloc the arrays of properties for class Location.
----------------------------------------------------------------------------------------*/
static void reallocLocations(
    uint32 newSize)
{
    utResizeArray(peLocations.NumPointers, (newSize));
    utResizeArray(peLocations.Visited, (newSize));
    utResizeArray(peLocations.Root, (newSize));
    utResizeArray(peLocations.RootIndex, (newSize));
    utResizeArray(peLocations.Pebble, (newSize));
    utResizeArray(peLocations.Location, (newSize));
    utResizeArray(peLocations.FirstLocation, (newSize));
    utResizeArray(peLocations.NextLocationLocation, (newSize));
    peSetAllocatedLocation(newSize);
}

/*----------------------------------------------------------------------------------------
  Allocate more Locations.
----------------------------------------------------------------------------------------*/
void peLocationAllocMore(void)
{
    reallocLocations((uint32)(peAllocatedLocation() + (peAllocatedLocation() >> 1)));
}

/*----------------------------------------------------------------------------------------
  Copy the properties of Location.
----------------------------------------------------------------------------------------*/
void peLocationCopyProps(
    peLocation oldLocation,
    peLocation newLocation)
{
    peLocationSetNumPointers(newLocation, peLocationGetNumPointers(oldLocation));
    peLocationSetVisited(newLocation, peLocationVisited(oldLocation));
}

/*----------------------------------------------------------------------------------------
  Add the Location to the head of the list on the Location.
----------------------------------------------------------------------------------------*/
void peLocationInsertLocation(
    peLocation Location,
    peLocation _Location)
{
#if defined(DD_DEBUG)
    if(Location == peLocationNull) {
        utExit("Non-existent Location");
    }
    if(_Location == peLocationNull) {
        utExit("Non-existent Location");
    }
    if(peLocationGetLocation(_Location) != peLocationNull) {
        utExit("Attempting to add Location to Location twice");
    }
#endif
    peLocationSetNextLocationLocation(_Location, peLocationGetFirstLocation(Location));
    peLocationSetFirstLocation(Location, _Location);
    peLocationSetLocation(_Location, Location);
}

/*----------------------------------------------------------------------------------------
  Insert the Location to the Location after the previous Location.
----------------------------------------------------------------------------------------*/
void peLocationInsertAfterLocation(
    peLocation Location,
    peLocation prevLocation,
    peLocation _Location)
{
    peLocation nextLocation = peLocationGetNextLocationLocation(prevLocation);

#if defined(DD_DEBUG)
    if(Location == peLocationNull) {
        utExit("Non-existent Location");
    }
    if(_Location == peLocationNull) {
        utExit("Non-existent Location");
    }
    if(peLocationGetLocation(_Location) != peLocationNull) {
        utExit("Attempting to add Location to Location twice");
    }
#endif
    peLocationSetNextLocationLocation(_Location, nextLocation);
    peLocationSetNextLocationLocation(prevLocation, _Location);
    peLocationSetLocation(_Location, Location);
}

/*----------------------------------------------------------------------------------------
 Remove the Location from the Location.
----------------------------------------------------------------------------------------*/
void peLocationRemoveLocation(
    peLocation Location,
    peLocation _Location)
{
    peLocation pLocation, nLocation;

#if defined(DD_DEBUG)
    if(_Location == peLocationNull) {
        utExit("Non-existent Location");
    }
    if(peLocationGetLocation(_Location) != peLocationNull && peLocationGetLocation(_Location) != Location) {
        utExit("Delete Location from non-owning Location");
    }
#endif
    pLocation = peLocationNull;
    for(nLocation = peLocationGetFirstLocation(Location); nLocation != peLocationNull && nLocation != _Location;
            nLocation = peLocationGetNextLocationLocation(nLocation)) {
        pLocation = nLocation;
    }
    if(pLocation != peLocationNull) {
        peLocationSetNextLocationLocation(pLocation, peLocationGetNextLocationLocation(_Location));
    } else {
        peLocationSetFirstLocation(Location, peLocationGetNextLocationLocation(_Location));
    }
    peLocationSetNextLocationLocation(_Location, peLocationNull);
    peLocationSetLocation(_Location, peLocationNull);
}

#if defined(DD_DEBUG)
/*----------------------------------------------------------------------------------------
  Write out all the fields of an object.
----------------------------------------------------------------------------------------*/
void peShowLocation(
    peLocation Location)
{
    utDatabaseShowObject("pe", "Location", peLocation2Index(Location));
}
#endif

/*----------------------------------------------------------------------------------------
  Destroy LocationArray including everything in it. Remove from parents.
----------------------------------------------------------------------------------------*/
void peLocationArrayDestroy(
    peLocationArray LocationArray)
{
    if(peLocationArrayDestructorCallback != NULL) {
        peLocationArrayDestructorCallback(LocationArray);
    }
    peLocationArrayFree(LocationArray);
}

/*----------------------------------------------------------------------------------------
  Default constructor wrapper for the database manager.
----------------------------------------------------------------------------------------*/
static uint64 allocLocationArray(void)
{
    peLocationArray LocationArray = peLocationArrayAlloc();

    return peLocationArray2Index(LocationArray);
}

/*----------------------------------------------------------------------------------------
  Destructor wrapper for the database manager.
----------------------------------------------------------------------------------------*/
static void destroyLocationArray(
    uint64 objectIndex)
{
    peLocationArrayDestroy(peIndex2LocationArray((uint32)objectIndex));
}

/*----------------------------------------------------------------------------------------
  Allocate the field arrays of LocationArray.
----------------------------------------------------------------------------------------*/
static void allocLocationArrays(void)
{
    peSetAllocatedLocationArray(2);
    peSetUsedLocationArray(1);
    peSetFirstFreeLocationArray(peLocationArrayNull);
    peLocationArrays.LocationIndex_ = utNewAInitFirst(uint32, (peAllocatedLocationArray()));
    peLocationArrays.NumLocation = utNewAInitFirst(uint32, (peAllocatedLocationArray()));
    peSetUsedLocationArrayLocation(0);
    peSetAllocatedLocationArrayLocation(2);
    peSetFreeLocationArrayLocation(0);
    peLocationArrays.Location = utNewAInitFirst(peLocation, peAllocatedLocationArrayLocation());
    peLocationArrays.UsedLocation = utNewAInitFirst(uint32, (peAllocatedLocationArray()));
    peLocationArrays.FreeList = utNewAInitFirst(peLocationArray, (peAllocatedLocationArray()));
}

/*----------------------------------------------------------------------------------------
  Realloc the arrays of properties for class LocationArray.
----------------------------------------------------------------------------------------*/
static void reallocLocationArrays(
    uint32 newSize)
{
    utResizeArray(peLocationArrays.LocationIndex_, (newSize));
    utResizeArray(peLocationArrays.NumLocation, (newSize));
    utResizeArray(peLocationArrays.UsedLocation, (newSize));
    utResizeArray(peLocationArrays.FreeList, (newSize));
    peSetAllocatedLocationArray(newSize);
}

/*----------------------------------------------------------------------------------------
  Allocate more LocationArrays.
----------------------------------------------------------------------------------------*/
void peLocationArrayAllocMore(void)
{
    reallocLocationArrays((uint32)(peAllocatedLocationArray() + (peAllocatedLocationArray() >> 1)));
}

/*----------------------------------------------------------------------------------------
  Compact the LocationArray.Location heap to free memory.
----------------------------------------------------------------------------------------*/
void peCompactLocationArrayLocations(void)
{
    uint32 elementSize = sizeof(peLocation);
    uint32 usedHeaderSize = (sizeof(peLocationArray) + elementSize - 1)/elementSize;
    uint32 freeHeaderSize = (sizeof(peLocationArray) + sizeof(uint32) + elementSize - 1)/elementSize;
    peLocation *toPtr = peLocationArrays.Location;
    peLocation *fromPtr = toPtr;
    peLocationArray LocationArray;
    uint32 size;

    while(fromPtr < peLocationArrays.Location + peUsedLocationArrayLocation()) {
        LocationArray = *(peLocationArray *)(void *)fromPtr;
        if(LocationArray != peLocationArrayNull) {
            /* Need to move it to toPtr */
            size = utMax(peLocationArrayGetNumLocation(LocationArray) + usedHeaderSize, freeHeaderSize);
            memmove((void *)toPtr, (void *)fromPtr, size*elementSize);
            peLocationArraySetLocationIndex_(LocationArray, toPtr - peLocationArrays.Location + usedHeaderSize);
            toPtr += size;
        } else {
            /* Just skip it */
            size = utMax(*(uint32 *)(void *)(((peLocationArray *)(void *)fromPtr) + 1), freeHeaderSize);
        }
        fromPtr += size;
    }
    peSetUsedLocationArrayLocation(toPtr - peLocationArrays.Location);
    peSetFreeLocationArrayLocation(0);
}

/*----------------------------------------------------------------------------------------
  Allocate more memory for the LocationArray.Location heap.
----------------------------------------------------------------------------------------*/
static void allocMoreLocationArrayLocations(
    uint32 spaceNeeded)
{
    uint32 freeSpace = peAllocatedLocationArrayLocation() - peUsedLocationArrayLocation();
    uint32 elementSize = sizeof(peLocation);
    uint32 usedHeaderSize = (sizeof(peLocationArray) + elementSize - 1)/elementSize;
    uint32 freeHeaderSize = (sizeof(peLocationArray) + sizeof(uint32) + elementSize - 1)/elementSize;
    peLocation *ptr = peLocationArrays.Location;
    peLocationArray LocationArray;
    uint32 size;

    while(ptr < peLocationArrays.Location + peUsedLocationArrayLocation()) {
        LocationArray = *(peLocationArray*)(void*)ptr;
        if(LocationArray != peLocationArrayNull) {
            peValidLocationArray(LocationArray);
            size = utMax(peLocationArrayGetNumLocation(LocationArray) + usedHeaderSize, freeHeaderSize);
        } else {
            size = utMax(*(uint32 *)(void *)(((peLocationArray *)(void *)ptr) + 1), freeHeaderSize);
        }
        ptr += size;
    }
    if((peFreeLocationArrayLocation() << 2) > peUsedLocationArrayLocation()) {
        peCompactLocationArrayLocations();
        freeSpace = peAllocatedLocationArrayLocation() - peUsedLocationArrayLocation();
    }
    if(freeSpace < spaceNeeded) {
        peSetAllocatedLocationArrayLocation(peAllocatedLocationArrayLocation() + spaceNeeded - freeSpace +
            (peAllocatedLocationArrayLocation() >> 1));
        utResizeArray(peLocationArrays.Location, peAllocatedLocationArrayLocation());
    }
}

/*----------------------------------------------------------------------------------------
  Allocate memory for a new LocationArray.Location array.
----------------------------------------------------------------------------------------*/
void peLocationArrayAllocLocations(
    peLocationArray LocationArray,
    uint32 numLocations)
{
    uint32 freeSpace = peAllocatedLocationArrayLocation() - peUsedLocationArrayLocation();
    uint32 elementSize = sizeof(peLocation);
    uint32 usedHeaderSize = (sizeof(peLocationArray) + elementSize - 1)/elementSize;
    uint32 freeHeaderSize = (sizeof(peLocationArray) + sizeof(uint32) + elementSize - 1)/elementSize;
    uint32 spaceNeeded = utMax(numLocations + usedHeaderSize, freeHeaderSize);

#if defined(DD_DEBUG)
    utAssert(peLocationArrayGetNumLocation(LocationArray) == 0);
#endif
    if(numLocations == 0) {
        return;
    }
    if(freeSpace < spaceNeeded) {
        allocMoreLocationArrayLocations(spaceNeeded);
    }
    peLocationArraySetLocationIndex_(LocationArray, peUsedLocationArrayLocation() + usedHeaderSize);
    peLocationArraySetNumLocation(LocationArray, numLocations);
    *(peLocationArray *)(void *)(peLocationArrays.Location + peUsedLocationArrayLocation()) = LocationArray;
    {
        uint32 xValue;
        for(xValue = (uint32)(peLocationArrayGetLocationIndex_(LocationArray)); xValue < peLocationArrayGetLocationIndex_(LocationArray) + numLocations; xValue++) {
            peLocationArrays.Location[xValue] = peLocationNull;
        }
    }
    peSetUsedLocationArrayLocation(peUsedLocationArrayLocation() + spaceNeeded);
}

/*----------------------------------------------------------------------------------------
  Wrapper around peLocationArrayGetLocations for the database manager.
----------------------------------------------------------------------------------------*/
static void *getLocationArrayLocations(
    uint64 objectNumber,
    uint32 *numValues)
{
    peLocationArray LocationArray = peIndex2LocationArray((uint32)objectNumber);

    *numValues = peLocationArrayGetNumLocation(LocationArray);
    return peLocationArrayGetLocations(LocationArray);
}

/*----------------------------------------------------------------------------------------
  Wrapper around peLocationArrayAllocLocations for the database manager.
----------------------------------------------------------------------------------------*/
static void *allocLocationArrayLocations(
    uint64 objectNumber,
    uint32 numValues)
{
    peLocationArray LocationArray = peIndex2LocationArray((uint32)objectNumber);

    peLocationArraySetLocationIndex_(LocationArray, 0);
    peLocationArraySetNumLocation(LocationArray, 0);
    if(numValues == 0) {
        return NULL;
    }
    peLocationArrayAllocLocations(LocationArray, numValues);
    return peLocationArrayGetLocations(LocationArray);
}

/*----------------------------------------------------------------------------------------
  Free memory used by the LocationArray.Location array.
----------------------------------------------------------------------------------------*/
void peLocationArrayFreeLocations(
    peLocationArray LocationArray)
{
    uint32 elementSize = sizeof(peLocation);
    uint32 usedHeaderSize = (sizeof(peLocationArray) + elementSize - 1)/elementSize;
    uint32 freeHeaderSize = (sizeof(peLocationArray) + sizeof(uint32) + elementSize - 1)/elementSize;
    uint32 size = utMax(peLocationArrayGetNumLocation(LocationArray) + usedHeaderSize, freeHeaderSize);
    peLocation *dataPtr = peLocationArrayGetLocations(LocationArray) - usedHeaderSize;

    if(peLocationArrayGetNumLocation(LocationArray) == 0) {
        return;
    }
    *(peLocationArray *)(void *)(dataPtr) = peLocationArrayNull;
    *(uint32 *)(void *)(((peLocationArray *)(void *)dataPtr) + 1) = size;
    peLocationArraySetNumLocation(LocationArray, 0);
    peSetFreeLocationArrayLocation(peFreeLocationArrayLocation() + size);
}

/*----------------------------------------------------------------------------------------
  Resize the LocationArray.Location array.
----------------------------------------------------------------------------------------*/
void peLocationArrayResizeLocations(
    peLocationArray LocationArray,
    uint32 numLocations)
{
    uint32 freeSpace;
    uint32 elementSize = sizeof(peLocation);
    uint32 usedHeaderSize = (sizeof(peLocationArray) + elementSize - 1)/elementSize;
    uint32 freeHeaderSize = (sizeof(peLocationArray) + sizeof(uint32) + elementSize - 1)/elementSize;
    uint32 newSize = utMax(numLocations + usedHeaderSize, freeHeaderSize);
    uint32 oldSize = utMax(peLocationArrayGetNumLocation(LocationArray) + usedHeaderSize, freeHeaderSize);
    peLocation *dataPtr;

    if(numLocations == 0) {
        if(peLocationArrayGetNumLocation(LocationArray) != 0) {
            peLocationArrayFreeLocations(LocationArray);
        }
        return;
    }
    if(peLocationArrayGetNumLocation(LocationArray) == 0) {
        peLocationArrayAllocLocations(LocationArray, numLocations);
        return;
    }
    freeSpace = peAllocatedLocationArrayLocation() - peUsedLocationArrayLocation();
    if(freeSpace < newSize) {
        allocMoreLocationArrayLocations(newSize);
    }
    dataPtr = peLocationArrayGetLocations(LocationArray) - usedHeaderSize;
    memcpy((void *)(peLocationArrays.Location + peUsedLocationArrayLocation()), dataPtr,
        elementSize*utMin(oldSize, newSize));
    if(newSize > oldSize) {
        {
            uint32 xValue;
            for(xValue = (uint32)(peUsedLocationArrayLocation() + oldSize); xValue < peUsedLocationArrayLocation() + oldSize + newSize - oldSize; xValue++) {
                peLocationArrays.Location[xValue] = peLocationNull;
            }
        }
    }
    *(peLocationArray *)(void *)dataPtr = peLocationArrayNull;
    *(uint32 *)(void *)(((peLocationArray *)(void *)dataPtr) + 1) = oldSize;
    peSetFreeLocationArrayLocation(peFreeLocationArrayLocation() + oldSize);
    peLocationArraySetLocationIndex_(LocationArray, peUsedLocationArrayLocation() + usedHeaderSize);
    peLocationArraySetNumLocation(LocationArray, numLocations);
    peSetUsedLocationArrayLocation(peUsedLocationArrayLocation() + newSize);
}

/*----------------------------------------------------------------------------------------
  Copy the properties of LocationArray.
----------------------------------------------------------------------------------------*/
void peLocationArrayCopyProps(
    peLocationArray oldLocationArray,
    peLocationArray newLocationArray)
{
}

/*----------------------------------------------------------------------------------------
  Add the indexed Location to the LocationArray.
----------------------------------------------------------------------------------------*/
void peLocationArrayInsertLocation(
    peLocationArray LocationArray,
    uint32 x,
    peLocation _Location)
{
#if defined(DD_DEBUG)
    if(LocationArray == peLocationArrayNull) {
        utExit("Non existent LocationArray");
    }
#endif
    peLocationArraySetiLocation(LocationArray, x, _Location);
    peLocationArraySetUsedLocation(LocationArray, utMax(peLocationArrayGetUsedLocation(LocationArray), x + 1));
}

/*----------------------------------------------------------------------------------------
  Add the Location to the end of the LocationArrayLocation array.
----------------------------------------------------------------------------------------*/
void peLocationArrayAppendLocation(
    peLocationArray LocationArray,
    peLocation _Location)
{
    uint32 usedLocation = peLocationArrayGetUsedLocation(LocationArray);

#if defined(DD_DEBUG)
    if(LocationArray == peLocationArrayNull) {
        utExit("Non existent LocationArray");
    }
#endif
    if(usedLocation >= peLocationArrayGetNumLocation(LocationArray)) {
        peLocationArrayResizeLocations(LocationArray, usedLocation + (usedLocation << 1) + 1);
    }
    peLocationArraySetiLocation(LocationArray, usedLocation, _Location);
    peLocationArraySetUsedLocation(LocationArray, usedLocation + 1);
}

#if defined(DD_DEBUG)
/*----------------------------------------------------------------------------------------
  Write out all the fields of an object.
----------------------------------------------------------------------------------------*/
void peShowLocationArray(
    peLocationArray LocationArray)
{
    utDatabaseShowObject("pe", "LocationArray", peLocationArray2Index(LocationArray));
}
#endif

/*----------------------------------------------------------------------------------------
  Free memory used by the pe database.
----------------------------------------------------------------------------------------*/
void peDatabaseStop(void)
{
    utFree(peRoots.LocationIndex_);
    utFree(peRoots.NumLocation);
    utFree(peRoots.Location);
    utFree(peRoots.UsedLocation);
    utFree(pePebbles.Location);
    utFree(peLocations.NumPointers);
    utFree(peLocations.Visited);
    utFree(peLocations.Root);
    utFree(peLocations.RootIndex);
    utFree(peLocations.Pebble);
    utFree(peLocations.Location);
    utFree(peLocations.FirstLocation);
    utFree(peLocations.NextLocationLocation);
    utFree(peLocationArrays.LocationIndex_);
    utFree(peLocationArrays.NumLocation);
    utFree(peLocationArrays.Location);
    utFree(peLocationArrays.UsedLocation);
    utFree(peLocationArrays.FreeList);
    utUnregisterModule(peModuleID);
}

/*----------------------------------------------------------------------------------------
  Allocate memory used by the pe database.
----------------------------------------------------------------------------------------*/
void peDatabaseStart(void)
{
    if(!utInitialized()) {
        utStart();
    }
    peRootData.hash = 0x347e8a4b;
    peModuleID = utRegisterModule("pe", false, peHash(), 4, 18, 0, sizeof(struct peRootType_),
        &peRootData, peDatabaseStart, peDatabaseStop);
    utRegisterClass("Root", 4, &peRootData.usedRoot, &peRootData.allocatedRoot,
        NULL, 65535, 4, allocRoot, NULL);
    utRegisterField("LocationIndex_", &peRoots.LocationIndex_, sizeof(uint32), UT_UINT, NULL);
    utSetFieldHidden();
    utRegisterField("NumLocation", &peRoots.NumLocation, sizeof(uint32), UT_UINT, NULL);
    utSetFieldHidden();
    utRegisterField("Location", &peRoots.Location, sizeof(peLocation), UT_POINTER, "Location");
    utRegisterArray(&peRootData.usedRootLocation, &peRootData.allocatedRootLocation,
        getRootLocations, allocRootLocations, peCompactRootLocations);
    utRegisterField("UsedLocation", &peRoots.UsedLocation, sizeof(uint32), UT_UINT, NULL);
    utRegisterClass("Pebble", 1, &peRootData.usedPebble, &peRootData.allocatedPebble,
        NULL, 65535, 4, allocPebble, NULL);
    utRegisterField("Location", &pePebbles.Location, sizeof(peLocation), UT_POINTER, "Location");
    utRegisterClass("Location", 8, &peRootData.usedLocation, &peRootData.allocatedLocation,
        NULL, 65535, 4, allocLocation, NULL);
    utRegisterField("NumPointers", &peLocations.NumPointers, sizeof(uint32), UT_UINT, NULL);
    utRegisterField("Visited", &peLocations.Visited, sizeof(uint8), UT_BOOL, NULL);
    utRegisterField("Root", &peLocations.Root, sizeof(peRoot), UT_POINTER, "Root");
    utRegisterField("RootIndex", &peLocations.RootIndex, sizeof(uint32), UT_UINT, NULL);
    utRegisterField("Pebble", &peLocations.Pebble, sizeof(pePebble), UT_POINTER, "Pebble");
    utRegisterField("Location", &peLocations.Location, sizeof(peLocation), UT_POINTER, "Location");
    utRegisterField("FirstLocation", &peLocations.FirstLocation, sizeof(peLocation), UT_POINTER, "Location");
    utRegisterField("NextLocationLocation", &peLocations.NextLocationLocation, sizeof(peLocation), UT_POINTER, "Location");
    utRegisterClass("LocationArray", 5, &peRootData.usedLocationArray, &peRootData.allocatedLocationArray,
        &peRootData.firstFreeLocationArray, 17, 4, allocLocationArray, destroyLocationArray);
    utRegisterField("LocationIndex_", &peLocationArrays.LocationIndex_, sizeof(uint32), UT_UINT, NULL);
    utSetFieldHidden();
    utRegisterField("NumLocation", &peLocationArrays.NumLocation, sizeof(uint32), UT_UINT, NULL);
    utSetFieldHidden();
    utRegisterField("Location", &peLocationArrays.Location, sizeof(peLocation), UT_POINTER, "Location");
    utRegisterArray(&peRootData.usedLocationArrayLocation, &peRootData.allocatedLocationArrayLocation,
        getLocationArrayLocations, allocLocationArrayLocations, peCompactLocationArrayLocations);
    utRegisterField("UsedLocation", &peLocationArrays.UsedLocation, sizeof(uint32), UT_UINT, NULL);
    utRegisterField("FreeList", &peLocationArrays.FreeList, sizeof(peLocationArray), UT_POINTER, "LocationArray");
    utSetFieldHidden();
    allocRoots();
    allocPebbles();
    allocLocations();
    allocLocationArrays();
}


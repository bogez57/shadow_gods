#ifndef MEMHANDLING_INCLUDE_H
#define MEMHANDLING_INCLUDE_H

#include <stdint.h>
#include <assert.h>

#define ASSERT(x) assert(x)

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef i32 b32;
typedef bool b;

typedef uint8_t ui8;
typedef uint16_t ui16;
typedef uint32_t ui32;
typedef uint64_t ui64;

typedef size_t sizet;

typedef float f32;
typedef double f64;

enum Allocator_Type
{
    DYNAMIC
};

struct Memory_Partition
{
    void* BaseAddress;
    void* EndAddress;
    i64 UsedAmount;
    i64 Size;
    Allocator_Type allocatorType;
};

struct Application_Memory
{
    bool Initialized { false };

    void* PermanentStorage { nullptr };
    void* TemporaryStorage { nullptr };

    i32 SizeOfPermanentStorage {};
    i64 SizeOfTemporaryStorage {};

    i64 TemporaryStorageUsed {};
    i64 TotalSize {};
    Memory_Partition partitions[10];
    i32 regionCount {};
};

void InitApplicationMemory(Application_Memory* userDefinedAppMemoryStruct);
void InitApplicationMemory(Application_Memory* userDefinedAppMemoryStruct, ui64 sizeOfMemory, ui32 sizeOfPermanentStore, void* memoryStartAddress);

i32 CreatePartitionFromMemoryBlock(Application_Memory* Memory, i64 size, Allocator_Type allocatorType);

i64 MemoryRegionSize(i32 memRegionID);

#endif

#ifdef MEMORY_HANDLING_IMPL

//TODO: Alignment
void* _PointerAddition(void* baseAddress, ui64 amountToAdvancePointer)
{
    void* newAddress {};

    newAddress = ((ui8*)baseAddress) + amountToAdvancePointer;

    return newAddress;
};

static Application_Memory* appMemory {};

void InitApplicationMemory(Application_Memory* userDefinedAppMemoryStruct)
{
    appMemory = userDefinedAppMemoryStruct;
};

void InitApplicationMemory(Application_Memory* userDefinedAppMemoryStruct, ui64 sizeOfMemory, ui32 sizeOfPermanentStore, void* memoryStartAddress)
{
    appMemory = userDefinedAppMemoryStruct;

    ui32 sizeOfPermanentStorage = sizeOfPermanentStore;
    appMemory->SizeOfPermanentStorage = sizeOfPermanentStorage;
    appMemory->SizeOfTemporaryStorage = sizeOfMemory - (ui64)sizeOfPermanentStorage;
    appMemory->TotalSize = sizeOfMemory;
    appMemory->PermanentStorage = memoryStartAddress;
    appMemory->TemporaryStorage = ((ui8*)appMemory->PermanentStorage + appMemory->SizeOfPermanentStorage);
    appMemory->regionCount = 0;
};

i32 CreatePartitionFromMemoryBlock(Application_Memory* appMemory, i64 size, Allocator_Type allocatorType)
{
    ASSERT(size < appMemory->SizeOfTemporaryStorage);
    ASSERT((size + appMemory->TemporaryStorageUsed) < appMemory->SizeOfTemporaryStorage);

    appMemory->Initialized = true;

    Memory_Partition* memRegion = &appMemory->partitions[appMemory->regionCount];
    memRegion->BaseAddress = _PointerAddition(appMemory->TemporaryStorage, appMemory->TemporaryStorageUsed);
    memRegion->EndAddress = _PointerAddition(memRegion->BaseAddress, (size - 1));
    memRegion->Size = size;
    memRegion->UsedAmount = 0;
    memRegion->allocatorType = allocatorType;

    appMemory->TemporaryStorageUsed += size;

    //TODO: Since region count acts as region identifer, we can't currently let user allocators
    //be randomly destroyed (E.g. user might destory allocator tied to memory region 2 in a 
    //list of 5 partitions. With current scheme we would only be able to decrement regionCount so 
    //memory region 5 would dissapear, even though that's not the correct memory region to destroy.
    return appMemory->regionCount++;
};

i64 MemoryRegionSize(i32 memRegionID)
{
   Memory_Partition region = appMemory->partitions[memRegionID];
   return region.Size;
};

auto _AllocSize(i32 MemRegionIdentifier, i64 size) -> void*
{
    ASSERT((appMemory->partitions[MemRegionIdentifier].UsedAmount + size) <= appMemory->partitions[MemRegionIdentifier].Size);
    void* Result = _PointerAddition(appMemory->partitions[MemRegionIdentifier].BaseAddress, appMemory->partitions[MemRegionIdentifier].UsedAmount);
    appMemory->partitions[MemRegionIdentifier].UsedAmount += (size);

    return Result;
};

auto _FreeSize(i32 MemRegionIdentifier, i64 sizeToFree) -> void
{
    ASSERT(sizeToFree < appMemory->partitions[MemRegionIdentifier].Size);
    ASSERT(sizeToFree < appMemory->partitions[MemRegionIdentifier].UsedAmount);

    appMemory->partitions[MemRegionIdentifier].UsedAmount -= sizeToFree;
};

#endif //MEMORY_HANDLING_IMPL
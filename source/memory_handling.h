#ifndef MEMHANDLING_INCLUDE
#define MEMHANDLING_INCLUDE

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

struct Memory_Partition
{
    void* baseAddress;
    i64 usedAmount;
    i64 size;
    i32 tempMemoryCount {}; //Number of active temporary memory sub partitions (created w/ BeginTemporaryMemory())
};

struct Application_Memory
{
    bool initialized { false };
    void* permanentStorage { nullptr };
    void* temporaryStorage { nullptr };
    i32 sizeOfPermanentStorage {};
    i64 sizeOfTemporaryStorage {};
    i64 temporaryStorageUsed {};
    i64 totalSize {};
    Memory_Partition partitions[10];
    i32 partitionCount {};
};

void* _AllocSize(Memory_Partition&& memPartition, i64 size);
void _Release(Memory_Partition&& memPartition);
#define PushType(memPartition, type, count) (type*)_AllocSize($(memPartition), ((sizeof(type)) * (count)))
#define Release(memPartition) _Release($(memPartition));

void InitApplicationMemory(Application_Memory* userDefinedAppMemoryStruct, i64 sizeOfMemory, i32 sizeOfPermanentStore, void* memoryStartAddress);
Memory_Partition* CreatePartitionFromMemoryBlock(Application_Memory&& Memory, i64 size);

#endif

#ifdef MEMORY_HANDLING_IMPL

void InitApplicationMemory(Application_Memory* appMemory, i64 sizeOfMemory, i32 sizeOfPermanentStore, void* memoryStartAddress)
{
    appMemory->sizeOfPermanentStorage = sizeOfPermanentStore;
    appMemory->sizeOfTemporaryStorage = sizeOfMemory - (i64)sizeOfPermanentStore;
    appMemory->totalSize = sizeOfMemory;
    appMemory->permanentStorage = memoryStartAddress;
    appMemory->temporaryStorage = ((ui8*)appMemory->permanentStorage + appMemory->sizeOfPermanentStorage);
};

//TODO: Alignment
void* _PointerAddition(void* baseAddress, i64 amountToAdvancePointer)
{
    void* newAddress {};
    newAddress = ((ui8*)baseAddress) + amountToAdvancePointer;
    return newAddress;
};

Memory_Partition* CreatePartitionFromMemoryBlock(Application_Memory&& appMemory, i64 size)
{
    ASSERT(size < appMemory.sizeOfTemporaryStorage);
    ASSERT((size + appMemory.temporaryStorageUsed) < appMemory.sizeOfTemporaryStorage);

    Memory_Partition memPartition {};
    memPartition.baseAddress = _PointerAddition(appMemory.temporaryStorage, appMemory.temporaryStorageUsed);
    memPartition.size = size;
    memPartition.usedAmount = 0;

    appMemory.partitions[appMemory.partitionCount] = memPartition;
    appMemory.temporaryStorageUsed += size;

    return &appMemory.partitions[appMemory.partitionCount++];
};

Memory_Partition* GetMemoryPartition(Application_Memory* appMemory, i32 memPartitionID)
{
    return &appMemory->partitions[memPartitionID];
};

auto _AllocSize(Memory_Partition&& memPartition, i64 size) -> void*
{
    ASSERT((memPartition.usedAmount + size) <= memPartition.size);
    void* Result = _PointerAddition(memPartition.baseAddress, memPartition.usedAmount);
    memPartition.usedAmount += (size);

    return Result;
};

auto _FreeSize(Memory_Partition&& memPartition, i64 sizeToFree) -> void
{
    ASSERT(sizeToFree < memPartition.size);
    ASSERT(sizeToFree < memPartition.usedAmount);

    memPartition.usedAmount -= sizeToFree;
};

struct Temporary_Memory
{
    Memory_Partition* memPartition {};
    i64 initialusedAmountFromMemPartition {};
};

Temporary_Memory BeginTemporaryMemory(Memory_Partition&& memPartition)
{
    Temporary_Memory result;

    result.memPartition = &memPartition;
    result.initialusedAmountFromMemPartition = memPartition.usedAmount;

    ++memPartition.tempMemoryCount;

    return (result);
}

void EndTemporaryMemory(Temporary_Memory TempMem)
{
    Memory_Partition* memPartition = TempMem.memPartition;
    ASSERT(memPartition->usedAmount >= TempMem.initialusedAmountFromMemPartition);

    memPartition->usedAmount = TempMem.initialusedAmountFromMemPartition;

    ASSERT(memPartition->tempMemoryCount > 0);
    --memPartition->tempMemoryCount;
}

void IsAllTempMemoryCleared(Memory_Partition* memPartition)
{
    ASSERT(memPartition->tempMemoryCount == 0);
}

void _Release(Memory_Partition&& memPartition)
{
    memPartition.usedAmount = 0;
    memPartition.tempMemoryCount = 0;
};

#endif //MEMORY_HANDLING_IMPL
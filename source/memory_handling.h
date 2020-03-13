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
    void* BaseAddress;
    void* EndAddress;
    i64 UsedAmount;
    i64 Size;
    i32 tempMemoryCount{};//Number of active temporary memory sub partitions (created w/ BeginTemporaryMemory())
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
};

void* _AllocSize(Memory_Partition&& memPartition, i64 size);
#define PushType(memPartition, type, count) (type*)_AllocSize($(memPartition), ((sizeof(type)) * (count)))

void InitApplicationMemory(Application_Memory* userDefinedAppMemoryStruct, i64 sizeOfMemory, i32 sizeOfPermanentStore, void* memoryStartAddress);
 Memory_Partition CreatePartitionFromMemoryBlock(Application_Memory&& Memory, i64 size);

#endif

#ifdef MEMORY_HANDLING_IMPL

void InitApplicationMemory(Application_Memory* appMemory, i64 sizeOfMemory, i32 sizeOfPermanentStore, void* memoryStartAddress)
{
    appMemory->SizeOfPermanentStorage = sizeOfPermanentStore;
    appMemory->SizeOfTemporaryStorage = sizeOfMemory - (i64)sizeOfPermanentStore;
    appMemory->TotalSize = sizeOfMemory;
    appMemory->PermanentStorage = memoryStartAddress;
    appMemory->TemporaryStorage = ((ui8*)appMemory->PermanentStorage + appMemory->SizeOfPermanentStorage);
};


//TODO: Alignment
void* _PointerAddition(void* baseAddress, i64 amountToAdvancePointer)
{
    void* newAddress {};
    newAddress = ((ui8*)baseAddress) + amountToAdvancePointer;
    return newAddress;
};

 Memory_Partition CreatePartitionFromMemoryBlock(Application_Memory&& appMemory, i64 size)
{
    ASSERT(size < appMemory.SizeOfTemporaryStorage);
    ASSERT((size + appMemory.TemporaryStorageUsed) < appMemory.SizeOfTemporaryStorage);

    Memory_Partition memPartition{};
    memPartition.BaseAddress = _PointerAddition(appMemory.TemporaryStorage, appMemory.TemporaryStorageUsed);
    memPartition.EndAddress = _PointerAddition(memPartition.BaseAddress, (size - 1));
    memPartition.Size = size;
    memPartition.UsedAmount = 0;

    appMemory.TemporaryStorageUsed += size;

    return memPartition;
};

auto _AllocSize(Memory_Partition&& memPartition, i64 size) -> void*
{
    ASSERT((memPartition.UsedAmount + size) <= memPartition.Size);
    void* Result = _PointerAddition(memPartition.BaseAddress, memPartition.UsedAmount);
    memPartition.UsedAmount += (size);

    return Result;
};

auto _FreeSize(Memory_Partition&& memPartition, i64 sizeToFree) -> void
{
    ASSERT(sizeToFree < memPartition.Size);
    ASSERT(sizeToFree < memPartition.UsedAmount);

    memPartition.UsedAmount -= sizeToFree;
};

struct Temporary_Memory
{
    Memory_Partition* memPartition{};
     i64 initialUsedAmountFromMemPartition{};
};

 Temporary_Memory BeginTemporaryMemory(Memory_Partition&& memPartition)
{
    Temporary_Memory result;

    result.memPartition = &memPartition;
    result.initialUsedAmountFromMemPartition = memPartition.UsedAmount;

    ++memPartition.tempMemoryCount;

    return(result);
}

void EndTemporaryMemory(Temporary_Memory TempMem)
{
Memory_Partition *memPartition = TempMem.memPartition;
    ASSERT(memPartition->UsedAmount >= TempMem.initialUsedAmountFromMemPartition);

    memPartition->UsedAmount = TempMem.initialUsedAmountFromMemPartition;

    ASSERT(memPartition->tempMemoryCount > 0);
    --memPartition->tempMemoryCount;
}

void IsAllTempMemoryCleared(Memory_Partition* memPartition)
{
    ASSERT(memPartition->tempMemoryCount == 0);
}

#endif //MEMORY_HANDLING_IMPL
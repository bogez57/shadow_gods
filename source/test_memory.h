#ifndef MEMHANDLING_TEST__INCLUDE
#define MEMHANDLING_TEST_INCLUDE

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
    DYNAMIC,
    LINEAR
};

struct Memory_Partition
{
    void* BaseAddress;
    void* EndAddress;
    i64 UsedAmount;
    i64 Size;
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
    i32 partitionCount {};
};

void InitApplicationMemory(Application_Memory* userDefinedAppMemoryStruct, i64 sizeOfMemory, i32 sizeOfPermanentStore, void* memoryStartAddress);

 Memory_Partition CreatePartitionFromMemoryBlock(Application_Memory* Memory, i64 size);

#endif

#ifdef MEMORY_HANDLING_TEST_IMPL

void InitApplicationMemory(Application_Memory* userDefinedAppMemoryStruct, i64 sizeOfMemory, i32 sizeOfPermanentStore, void* memoryStartAddress)
{
    i32 sizeOfPermanentStorage = sizeOfPermanentStore;
    appMemory->SizeOfPermanentStorage = sizeOfPermanentStorage;
    appMemory->SizeOfTemporaryStorage = sizeOfMemory - (i64)sizeOfPermanentStorage;
    appMemory->TotalSize = sizeOfMemory;
    appMemory->PermanentStorage = memoryStartAddress;
    appMemory->TemporaryStorage = ((ui8*)appMemory->PermanentStorage + appMemory->SizeOfPermanentStorage);
    appMemory->partitionCount = 0;
};

//TODO: Alignment
void* _PointerAddition(void* baseAddress, i64 amountToAdvancePointer)
{
    void* newAddress {};
    newAddress = ((ui8*)baseAddress) + amountToAdvancePointer;
    return newAddress;
};

 Memory_Partition CreatePartitionFromMemoryBlock(Application_Memory* appMemory, i64 size)
{
    ASSERT(size < appMemory->SizeOfTemporaryStorage);
    ASSERT((size + appMemory->TemporaryStorageUsed) < appMemory->SizeOfTemporaryStorage);

    appMemory->Initialized = true;

    Memory_Partition* memPartition = &appMemory->partitions[appMemory->partitionCount];
    memPartition->BaseAddress = _PointerAddition(appMemory->TemporaryStorage, appMemory->TemporaryStorageUsed);
    memPartition->EndAddress = _PointerAddition(memPartition->BaseAddress, (size - 1));
    memPartition->Size = size;
    memPartition->UsedAmount = 0;
    memPartition->allocatorType = allocatorType;

    appMemory->TemporaryStorageUsed += size;
    ++appMemory->partitionCount;

    return *memPartition;
};

auto _AllocSize(i32 MemPartitionID, i64 size) -> void*
{
    ASSERT((appMemory->partitions[MemPartitionID].UsedAmount + size) <= appMemory->partitions[MemPartitionID].Size);
    void* Result = _PointerAddition(appMemory->partitions[MemPartitionID].BaseAddress, appMemory->partitions[MemPartitionID].UsedAmount);
    appMemory->partitions[MemPartitionID].UsedAmount += (size);

    return Result;
};

auto _FreeSize(i32 MemPartitionID, i64 sizeToFree) -> void
{
    ASSERT(sizeToFree < appMemory->partitions[MemPartitionID].Size);
    ASSERT(sizeToFree < appMemory->partitions[MemPartitionID].UsedAmount);

    appMemory->partitions[MemPartitionID].UsedAmount -= sizeToFree;
};

#endif //MEMORY_HANDLING_IMPL

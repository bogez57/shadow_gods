#ifndef MEMHANDLING_TEST__INCLUDE
#define MEMHANDLING_TEST_INCLUDE

#include <stdint.h>
#include <assert.h>

#define ASSERT(x) assert(x)

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef s32 b32;
typedef bool  b;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

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
    s64 UsedAmount;
    s64 Size;
};

struct Application_Memory
{
    bool  Initialized { false };
    
    void* PermanentStorage { nullptr };
    void* TemporaryStorage { nullptr };
    
    s32 SizeOfPermanentStorage {};
    s64 SizeOfTemporaryStorage {};
    
    s64 TemporaryStorageUsed {};
    s64 TotalSize {};
    Memory_Partition partitions[10];
    s32 partitionCount {};
};

void InitApplicationMemory(Application_Memory* userDefinedAppMemoryStruct, s64 sizeOfMemory, s32 sizeOfPermanentStore, void* memoryStartAddress);

Memory_Partition CreatePartitionFromMemoryBlock(Application_Memory* Memory, s64 size);

#endif

#ifdef MEMORY_HANDLING_TEST_IMPL

void InitApplicationMemory(Application_Memory* userDefinedAppMemoryStruct, s64 sizeOfMemory, s32 sizeOfPermanentStore, void* memoryStartAddress)
{
    s32 sizeOfPermanentStorage = sizeOfPermanentStore;
    appMemory->SizeOfPermanentStorage = sizeOfPermanentStorage;
    appMemory->SizeOfTemporaryStorage = sizeOfMemory - (s64)sizeOfPermanentStorage;
    appMemory->TotalSize = sizeOfMemory;
    appMemory->PermanentStorage = memoryStartAddress;
    appMemory->TemporaryStorage = ((u8*)appMemory->PermanentStorage + appMemory->SizeOfPermanentStorage);
    appMemory->partitionCount = 0;
};

//TODO: Alignment
void* _PointerAddition(void* baseAddress, s64 amountToAdvancePointer)
{
    void* newAddress {};
    newAddress = ((u8*)baseAddress) + amountToAdvancePointer;
    return newAddress;
};

Memory_Partition CreatePartitionFromMemoryBlock(Application_Memory* appMemory, s64 size)
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

auto _AllocSize(s32 MemPartitionID, s64 size) -> void*
{
    ASSERT((appMemory->partitions[MemPartitionID].UsedAmount + size) <= appMemory->partitions[MemPartitionID].Size);
    void* Result = _PointerAddition(appMemory->partitions[MemPartitionID].BaseAddress, appMemory->partitions[MemPartitionID].UsedAmount);
    appMemory->partitions[MemPartitionID].UsedAmount += (size);
    
    return Result;
};

auto _FreeSize(s32 MemPartitionID, s64 sizeToFree) -> void
{
    ASSERT(sizeToFree < appMemory->partitions[MemPartitionID].Size);
    ASSERT(sizeToFree < appMemory->partitions[MemPartitionID].UsedAmount);
    
    appMemory->partitions[MemPartitionID].UsedAmount -= sizeToFree;
};

#endif //MEMORY_HANDLING_IMPL

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

struct _PartitionMap
{
    Memory_Partition partitions[10];
    i32 keys[10];
    i32 currentCount{};
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
    _PartitionMap partitionMap{};
};

void* _AllocSize(Memory_Partition* memPartition, i64 size);
#define PushType(memPartition, type, count) (type*)_AllocSize(memPartition, ((sizeof(type)) * (count)))
void Release(Memory_Partition&& memPartition);

void InitApplicationMemory(Application_Memory* userDefinedAppMemoryStruct, i64 sizeOfMemory, i32 sizeOfPermanentStore, void* memoryStartAddress);
Memory_Partition* CreatePartitionFromMemoryBlock(Application_Memory&& appMemory, i64 size, const char* partName);

#endif

#ifdef MEMORY_HANDLING_IMPL

void _InsertPartition(_PartitionMap&& partMap, const char* partName, Memory_Partition memPartToInsert)
{
    i32 uniqueID {};//TODO: Using this method to come up with keys isn't full proof. Could have a name with same letters in different order and it would produce conflicting keys. Prob need to change.
    for (i32 i {}; partName[i] != 0; ++i)
        uniqueID += partName[i];
    
    partMap.keys[partMap.currentCount] = uniqueID;
    partMap.partitions[partMap.currentCount++] = memPartToInsert;
};

Memory_Partition* _GetPartition(_PartitionMap* partMap, const char* partName)
{
    i32 uniqueID {};
    for (i32 i {}; partName[i] != 0; ++i)
        uniqueID += partName[i];
    
    i32 keyIndex { -1 };
    for (i32 i {}; i < partMap->currentCount; ++i)
    {
        if (uniqueID == partMap->keys[i])
        {
            keyIndex = i;
            break;
        }
    };
    
    if (keyIndex == -1)
        BGZ_ASSERT(1 < 0, "Partition name is either incorrect or requested partition doesn't exist!");
    
    return &partMap->partitions[keyIndex];
};

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

Memory_Partition* CreatePartitionFromMemoryBlock(Application_Memory&& appMemory, i64 size, const char* partName)
{
    ASSERT(size < appMemory.sizeOfTemporaryStorage);
    ASSERT((size + appMemory.temporaryStorageUsed) < appMemory.sizeOfTemporaryStorage);
    
    Memory_Partition memPartition {};
    memPartition.baseAddress = _PointerAddition(appMemory.temporaryStorage, appMemory.temporaryStorageUsed);
    memPartition.size = size;
    memPartition.usedAmount = 0;
    
    appMemory.temporaryStorageUsed += size;
    _InsertPartition($(appMemory.partitionMap), partName, memPartition);
    
    return _GetPartition(&appMemory.partitionMap, partName);
};

Memory_Partition* GetMemoryPartition(Application_Memory* appMemory, const char* partName)
{
    return _GetPartition(&appMemory->partitionMap, partName);
};

auto _AllocSize(Memory_Partition* memPartition, i64 size) -> void*
{
    ASSERT((memPartition->usedAmount + size) <= memPartition->size);
    void* Result = _PointerAddition(memPartition->baseAddress, memPartition->usedAmount);
    memPartition->usedAmount += (size);
    
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

void Release(Memory_Partition&& memPartition)
{
    memPartition.usedAmount = 0;
    memPartition.tempMemoryCount = 0;
};

#endif //MEMORY_HANDLING_IMPL
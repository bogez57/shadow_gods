#ifndef LINEAR_ALLOCATOR_H 
#define LINEAR_ALLOCATOR_H 

#include "allocator.h"
#include "boagz/error_handling.h"

void* _PushSize(i32 memPartitionID, i64 size);
#define PushType(memPartitionID, type, count) (type*)_PushSize(memPartitionID, ((sizeof(type)) * (count)))
#define PushSize(memPartitionID, size) _PushSize(memPartitionID, size)
void Release(i32 memPartitionID);

#endif

#ifdef LINEAR_ALLOCATOR_IMPL

struct _Linear_Allocator
{
    void* baseAddress;
    i64 usedAmount;
    i64 size;
};

_Linear_Allocator linearAllocators[10] = {};

void InitLinearAllocator(i32 memPartitionID)
{
    ASSERT(appMemory->partitions[memPartitionID].allocatorType == LINEAR);

    linearAllocators[memPartitionID].baseAddress = _AllocSize(memPartitionID, MemoryPartitionSize(memPartitionID));
    linearAllocators[memPartitionID].size = MemoryPartitionSize(memPartitionID);
    linearAllocators[memPartitionID].usedAmount = 0;
};

void* _PushSize(i32 memPartitionID, i64 size) 
{
    BGZ_ASSERT((linearAllocators[memPartitionID].usedAmount + size) <= linearAllocators[memPartitionID].size, "Not enough space in linear allocator to allocate requested size");
    ASSERT(appMemory->partitions[memPartitionID].allocatorType == LINEAR);

    void* memoryPointer = ((ui8*)linearAllocators[memPartitionID].baseAddress) + linearAllocators[memPartitionID].usedAmount;
    linearAllocators[memPartitionID].usedAmount += (size);

    return memoryPointer;
};

void Release(i32 memPartitionID) 
{
    ASSERT(appMemory->partitions[memPartitionID].allocatorType == LINEAR);

    linearAllocators[memPartitionID].usedAmount = 0;
};

#endif LINEAR_ALLOCATOR_IMPL
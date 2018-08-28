#pragma once
#include <string.h>
#include "list.h"


struct Block_Header
{
    b IsFree{true};
    sizet Size{0};
    void* nextBlock{nullptr};
    void* prevBlock{nullptr};
};

struct Memory_Partition
{
    ui64* BaseAddress;
    ui64* EndAddress;
    ui64 UsedAmount;
    ui64 Size;
};

class Memory_Allocator
{
public:
    Memory_Allocator(ui64* BaseAddress, ui64 SizeOfMemoryStore) :
        BaseAddress(BaseAddress),
        TotalMemorySize(SizeOfMemoryStore),
        UsedAmount(0)
    {
        for (ui32 PartitionIndex{0}; PartitionIndex < ArrayCount(this->Partitions); ++PartitionIndex)
        {
            this->Partitions[PartitionIndex] = nullptr;
        };
    };

    ui64* BaseAddress;
    ui64 UsedAmount;
    ui64 TotalMemorySize;
    Memory_Partition* Partitions[5];
};

class Dynamic_Allocator : public Memory_Allocator
{
public:
    Dynamic_Allocator(ui64* BaseAddress, ui64 SizeOfMemoryStore) :
        Memory_Allocator(BaseAddress, SizeOfMemoryStore)
    {};

    List* FreeBlocks;
    List* FilledBlocks;

    auto AddPartition(Memory_Partition* MemPartition) -> void;
};

auto
Dynamic_Allocator::AddPartition(Memory_Partition* MemPartition) -> void
{
    BGZ_ASSERT((this->TotalMemorySize - this->UsedAmount) > MemPartition->Size);

    for(ui32 PartitionIndex{0}; PartitionIndex < ArrayCount(this->Partitions); ++PartitionIndex)
    {
        if(this->Partitions[PartitionIndex] != nullptr)
        {
            MemPartition->BaseAddress = this->BaseAddress + this->UsedAmount;
            this->UsedAmount += MemPartition->Size;
            this->Partitions[PartitionIndex] = MemPartition;
            return;
        };
    };
};

#define MallocType(Type, Count) (Type*)_MallocType(&GlobalGameState->SpineData, ((sizeof(Type)) * (Count)))
#define MallocSize(Size) _MallocType(&GlobalGameState->SpineData, (Size))
#define CallocType(Type, Count) (Type*)_CallocType(&GlobalGameState->SpineData, ((sizeof(Type)) * (Count)))
#define CallocSize(Type, Count) _CallocType(&GlobalGameState->SpineData, (Size))
#define ReAlloc(Ptr, Type, Count) (Type*)_ReAlloc(&GlobalGameState->SpineData, Ptr, sizeof(Type) * Count)
#define DeAlloc(PtrToMemory) _DeAlloc(&GlobalGameState->SpineData, (ui64**)PtrToMemory)


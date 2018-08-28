#pragma once
#include <string.h>
#include "list.h"

global_variable List* FreeBlocks;
global_variable List* FilledBlocks;

struct Block_Header
{
    b IsFree{true};
    sizet Size{0};
    void* nextBlock{nullptr};
    void* prevBlock{nullptr};
};

struct Memory_Sub_Partition
{
    ui64* BaseAddress;
    ui64* EndAddress;
    ui64 UsedAmount;
    ui64 Size;
};

struct Memory_Partition
{
    ui64* BaseAddress{nullptr};
    ui64 Size{0};
    ui64 UsedAmount{0};
    Memory_Sub_Partition* SubPartitions[5];
};

#define MallocType(Type, Count) (Type*)_MallocType(&GlobalGameState->SpineData, ((sizeof(Type)) * (Count)))
#define MallocSize(Size) _MallocType(&GlobalGameState->SpineData, (Size))
#define CallocType(Type, Count) (Type*)_CallocType(&GlobalGameState->SpineData, ((sizeof(Type)) * (Count)))
#define CallocSize(Type, Count) _CallocType(&GlobalGameState->SpineData, (Size))
#define ReAlloc(Ptr, Type, Count) (Type*)_ReAlloc(&GlobalGameState->SpineData, Ptr, sizeof(Type) * Count)
#define DeAlloc(PtrToMemory) _DeAlloc(&GlobalGameState->SpineData, (ui64**)PtrToMemory)


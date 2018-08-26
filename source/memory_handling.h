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
    ui64* BaseAddress{nullptr};
    ui64* EndAddress{nullptr};
    ui64 Size{0};
    ui64 UsedAmount{0};
    List* FreeBlocks{};
    List* FilledBlocks{};
};

#define MallocType(Type, Count) (Type*)_MallocType(&GlobalGameState->DynamicMem, ((sizeof(Type)) * (Count)))
#define MallocSize(Size) _MallocType(&GlobalGameState->DynamicMem, (Size))
#define ReAlloc(Ptr, Type, Count) (Type*)_ReAlloc(&GlobalGameState->DynamicMem, Ptr, sizeof(Type) * Count)
#define DeAlloc(PtrToMemory) _DeAlloc(&GlobalGameState->DynamicMem, (ui64**)PtrToMemory)


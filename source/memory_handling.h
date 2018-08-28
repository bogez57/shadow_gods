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

struct Memory_Region
{
    ui64* BaseAddress;
    ui64* EndAddress;
    ui64 UsedAmount;
    ui64 Size;
};

enum Mem_Region_Type
{
    SPINE,
    REGION_COUNT
};

struct Dynamic_Mem_Allocator 
{
    Memory_Region MemRegions[REGION_COUNT];

    List* FreeBlocks;
    List* FilledBlocks;
};

local_func auto
CreateRegionFromPlatformMem(Game_Memory* GameMemory, ui64 Size) -> Memory_Region 
{
    Memory_Region Result{};

    Result.BaseAddress = (ui64*)((ui8*)GameMemory->TemporaryStorage) + GameMemory->TemporaryStorageUsed;
    Result.EndAddress = (ui64*)((ui8*)Result.BaseAddress) + (Size - 1);
    Result.Size = Size;
    Result.UsedAmount = 0;
    GameMemory->TemporaryStorageUsed += Size;

    return Result;
};

#define MallocType(Type, Count) (Type*)_MallocType(&GlobalGameState->DynamAllocator, ((sizeof(Type)) * (Count)), SPINE)
#define MallocSize(Size) _MallocType(&GlobalGameState->DynamicAllocator, (Size), SPINE)
#define CallocType(Type, Count) (Type*)_CallocType(&GlobalGameState->DynamAllocator, ((sizeof(Type)) * (Count)), SPINE)
#define CallocSize(Type, Count) _CallocType(&GlobalGameState->DynamAllocator, (Size), SPINE)
#define ReAlloc(Ptr, Type, Count) (Type*)_ReAlloc(&GlobalGameState->DynamAllocator, Ptr, sizeof(Type) * Count, SPINE)
#define DeAlloc(PtrToMemory) _DeAlloc(&GlobalGameState->DynamAllocator, (ui64**)PtrToMemory, SPINE)


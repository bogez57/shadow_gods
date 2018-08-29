#pragma once
#include <string.h>
#include "list.h"

struct Memory_Region
{
    ui64* BaseAddress;
    ui64* EndAddress;
    ui64 UsedAmount;
    ui64 Size;
};

enum Mem_Region_Type
{
    SPINEDATA,
    LISTDATA,
    REGION_COUNT
};

#define AllocSize(MemRegion, Size) _AllocSize(MemRegion, Size)
#define AllocType(MemRegion, Type, Count) (Type*)_AllocType(MemRegion, sizeof(Type), Count)
#define FreeSize(MemRegion, Size) _FreeSize(MemRegion, Size)


/*** Linear Allocator ***/

struct Linear_Mem_Allocator 
{
    Memory_Region MemRegions[REGION_COUNT];
};

#define PushSize(Size) _PushType(&GlobalGameState->LinearAllocator, (Size), (LISTDATA))
#define PushType(Type, Count) (Type*)_PushType(&GlobalGameState->LinearAllocator, ((sizeof(Type)) * (Count)), LISTDATA)
#define PopSize(PtrToMemory) _PopSize(&GlobalGameState->LinearAllocator, (ui64**)PtrToMemory, LISTDATA)


/*** Dynamic Allocator ***/

struct Block_Header
{
    b IsFree{true};
    sizet Size{0};
    void* nextBlock{nullptr};
    void* prevBlock{nullptr};
};

struct Dynamic_Mem_Allocator 
{
    Memory_Region MemRegions[REGION_COUNT];

    List* FreeBlocks;
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

#define MallocType(Type, Count) (Type*)_MallocType(&GlobalGameState->DynamAllocator, ((sizeof(Type)) * (Count)), SPINEDATA)
#define MallocSize(Size) _MallocType(&GlobalGameState->DynamicAllocator, (Size), SPINEDATA)
#define CallocType(Type, Count) (Type*)_CallocType(&GlobalGameState->DynamAllocator, ((sizeof(Type)) * (Count)), SPINEDATA)
#define CallocSize(Type, Count) _CallocType(&GlobalGameState->DynamAllocator, (Size), SPINEDATA)
#define ReAlloc(Ptr, Type, Count) (Type*)_ReAlloc(&GlobalGameState->DynamAllocator, Ptr, sizeof(Type) * Count, SPINEDATA)
#define DeAlloc(PtrToMemory) _DeAlloc(&GlobalGameState->DynamAllocator, (ui64**)PtrToMemory, SPINEDATA)


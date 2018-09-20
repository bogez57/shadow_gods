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
    DYNAMIC,
    LINEAR,
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

#define PushSize(Size) _PushType(&GlobalGameState->LinearAllocator, (Size))
#define PushType(Type, Count) (Type*)_PushType(&GlobalGameState->LinearAllocator, ((sizeof(Type)) * (Count)))
#define PopSize(Size) _PopSize(&GlobalGameState->LinearAllocator, (Size))


/*** Dynamic Allocator ***/

struct Memory_Block 
{
    b IsFree{true};
    sizet Size{0};
    void* data{nullptr};
    Memory_Block* nextBlock{nullptr};
    Memory_Block* prevBlock{nullptr};
};

struct Dynamic_Mem_Allocator 
{
    Memory_Block* head;
    Memory_Block* tail;
    ui32 AmountOfBlocks;
};

local_func auto
CreateRegionFromGameMem(Game_Memory* GameMemory, ui64 Size) -> Memory_Region 
{
    Memory_Region Result{};

    Result.BaseAddress = (ui64*)((ui8*)GameMemory->TemporaryStorage) + GameMemory->TemporaryStorageUsed;
    Result.EndAddress = (ui64*)((ui8*)Result.BaseAddress) + (Size - 1);
    Result.Size = Size;
    Result.UsedAmount = 0;
    GameMemory->TemporaryStorageUsed += Size;

    return Result;
};

#define MallocType(Type, Count) (Type*)_MallocType(&GlobalGameState->DynamAllocator, ((sizeof(Type)) * (Count)))
#define MallocSize(Size) _MallocType(&GlobalGameState->DynamicAllocator, (Size))
#define CallocType(Type, Count) (Type*)_CallocType(&GlobalGameState->DynamAllocator, ((sizeof(Type)) * (Count)))
#define CallocSize(Type, Count) _CallocType(&GlobalGameState->DynamAllocator, (Size))
#define ReAlloc(Ptr, Type, Count) (Type*)_ReAlloc(&GlobalGameState->DynamAllocator, Ptr, sizeof(Type) * Count)
#define DeAlloc(PtrToMemory) _DeAlloc(&GlobalGameState->DynamAllocator, (ui64**)&PtrToMemory)


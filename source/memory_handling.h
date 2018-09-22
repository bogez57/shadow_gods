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
};

#define PushSize(Size) _PushType(&globalGameState->LinearAllocator, (Size))
#define PushType(Type, Count) (Type*)_PushType(&globalGameState->LinearAllocator, ((sizeof(Type)) * (Count)))
#define PopSize(Size) _PopSize(&globalGameState->LinearAllocator, (Size))


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

auto CreateRegionFromGameMem(Game_Memory GameMemory, ui64 Size)
{
    Memory_Region newRegion{};

    newRegion.BaseAddress = (ui64*)((ui8*)GameMemory.TemporaryStorage) + GameMemory.TemporaryStorageUsed;
    newRegion.EndAddress = (ui64*)((ui8*)newRegion.BaseAddress) + (Size - 1);
    newRegion.Size = Size;
    newRegion.UsedAmount = 0;
    GameMemory.TemporaryStorageUsed += Size;

    struct MultipleValues{Game_Memory mem; Memory_Region region;};
    return MultipleValues{GameMemory, newRegion};
};

#define MallocType(Type, Count) (Type*)_MallocType(&globalGameState->dynamAllocator, ((sizeof(Type)) * (Count)))
#define MallocSize(Size) _MallocType(&globalGameState->dynamicAllocator, (Size))
#define CallocType(Type, Count) (Type*)_CallocType(&globalGameState->dynamAllocator, ((sizeof(Type)) * (Count)))
#define CallocSize(Type, Count) _CallocType(&globalGameState->dynamAllocator, (Size))
#define ReAlloc(Ptr, Type, Count) (Type*)_ReAlloc(&globalGameState->dynamAllocator, Ptr, sizeof(Type) * Count)
#define DeAlloc(PtrToMemory) _DeAlloc(&globalGameState->dynamAllocator, (ui64**)&PtrToMemory)


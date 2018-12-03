#pragma once

#include "memory_allocators.h"

enum Mem_Region_Type
{
    DYNAMIC,
    LINEAR,
    REGION_COUNT
};

struct Memory_Region
{
    i64* BaseAddress;
    i64* EndAddress;
    i64 UsedAmount;
    i64 Size;
};

struct Memory_Handler
{
    Memory_Region memRegions[REGION_COUNT];
    Dynamic_Mem_Allocator dynamAllocator;
};

Memory_Region CreateRegionFromGameMem_1(Game_Memory* GameMemory, i64 Size);

#define AllocType(MemRegion, Type, Count) (Type*)_AllocType(MemRegion, sizeof(Type), Count)
#define AllocSize(MemRegion, Size) _AllocSize(MemRegion, Size)
#define FreeSize(MemRegion, Size) _FreeSize(MemRegion, Size)

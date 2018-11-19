#pragma once
#include <string.h>
#include "memory_handling.h"

Memory_Region CreateRegionFromGameMem_1(Game_Memory* GameMemory, ui64 Size)
{
    Memory_Region newRegion {};

    newRegion.BaseAddress = (ui64*)((ui8*)GameMemory->TemporaryStorage) + GameMemory->TemporaryStorageUsed;
    newRegion.EndAddress = (ui64*)((ui8*)newRegion.BaseAddress) + (Size - 1);
    newRegion.Size = Size;
    newRegion.UsedAmount = 0;
    GameMemory->TemporaryStorageUsed += Size;

    return newRegion;
};

auto _AllocType(Memory_Region* MemRegion, ui64 size, sizet Count) -> void*
{
    BGZ_ASSERT((MemRegion->UsedAmount + size) <= MemRegion->Size, "Memory requested, %x bytes, greater than maximum region size of %x bytes!", (MemRegion->UsedAmount + size), MemRegion->Size);
    void* Result = MemRegion->BaseAddress + MemRegion->UsedAmount;
    MemRegion->UsedAmount += (size * Count);

    return Result;
};

auto _AllocSize(Memory_Region* MemRegion, sizet size) -> void*
{
    BGZ_ASSERT((MemRegion->UsedAmount + size) <= MemRegion->Size, "Memory requested, %x bytes, greater than maximum region size of %x bytes!", (MemRegion->UsedAmount + size), MemRegion->Size);
    void* Result = MemRegion->BaseAddress + MemRegion->UsedAmount;
    MemRegion->UsedAmount += (size);

    return Result;
};

auto _FreeSize(Memory_Region* MemRegion, ui64 sizeToFree) -> void
{
    BGZ_ASSERT(sizeToFree < MemRegion->Size, "Trying to free more bytes then memory region holds!");
    BGZ_ASSERT(sizeToFree < MemRegion->UsedAmount, "Trying to free %x bytes with only %x bytes used!", sizeToFree, MemRegion->UsedAmount);

    MemRegion->UsedAmount -= sizeToFree;
};

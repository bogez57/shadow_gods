#pragma once

struct Memory_Region
{
    i64* BaseAddress;
    i64* EndAddress;
    i64 UsedAmount;
    i64 Size;
};

global_variable Memory_Region MemoryRegion;

#define AllocType(Type, Count) (Type*)_AllocType(sizeof(Type), Count)
#define AllocSize(Size) _AllocSize(Size)
#define FreeSize(Size) _FreeSize(Size)

void CreateRegionFromGameMem_1(Game_Memory* GameMemory, i64 Size)
{
    MemoryRegion.BaseAddress = (i64*)((i8*)GameMemory->TemporaryStorage) + GameMemory->TemporaryStorageUsed;
    MemoryRegion.EndAddress = (i64*)((i8*)MemoryRegion.BaseAddress) + (Size - 1);
    MemoryRegion.Size = Size;
    MemoryRegion.UsedAmount = 0;
    GameMemory->TemporaryStorageUsed += Size;
};

auto _AllocType(i64 size, i64 Count) -> void*
{
    BGZ_ASSERT((MemoryRegion.UsedAmount + size) <= MemoryRegion.Size, "Memory requested, %x bytes, greater than maximum region size of %x bytes!", (MemoryRegion.UsedAmount + size), MemoryRegion.Size);
    void* Result = MemoryRegion.BaseAddress + MemoryRegion.UsedAmount;
    MemoryRegion.UsedAmount += (size * Count);

    return Result;
};

auto _AllocSize(i64 size) -> void*
{
    BGZ_ASSERT((MemoryRegion.UsedAmount + size) <= MemoryRegion.Size, "Memory requested, %x bytes, greater than maximum region size of %x bytes!", (MemoryRegion.UsedAmount + size), MemoryRegion.Size);
    void* Result = MemoryRegion.BaseAddress + MemoryRegion.UsedAmount;
    MemoryRegion.UsedAmount += (size);

    return Result;
};

auto _FreeSize(i64 sizeToFree) -> void
{
    BGZ_ASSERT(sizeToFree < MemoryRegion.Size, "Trying to free more bytes then memory region holds!");
    BGZ_ASSERT(sizeToFree < MemoryRegion.UsedAmount, "Trying to free %x bytes with only %x bytes used!", sizeToFree, MemoryRegion.UsedAmount);

    MemoryRegion.UsedAmount -= sizeToFree;
};

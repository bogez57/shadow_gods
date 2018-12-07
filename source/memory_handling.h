#pragma once

#define MAX_SUB_REGIONS 10

struct Memory_Sub_Region
{
    void* BaseAddress;
    void* EndAddress;
    i64 UsedAmount;
    i64 Size;
};

struct Memory_Region
{
    void* BaseAddress;
    void* EndAddress;
    i64 UsedAmount;
    i64 Size;
    Memory_Sub_Region memorySubRegions[MAX_SUB_REGIONS];
    i32 numberOfSubRegions;
};

global_variable Memory_Region mainMemoryRegion;

#define AllocType(subRegionIdentifier, Type, Count) (Type*)_AllocType(sizeof(Type), Count)
#define AllocSize(subRegionIdentifier, Size) _AllocSize(subRegionIdentifier, Size)
#define FreeSize(subRegionIdentifier, Size) _FreeSize(subRegionIdentifier, Size)

void* PointerAddition(void* baseAddress, ui64 amountToAdvancePointer)
{
    void* newAddress {};

    newAddress = ((ui8*)baseAddress) + amountToAdvancePointer;

    return newAddress;
};

void CreateRegionFromGameMem_1(Game_Memory* GameMemory, i64 size)
{
    mainMemoryRegion.BaseAddress = PointerAddition(GameMemory->TemporaryStorage, GameMemory->TemporaryStorageUsed);
    mainMemoryRegion.EndAddress = PointerAddition(mainMemoryRegion.BaseAddress, (size - 1));
    mainMemoryRegion.Size = size;
    mainMemoryRegion.UsedAmount = 0;

    GameMemory->TemporaryStorageUsed += size;
};

//TODO: add error checks so don't create too many or too large sub regions
i32 CreateSubRegion(i64 size)
{
    if (mainMemoryRegion.numberOfSubRegions < MAX_SUB_REGIONS)
    {
        Memory_Sub_Region* newSubRegion = &mainMemoryRegion.memorySubRegions[mainMemoryRegion.numberOfSubRegions];

        newSubRegion->BaseAddress = PointerAddition(mainMemoryRegion.BaseAddress, mainMemoryRegion.UsedAmount);
        newSubRegion->EndAddress = PointerAddition(mainMemoryRegion.BaseAddress, (size - 1));
        newSubRegion->Size = size;
        newSubRegion->UsedAmount = 0;

        mainMemoryRegion.UsedAmount += size;

        return mainMemoryRegion.numberOfSubRegions++;
    }

    return -1; //error
};

auto _AllocType(i32 subRegionIdentifier, i64 size, i64 Count) -> void*
{
    BGZ_ASSERT((mainMemoryRegion.memorySubRegions[subRegionIdentifier].UsedAmount + size) <= mainMemoryRegion.memorySubRegions[subRegionIdentifier].Size, "Memory requested, %x bytes, greater than maximum region size of %x bytes!", (mainMemoryRegion.memorySubRegions[subRegionIdentifier].UsedAmount + size), mainMemoryRegion.memorySubRegions[subRegionIdentifier].Size);
    void* Result = PointerAddition(mainMemoryRegion.memorySubRegions[subRegionIdentifier].BaseAddress, mainMemoryRegion.memorySubRegions[subRegionIdentifier].UsedAmount);
    mainMemoryRegion.memorySubRegions[subRegionIdentifier].UsedAmount += (size * Count);

    return Result;
};

auto _AllocSize(i32 subRegionIdentifier, i64 size) -> void*
{
    BGZ_ASSERT((mainMemoryRegion.memorySubRegions[subRegionIdentifier].UsedAmount + size) <= mainMemoryRegion.memorySubRegions[subRegionIdentifier].Size, "Memory requested, %x bytes, greater than maximum region size of %x bytes!", (mainMemoryRegion.memorySubRegions[subRegionIdentifier].UsedAmount + size), mainMemoryRegion.memorySubRegions[subRegionIdentifier].Size);
    void* Result = PointerAddition(mainMemoryRegion.memorySubRegions[subRegionIdentifier].BaseAddress, mainMemoryRegion.memorySubRegions[subRegionIdentifier].UsedAmount);
    mainMemoryRegion.memorySubRegions[subRegionIdentifier].UsedAmount += (size);

    return Result;
};

auto _FreeSize(i32 subRegionIdentifier, i64 sizeToFree) -> void
{
    BGZ_ASSERT(sizeToFree < mainMemoryRegion.memorySubRegions[subRegionIdentifier].Size, "Trying to free more bytes then memory region holds!");
    BGZ_ASSERT(sizeToFree < mainMemoryRegion.memorySubRegions[subRegionIdentifier].UsedAmount, "Trying to free %x bytes with only %x bytes used!", sizeToFree, mainMemoryRegion.memorySubRegions[subRegionIdentifier].UsedAmount);

    mainMemoryRegion.memorySubRegions[subRegionIdentifier].UsedAmount -= sizeToFree;
};

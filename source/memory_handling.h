#pragma once

#define MAX_SUB_REGIONS 10

struct Memory_Sub_Region
{
    i64* BaseAddress;
    i64* EndAddress;
    i64 UsedAmount;
    i64 Size;
};

struct Memory_Region
{
    i64* BaseAddress;
    i64* EndAddress;
    i64 UsedAmount;
    i64 Size;
    Memory_Sub_Region memorySubRegions[MAX_SUB_REGIONS];
    i32 numberOfSubRegions;
};

global_variable Memory_Region mainMemoryRegion;

#define AllocType(subRegionIdentifier, Type, Count) (Type*)_AllocType(sizeof(Type), Count)
#define AllocSize(subRegionIdentifier, Size) _AllocSize(subRegionIdentifier, Size)
#define FreeSize(subRegionIdentifier, Size) _FreeSize(subRegionIdentifier, Size)

i64* PointerAdditionx64Aligned(i64* baseAddress, i64 amountToAdvancePointer)
{
    i64* newAddress {};

    //Consider separating malloc function to take into account proper alignment. So if you want to reserve mulitples
    //of an object of size 80 bits, then it would be best to know the original, individual size of the object so each new
    //object in memory can be aligned to 64 bit boundry
    newAddress = (i64*)((((i8*)baseAddress) + amountToAdvancePointer) + (amountToAdvancePointer % 8);

    return newAddress;
};

void CreateRegionFromGameMem_1(Game_Memory* GameMemory, i64 size)
{
    mainMemoryRegion.BaseAddress = PointerAdditionx64Aligned((i64*)GameMemory->TemporaryStorage, (i64)GameMemory->TemporaryStorageUsed);
    mainMemoryRegion.EndAddress = PointerAdditionx64Aligned(mainMemoryRegion.BaseAddress, (size - 1));
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

        newSubRegion->BaseAddress = PointerAdditionx64Aligned(mainMemoryRegion.BaseAddress, mainMemoryRegion.UsedAmount);
        newSubRegion->EndAddress = PointerAdditionx64Aligned(mainMemoryRegion.BaseAddress, (size - 1));
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
    i64* Result = PointerAdditionx64Aligned(mainMemoryRegion.memorySubRegions[subRegionIdentifier].BaseAddress, mainMemoryRegion.memorySubRegions[subRegionIdentifier].UsedAmount);
    mainMemoryRegion.memorySubRegions[subRegionIdentifier].UsedAmount += (size * Count);

    return Result;
};

auto _AllocSize(i32 subRegionIdentifier, i64 size) -> void*
{
    BGZ_ASSERT((mainMemoryRegion.memorySubRegions[subRegionIdentifier].UsedAmount + size) <= mainMemoryRegion.memorySubRegions[subRegionIdentifier].Size, "Memory requested, %x bytes, greater than maximum region size of %x bytes!", (mainMemoryRegion.memorySubRegions[subRegionIdentifier].UsedAmount + size), mainMemoryRegion.memorySubRegions[subRegionIdentifier].Size);
    i64* Result = PointerAdditionx64Aligned(mainMemoryRegion.memorySubRegions[subRegionIdentifier].BaseAddress, mainMemoryRegion.memorySubRegions[subRegionIdentifier].UsedAmount);
    mainMemoryRegion.memorySubRegions[subRegionIdentifier].UsedAmount += (size);

    return Result;
};

auto _FreeSize(i32 subRegionIdentifier, i64 sizeToFree) -> void
{
    BGZ_ASSERT(sizeToFree < mainMemoryRegion.memorySubRegions[subRegionIdentifier].Size, "Trying to free more bytes then memory region holds!");
    BGZ_ASSERT(sizeToFree < mainMemoryRegion.memorySubRegions[subRegionIdentifier].UsedAmount, "Trying to free %x bytes with only %x bytes used!", sizeToFree, mainMemoryRegion.memorySubRegions[subRegionIdentifier].UsedAmount);

    mainMemoryRegion.memorySubRegions[subRegionIdentifier].UsedAmount -= sizeToFree;
};

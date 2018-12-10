#pragma once

#include <cstring>

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

void CreateRegionFromMemory(void* GameMemory, i64 size)
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

/******************************************************* 
   Dynamic Allocator
********************************************************/

/*
    TODO: 
    1.) Maybe get rid of Blocks linked list data structure and use array instead?
    2.) Alignment?
*/

struct Memory_Block;

struct Dynamic_Mem_Allocator
{
    Memory_Block* head;
    Memory_Block* tail;
    ui32 AmountOfBlocks;
    i32 memoryRegionIdentifier;
};

struct Memory_Block
{
    b IsFree { true };
    i64 Size { 0 };
    void* data { nullptr };
    Memory_Block* nextBlock { nullptr };
    Memory_Block* prevBlock { nullptr };

    void FreeBlockAndMergeIfNecessary();
};

global_variable Dynamic_Mem_Allocator dynamAllocator;

local_func auto
ConvertDataToMemoryBlock(void* Ptr) -> Memory_Block*
{
    Memory_Block* BlockHeader {};
    BlockHeader = (Memory_Block*)(((ui8*)Ptr) - (sizeof(Memory_Block)));
    BlockHeader->data = Ptr;

    return BlockHeader;
};

local_func auto
GetDataFromBlock(Memory_Block* Header) -> void*
{
    BGZ_ASSERT(Header->Size != 0, "Cannot get data from non-existent memory header!");

    void* BlockData = (void*)(Header + 1);

    return BlockData;
};

local_func void InitDynamAllocator()
{
    BGZ_ERRCTXT1("When Initializing Dynamic Allocator");

    dynamAllocator.memoryRegionIdentifier = CreateSubRegion(Megabytes(10));

    dynamAllocator.AmountOfBlocks = 0;

    ui16 BlockSize = 8;
    ui16 TotalSize = sizeof(Memory_Block) + BlockSize;
    Memory_Block* InitialBlock = (Memory_Block*)AllocSize(dynamAllocator.memoryRegionIdentifier, TotalSize);

    InitialBlock->Size = BlockSize;
    InitialBlock->IsFree = false;
    InitialBlock->data = GetDataFromBlock(InitialBlock);
    InitialBlock->nextBlock = nullptr;
    InitialBlock->prevBlock = nullptr;

    dynamAllocator.head = InitialBlock;
    dynamAllocator.tail = InitialBlock;
};

local_func auto
SplitBlock(OUT Memory_Block* BlockToSplit, sizet SizeOfNewBlock) -> Memory_Block*
{
    BGZ_ERRCTXT1("When trying to split a memory block into two");

    BGZ_ASSERT(BlockToSplit->data, "Memory block data does not exist!");
    BGZ_ASSERT(SizeOfNewBlock > sizeof(Memory_Block), "Cannot fit new requested block size into block!");

    Memory_Block* NewBlock = (Memory_Block*)((ui8*)(BlockToSplit) + (BlockToSplit->Size - 1) + (sizeof(Memory_Block)));

    NewBlock->Size = SizeOfNewBlock;
    NewBlock->data = GetDataFromBlock(NewBlock);
    NewBlock->prevBlock = BlockToSplit;
    NewBlock->nextBlock = BlockToSplit->nextBlock;
    BlockToSplit->nextBlock = NewBlock;

    ++dynamAllocator.AmountOfBlocks;

    return NewBlock;
};

local_func auto
ReSizeAndMarkAsInUse(OUT Memory_Block* BlockToResize, sizet NewSize) -> sizet
{
    sizet SizeDiff = BlockToResize->Size - NewSize;
    BlockToResize->Size = NewSize;
    BlockToResize->IsFree = false;

    return SizeDiff;
};

local_func auto
GetFirstFreeBlockOfSize(i64 Size) -> Memory_Block*
{
    BGZ_ERRCTXT1("When trying to find first free memory block for dynamic memory");

    BGZ_ASSERT(dynamAllocator.head, "No head for dynamic memory list!");

    Memory_Block* Result {};
    Memory_Block* MemBlock = dynamAllocator.head;

    //Not first or last block
    if (MemBlock->nextBlock != nullptr)
    {
        for (ui32 BlockIndex { 0 }; BlockIndex < dynamAllocator.AmountOfBlocks; ++BlockIndex)
        {
            if (MemBlock->IsFree && MemBlock->Size > Size)
            {
                Result = MemBlock;
                return Result;
            }
            else
            {
                MemBlock = MemBlock->nextBlock;
            }
        };
    };

    //No free blocks found
    return nullptr;
};

void Memory_Block::FreeBlockAndMergeIfNecessary() //TODO: split up this function? What to do with more than 1 param that needs modified?
{
    this->IsFree = true;

    //If not the last block
    if (this->nextBlock)
    {
        { //Merge block with next and/or previous blocks
            if (this->nextBlock->IsFree)
            {
                this->Size += this->nextBlock->Size;
                this->nextBlock->Size = 0;

                if (this->nextBlock->nextBlock)
                {
                    this->nextBlock = this->nextBlock->nextBlock;
                }
                else
                {
                    this->nextBlock = nullptr;
                };

                --dynamAllocator.AmountOfBlocks;
            };

            if (this->prevBlock->IsFree)
            {
                this->prevBlock->Size += this->Size;

                if (this->nextBlock)
                {
                    this->prevBlock->nextBlock = this->nextBlock;
                }
                else
                {
                    this->prevBlock->nextBlock = nullptr;
                };

                this->Size = 0;

                --dynamAllocator.AmountOfBlocks;
            };
        };
    }
    else
    {
        FreeSize(dynamAllocator.memoryRegionIdentifier, this->Size);
        this->prevBlock->nextBlock = nullptr;
        dynamAllocator.tail = this->prevBlock;
        --dynamAllocator.AmountOfBlocks;
    };
};

local_func auto
AppendNewBlockAndMarkInUse(i64 Size) -> Memory_Block*
{
    i64 TotalSize = sizeof(Memory_Block) + Size;
    Memory_Block* NewBlock = (Memory_Block*)AllocSize(dynamAllocator.memoryRegionIdentifier, TotalSize);

    NewBlock->Size = Size;
    NewBlock->IsFree = false;
    NewBlock->data = GetDataFromBlock(NewBlock);

    NewBlock->prevBlock = dynamAllocator.tail;
    NewBlock->nextBlock = nullptr;
    dynamAllocator.tail->nextBlock = NewBlock;
    dynamAllocator.tail = NewBlock;

    ++dynamAllocator.AmountOfBlocks;

    return NewBlock;
};

auto _MallocType(i64 Size) -> void*
{
    BGZ_ERRCTXT1("When trying to allocate in dymamic memory");

    BGZ_ASSERT(dynamAllocator.head, "Dynamic allocator not initialized!");
    //BGZ_ASSERT(Size <= (globalMemHandler->memRegions[DYNAMIC].Size - globalMemHandler->memRegions[DYNAMIC].UsedAmount), "Not enough memory left for dynmaic memory allocation!");

    void* Result { nullptr };

    if (Size > 0)
    {
        Memory_Block* MemBlock = GetFirstFreeBlockOfSize(Size);

        //No free blocks found
        if (!MemBlock)
        {
            MemBlock = AppendNewBlockAndMarkInUse(Size);

            Result = MemBlock->data;
            return Result;
        }
        else
        {
            sizet SizeDiff = ReSizeAndMarkAsInUse(MemBlock, Size);

            if (SizeDiff > sizeof(Memory_Block))
            {
                Memory_Block* NewBlock = SplitBlock(MemBlock, SizeDiff);
                NewBlock->IsFree = true;
            }
            else
            {
                MemBlock->Size += SizeDiff;
            };

            Result = MemBlock->data;
            return Result;
        };
    };

    return Result;
};

auto _CallocType(i64 Size) -> void*
{
    BGZ_ERRCTXT1("When trying to allocate and initialize memory in dymamic memory");

    BGZ_ASSERT(dynamAllocator.head, "Dynamic allocator not initialized!");
    //BGZ_ASSERT(Size <= (globalMemHandler->memRegions[DYNAMIC].Size - globalMemHandler->memRegions[DYNAMIC].UsedAmount), "Not enough memory left for dynmaic memory allocation!");

    void* MemBlockData = _MallocType(Size);

    if (MemBlockData)
    {
        Memory_Block* Block = ConvertDataToMemoryBlock(MemBlockData);
        BGZ_ASSERT(Block->data, "Data retrieved invalid!");

        memset(MemBlockData, 0, Block->Size);
    };

    return MemBlockData;
};

auto _ReAlloc(void* DataToRealloc, i64 NewSize) -> void*
{
    BGZ_ERRCTXT1("When trying to reallocate memory in dymamic memory");

    BGZ_ASSERT(dynamAllocator.head, "Dynamic allocator not initialized!");
    //BGZ_ASSERT((globalMemHandler->memRegions[DYNAMIC].Size - globalMemHandler->memRegions[DYNAMIC].UsedAmount) > NewSize, "Not enough room left in memory region!");

    Memory_Block* BlockToRealloc;
    if (DataToRealloc)
    {
        BlockToRealloc = ConvertDataToMemoryBlock(DataToRealloc);

        if (NewSize < BlockToRealloc->Size)
        {
            sizet SizeDiff = ReSizeAndMarkAsInUse(BlockToRealloc, NewSize);

            if (SizeDiff > sizeof(Memory_Block))
            {
                Memory_Block* NewBlock = SplitBlock(BlockToRealloc, SizeDiff);
                NewBlock->IsFree = true;
            }
            else
            {
                BlockToRealloc->Size += SizeDiff;
            };
        }
        else
        {
            void* newBlockData = _MallocType(NewSize);

            memcpy(newBlockData, BlockToRealloc->data, BlockToRealloc->Size);
            memset(BlockToRealloc->data, 0, BlockToRealloc->Size); //TODO: Remove if speed becomes an issue;

            BlockToRealloc->FreeBlockAndMergeIfNecessary();

            return newBlockData;
        };
    }
    else
    {
        DataToRealloc = _MallocType(NewSize);
    };

    return DataToRealloc;
};

auto _DeAlloc(void** MemToFree) -> void
{
    BGZ_ERRCTXT1("When trying to free some memory in dynamic memory");

    if (*MemToFree)
    {
        //BGZ_ASSERT(*MemToFree > globalMemHandler->memRegions[DYNAMIC].BaseAddress && *MemToFree < globalMemHandler->memRegions[DYNAMIC].EndAddress, "Ptr to free not within dynmaic memory region!");

        Memory_Block* Block = ConvertDataToMemoryBlock(*MemToFree);

        Block->FreeBlockAndMergeIfNecessary();
        memset(*MemToFree, 0, Block->Size); //TODO: Remove if speed becomes an issue;

        *MemToFree = nullptr;
    };
};

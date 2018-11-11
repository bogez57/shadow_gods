#include "memory_handling.h"
#include "memory_allocators.h"

/******************************************************* 
   Dynamic Allocator
********************************************************/

/*
    TODO: 
    1.) Maybe get rid of Blocks linked list data structure and use array instead?
    2.) Alignment?
*/

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

auto CreateAndInitDynamAllocator() -> Dynamic_Mem_Allocator
{
    BGZ_ERRCTXT1("When Initializing Dynamic Allocator");

    Dynamic_Mem_Allocator dynamAllocator {};

    dynamAllocator.AmountOfBlocks = 0;

    ui16 BlockSize = 8;
    ui16 TotalSize = sizeof(Memory_Block) + BlockSize;
    Memory_Block* InitialBlock = (Memory_Block*)AllocSize(&globalMemHandler->memRegions[DYNAMIC], TotalSize);

    InitialBlock->Size = BlockSize;
    InitialBlock->IsFree = false;
    InitialBlock->data = GetDataFromBlock(InitialBlock);
    InitialBlock->nextBlock = nullptr;
    InitialBlock->prevBlock = nullptr;

    dynamAllocator.head = InitialBlock;
    dynamAllocator.tail = InitialBlock;

    return dynamAllocator;
};

local_func auto
SplitBlock(OUT Memory_Block* BlockToSplit, sizet SizeOfNewBlock, Dynamic_Mem_Allocator* DynamAllocator) -> Memory_Block*
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

    ++DynamAllocator->AmountOfBlocks;

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
GetFirstFreeBlockOfSize(ui64 Size, Dynamic_Mem_Allocator* DynamAllocator) -> Memory_Block*
{
    BGZ_ERRCTXT1("When trying to find first free memory block for dynamic memory");

    BGZ_ASSERT(DynamAllocator->head, "No head for dynamic memory list!");

    Memory_Block* Result {};
    Memory_Block* MemBlock = DynamAllocator->head;

    //Not first or last block
    if (MemBlock->nextBlock != nullptr)
    {
        for (ui32 BlockIndex { 0 }; BlockIndex < DynamAllocator->AmountOfBlocks; ++BlockIndex)
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

local_func auto
AppendNewFilledBlock(OUT Dynamic_Mem_Allocator* DynamAllocator, ui64 Size) -> Memory_Block*
{
    ui64 TotalSize = sizeof(Memory_Block) + Size;
    Memory_Block* NewBlock = (Memory_Block*)AllocSize(&globalMemHandler->memRegions[DYNAMIC], TotalSize);

    NewBlock->Size = Size;
    NewBlock->IsFree = false;
    NewBlock->data = GetDataFromBlock(NewBlock);

    NewBlock->prevBlock = DynamAllocator->tail;
    NewBlock->nextBlock = nullptr;
    DynamAllocator->tail->nextBlock = NewBlock;
    DynamAllocator->tail = NewBlock;

    ++DynamAllocator->AmountOfBlocks;

    return NewBlock;
};

local_func auto
FreeBlockButDontZero(OUT Dynamic_Mem_Allocator* DynamAllocator, OUT Memory_Block* BlockToFree) -> void
{
    BlockToFree->IsFree = true;

    //If not the last block
    if (BlockToFree->nextBlock)
    {
        if (BlockToFree->nextBlock->IsFree)
        {
            BlockToFree->Size += BlockToFree->nextBlock->Size;
            BlockToFree->nextBlock->Size = 0;

            if (BlockToFree->nextBlock->nextBlock)
            {
                BlockToFree->nextBlock = BlockToFree->nextBlock->nextBlock;
            }
            else
            {
                BlockToFree->nextBlock = nullptr;
            };

            --DynamAllocator->AmountOfBlocks;
        };

        if (BlockToFree->prevBlock->IsFree)
        {
            BlockToFree->prevBlock->Size += BlockToFree->Size;

            if (BlockToFree->nextBlock)
                BlockToFree->prevBlock = BlockToFree->nextBlock;
            else
                BlockToFree->prevBlock = nullptr;

            --DynamAllocator->AmountOfBlocks;

            return;
        }
    }
    else
    {
        FreeSize(&globalMemHandler->memRegions[DYNAMIC], BlockToFree->Size);
        BlockToFree->prevBlock->nextBlock = nullptr;
        DynamAllocator->tail = BlockToFree->prevBlock;
        --DynamAllocator->AmountOfBlocks;
    }
};

auto _MallocType(Dynamic_Mem_Allocator* DynamAllocator, sizet Size) -> void*
{
    BGZ_ERRCTXT1("When trying to allocate in dymamic memory");

    BGZ_ASSERT(DynamAllocator->head, "Dynamic allocator not initialized!");
    BGZ_ASSERT(Size <= (globalMemHandler->memRegions[DYNAMIC].Size - globalMemHandler->memRegions[DYNAMIC].UsedAmount), "Not enough memory left for dynmaic memory allocation!");

    void* Result { nullptr };

    if (Size > 0)
    {
        Memory_Block* MemBlock = GetFirstFreeBlockOfSize(Size, DynamAllocator);

        //No free blocks found
        if (!MemBlock)
        {
            MemBlock = AppendNewFilledBlock(DynamAllocator, Size);

            Result = MemBlock->data;
            return Result;
        }
        else
        {
            sizet SizeDiff = ReSizeAndMarkAsInUse(MemBlock, Size);

            if (SizeDiff > sizeof(Memory_Block))
            {
                Memory_Block* NewBlock = SplitBlock(MemBlock, SizeDiff, DynamAllocator);
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

auto _CallocType(Dynamic_Mem_Allocator* DynamAllocator, sizet Size) -> void*
{
    BGZ_ERRCTXT1("When trying to allocate and initialize memory in dymamic memory");

    BGZ_ASSERT(DynamAllocator->head, "Dynamic allocator not initialized!");
    BGZ_ASSERT(Size <= (globalMemHandler->memRegions[DYNAMIC].Size - globalMemHandler->memRegions[DYNAMIC].UsedAmount), "Not enough memory left for dynmaic memory allocation!");

    void* MemBlockData = _MallocType(DynamAllocator, Size);

    if (MemBlockData)
    {
        Memory_Block* Block = ConvertDataToMemoryBlock(MemBlockData);
        BGZ_ASSERT(Block->data, "Data retrieved invalid!");

        memset(MemBlockData, 0, Block->Size);
    };

    return MemBlockData;
};

auto _ReAlloc(Dynamic_Mem_Allocator* DynamAllocator, void* DataToRealloc, ui64 NewSize) -> void*
{
    BGZ_ERRCTXT1("When trying to reallocate memory in dymamic memory");

    BGZ_ASSERT(DynamAllocator->head, "Dynamic allocator not initialized!");
    BGZ_ASSERT((globalMemHandler->memRegions[DYNAMIC].UsedAmount - NewSize) > NewSize, "Not enough room left in memory region!");

    Memory_Block* BlockToRealloc;
    if (DataToRealloc)
    {
        BlockToRealloc = ConvertDataToMemoryBlock(DataToRealloc);

        if (NewSize < BlockToRealloc->Size)
        {
            sizet SizeDiff = ReSizeAndMarkAsInUse(BlockToRealloc, NewSize);

            if (SizeDiff > sizeof(Memory_Block))
            {
                Memory_Block* NewBlock = SplitBlock(BlockToRealloc, SizeDiff, DynamAllocator);
                NewBlock->IsFree = true;
            }
            else
            {
                BlockToRealloc->Size += SizeDiff;
            };
        }
        else
        {
            FreeBlockButDontZero(DynamAllocator, BlockToRealloc);
            Memory_Block* NewBlock = AppendNewFilledBlock(DynamAllocator, NewSize);

            memcpy(NewBlock->data, BlockToRealloc->data, NewSize);
            memset(BlockToRealloc->data, 0, BlockToRealloc->Size); //TODO: Remove if speed becomes an issue;

            return NewBlock->data;
        };
    }
    else
    {
        BlockToRealloc = AppendNewFilledBlock(DynamAllocator, NewSize);
        DataToRealloc = BlockToRealloc->data;
    };

    return DataToRealloc;
};

auto _DeAlloc(Dynamic_Mem_Allocator* DynamAllocator, ui64** MemToFree) -> void
{
    BGZ_ERRCTXT1("When trying to free some memory in dynamic memory");

    if (*MemToFree)
    {
        BGZ_ASSERT(*MemToFree > globalMemHandler->memRegions[DYNAMIC].BaseAddress && *MemToFree < globalMemHandler->memRegions[DYNAMIC].EndAddress, "Ptr to free not within dynmaic memory region!");

        Memory_Block* Block = ConvertDataToMemoryBlock(*MemToFree);

        FreeBlockButDontZero(DynamAllocator, Block);
        memset(*MemToFree, 0, Block->Size); //TODO: Remove if speed becomes an issue;

        *MemToFree = nullptr;
    };
};

/******************************************************* 
   Linear Allocator 
********************************************************/

auto _PushType(Linear_Mem_Allocator* LinearAllocator, ui64 size) -> void*
{
    Memory_Region* MemRegion = &globalMemHandler->memRegions[LINEAR];
    BGZ_ASSERT((MemRegion->UsedAmount + size) <= MemRegion->Size, "Memory bytes requested, %x, greater than total memory region size, %x", (MemRegion->UsedAmount + size), MemRegion->Size);

    void* Result = MemRegion->BaseAddress + MemRegion->UsedAmount;
    MemRegion->UsedAmount += (size);

    return Result;
};

auto _PopSize(Linear_Mem_Allocator* linearAllocator, ui64 sizeToFree) -> void
{
    Memory_Region* MemRegion = &globalMemHandler->memRegions[LINEAR];
    BGZ_ASSERT(sizeToFree < MemRegion->Size || sizeToFree < MemRegion->UsedAmount, "Trying to free more bytes then available!");

    MemRegion->UsedAmount -= sizeToFree;
};
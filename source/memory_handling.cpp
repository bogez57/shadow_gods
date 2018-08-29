#pragma once
#include <string.h>
#include "memory_handling.h"
#include "list.h"

/*
    TODO: 
    1.) Maybe get rid of Blocks linked list data structure and use array instead?
    2.) Alignment?
*/

auto
_AllocSize(Memory_Region* MemRegion, sizet Size) -> void*
{
    BGZ_ASSERT((MemRegion->UsedAmount + Size) <= MemRegion->Size);
    void* Result = MemRegion->BaseAddress + MemRegion->UsedAmount;
    MemRegion->UsedAmount += (Size);

    return Result;
};

auto
_AllocType(Memory_Region* MemRegion, ui64 Size, sizet Count) -> void*
{
    BGZ_ASSERT((MemRegion->UsedAmount + Size) <= MemRegion->Size);
    void* Result = MemRegion->BaseAddress + MemRegion->UsedAmount;
    MemRegion->UsedAmount += (Size * Count);

    return Result;
};

auto
_FreeSize(Memory_Region* MemRegion, ui64 SizeToFree) -> void
{
    BGZ_ASSERT(SizeToFree < MemRegion->Size || SizeToFree < MemRegion->UsedAmount);

    MemRegion->UsedAmount -= SizeToFree;
};

//////////////////////////////////////////////////////////////////////

/*** Linear Allocator ***/

auto
_PushType(Linear_Mem_Allocator* LinearAllocator, ui64 Size, Mem_Region_Type Region)-> void*
{
    Memory_Region* MemRegion = &LinearAllocator->MemRegions[Region];
    BGZ_ASSERT((MemRegion->UsedAmount + Size) <= MemRegion->Size);

    void* Result = MemRegion->BaseAddress + MemRegion->UsedAmount;
    MemRegion->UsedAmount += (Size);

    return Result;
};

auto
_PopSize(Linear_Mem_Allocator* LinearAllocator, ui64 SizeToFree, Mem_Region_Type Region) -> void
{
    Memory_Region* MemRegion = &LinearAllocator->MemRegions[Region];
    BGZ_ASSERT(SizeToFree < MemRegion->Size || SizeToFree < MemRegion->UsedAmount);

    MemRegion->UsedAmount -= SizeToFree;
};


/*** Dynamic Allocator ***/

local_func auto
ConvertDataToMemoryBlock(void* Ptr) -> Memory_Block*
{
    Memory_Block *BlockHeader{};
    BlockHeader= (Memory_Block*)(((ui8*)Ptr) - (sizeof(Memory_Block)));
    BlockHeader->data = Ptr;

    return BlockHeader;
};

local_func auto
GetDataFromBlock(Memory_Block* Header) -> void*
{
    BGZ_ASSERT(Header->Size != 0);

    void* BlockData = (void *)(Header + 1);

    return BlockData;
};

local_func auto
SplitBlock(OUT Memory_Block* BlockToSplit, sizet SizeOfNewBlock, Dynamic_Mem_Allocator* DynamAllocator) -> Memory_Block*
{
    BGZ_ASSERT(BlockToSplit->data);
    BGZ_ASSERT(SizeOfNewBlock > sizeof(Memory_Block));

    Memory_Block *NewBlock = (Memory_Block*)((ui8*)(BlockToSplit) + (BlockToSplit->Size - 1) + (sizeof(Memory_Block)));

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
    Memory_Block* Result{};
    BGZ_ASSERT(DynamAllocator->head);

    Memory_Block* MemBlock = DynamAllocator->head;

    //Not first or last block
    if(MemBlock->nextBlock != nullptr)
    {
        for (ui32 BlockIndex{0}; BlockIndex < DynamAllocator->AmountOfBlocks; ++BlockIndex)
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
AppendNewFilledBlock(OUT Dynamic_Mem_Allocator* DynamAllocator, ui64 Size, Mem_Region_Type Region) -> Memory_Block*
{
    ui64 TotalSize = sizeof(Memory_Block) + Size;
    Memory_Block* NewBlock = (Memory_Block*)AllocSize(&DynamAllocator->MemRegions[Region], TotalSize);

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
FreeBlockButDontZero(OUT Dynamic_Mem_Allocator* DynamAllocator, OUT Memory_Block* BlockToFree, Mem_Region_Type Region) -> void
{
    BlockToFree->IsFree = true;

    //If not the last block
    if (BlockToFree->nextBlock)
    {
        if (BlockToFree->nextBlock->IsFree)
        {
            BlockToFree->Size += BlockToFree->nextBlock->Size;

            if (BlockToFree->nextBlock->nextBlock) BlockToFree->nextBlock = BlockToFree->nextBlock->nextBlock;
            else BlockToFree->nextBlock = nullptr;

            BlockToFree->nextBlock->Size = 0;

            --DynamAllocator->AmountOfBlocks;
        };

        if (BlockToFree->prevBlock->IsFree)
        {
            BlockToFree->prevBlock->Size += BlockToFree->Size;

            if (BlockToFree->nextBlock) BlockToFree->prevBlock = BlockToFree->nextBlock;
            else BlockToFree->prevBlock = nullptr;

            BlockToFree->Size = 0;

            --DynamAllocator->AmountOfBlocks;

            return;
        }
    }
    else
    {
        FreeSize(&DynamAllocator->MemRegions[Region], BlockToFree->Size);
        BlockToFree->prevBlock->nextBlock = nullptr;
        DynamAllocator->tail = BlockToFree->prevBlock;
        --DynamAllocator->AmountOfBlocks;
    }
};

local_func auto
InitDynamAllocator(Dynamic_Mem_Allocator* DynamAllocator, Mem_Region_Type Region) -> void
{
    DynamAllocator->AmountOfBlocks = 0;
    ui16 BlockSize = 8;
    ui16 TotalSize = sizeof(Memory_Block) + BlockSize;
    Memory_Block* InitialBlock = (Memory_Block*)AllocSize(&DynamAllocator->MemRegions[Region], TotalSize);

    InitialBlock->Size = BlockSize;
    InitialBlock->IsFree = false;
    InitialBlock->data = GetDataFromBlock(InitialBlock);
    InitialBlock->nextBlock = nullptr;
    InitialBlock->prevBlock = nullptr;

    DynamAllocator->head = InitialBlock;
    DynamAllocator->tail = InitialBlock;
};

auto 
_MallocType(Dynamic_Mem_Allocator* DynamAllocator, sizet Size, Mem_Region_Type Region) -> void*
{
    BGZ_ASSERT(Size <= DynamAllocator->MemRegions[Region].Size);

    void* Result{nullptr};

    if(Size > 0)
    {
        Memory_Block* MemBlock = GetFirstFreeBlockOfSize(Size, DynamAllocator);

        //No free blocks found
        if (!MemBlock)
        {
            MemBlock = AppendNewFilledBlock(DynamAllocator, Size, Region);

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

auto
_CallocType(Dynamic_Mem_Allocator* DynamAllocator, sizet Size, Mem_Region_Type Region) -> void*
{
    BGZ_ASSERT(Size <= DynamAllocator->MemRegions[Region].Size);

    void* MemBlockData = _MallocType(DynamAllocator, Size, Region);

    if(MemBlockData)
    {
        Memory_Block* Block = ConvertDataToMemoryBlock(MemBlockData);
        BGZ_ASSERT(Block->data);

        memset(MemBlockData, 0, Block->Size);
    };

    return MemBlockData;
};

auto
_ReAlloc(Dynamic_Mem_Allocator* DynamAllocator, void* DataToRealloc, ui64 NewSize, Mem_Region_Type Region) -> void*
{
    BGZ_ASSERT((DynamAllocator->MemRegions[Region].UsedAmount - NewSize) > NewSize);

    Memory_Block* BlockToRealloc = ConvertDataToMemoryBlock(DataToRealloc);

    if(DataToRealloc)
    {
        if (NewSize < BlockToRealloc->Size)
        {
            sizet SizeDiff = ReSizeAndMarkAsInUse(BlockToRealloc, NewSize);

            if(SizeDiff > sizeof(Memory_Block))
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
            FreeBlockButDontZero(DynamAllocator, BlockToRealloc, Region);
            Memory_Block* NewBlock = AppendNewFilledBlock(DynamAllocator, NewSize, Region);

            memcpy(NewBlock->data, BlockToRealloc->data, NewSize);
            memset(BlockToRealloc->data, 0, BlockToRealloc->Size); //TODO: Remove if speed becomes an issue;

            return NewBlock->data;
        };
    }
    else
    {
        BlockToRealloc = AppendNewFilledBlock(DynamAllocator, NewSize, Region);
        DataToRealloc = BlockToRealloc->data;
    };

    return DataToRealloc;
};

auto
_DeAlloc(Dynamic_Mem_Allocator* DynamAllocator, ui64** MemToFree, Mem_Region_Type Region) -> void
{
    if (*MemToFree) 
    {
        BGZ_ASSERT(*MemToFree > DynamAllocator->MemRegions[Region].BaseAddress && *MemToFree < DynamAllocator->MemRegions[Region].EndAddress);

        Memory_Block* Block = ConvertDataToMemoryBlock(*MemToFree);

        FreeBlockButDontZero(DynamAllocator, Block, Region);
        memset(*MemToFree, 0, Block->Size); //TODO: Remove if speed becomes an issue;

        *MemToFree = nullptr;
    };
};
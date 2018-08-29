#pragma once
#include <string.h>
#include "memory_handling.h"
#include "list.h"

/*
    TODO: 
    1.) Maybe get rid of FilledBlocks linked list data structure and use array instead?
    2.) Alignment?
    3.) Right now, list data structure goes through C's malloc and free. Want to try and use my memory instead if I can
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
SplitBlock(OUT Memory_Block* BlockToSplit, sizet SizeOfNewBlock) -> Memory_Block*
{
    BGZ_ASSERT(BlockToSplit->data);
    BGZ_ASSERT(SizeOfNewBlock > sizeof(Memory_Block));

    Memory_Block *NewBlock = (Memory_Block*)((ui8*)(BlockToSplit) + (BlockToSplit->Size - 1) + (sizeof(Memory_Block)));

    NewBlock->Size = SizeOfNewBlock;
    NewBlock->data = GetDataFromBlock(NewBlock);
    NewBlock->prevBlock = BlockToSplit;
    NewBlock->nextBlock = BlockToSplit->nextBlock;
    BlockToSplit->nextBlock = NewBlock;

    return NewBlock;
};

local_func auto
ReSizeAndMarkAsInUse(OUT Memory_Block* BlockToResize, sizet NewSize, List* FreeList) -> sizet
{
    sizet SizeDiff = BlockToResize->Size - NewSize;
    BlockToResize->Size = NewSize;
    if (BlockToResize->IsFree)
        list_remove(FreeList, BlockToResize, NULL);

    BlockToResize->IsFree = false;

    return SizeDiff;
};

local_func auto
GetFirstFreeBlockOfSize(ui64 Size, List* FreeList) -> Memory_Block*
{
    Memory_Block* Result{};

    ListIter FreeListIter{};
    Memory_Block* MemBlock{};
    list_iter_init(&FreeListIter, FreeList);
    list_get_at(FreeList, FreeListIter.index, &(void*)MemBlock);
    list_iter_next(&FreeListIter, &(void*)MemBlock);

    for (ui32 BlockIndex{0}; BlockIndex < FreeList->size; ++BlockIndex)
    {
        if (MemBlock)
        {
            if (MemBlock->IsFree && MemBlock->Size > Size)
            {
                Result = MemBlock;
                return Result;
            }
        }

        list_iter_next(&FreeListIter, &(void*)MemBlock);
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

    GetLastElem(DynamAllocator->FilledBlocks, &(void*)NewBlock->prevBlock);

    NewBlock->prevBlock->nextBlock = NewBlock;

    AddToEnd(DynamAllocator->FilledBlocks, NewBlock);

    return NewBlock;
};

local_func auto
FreeBlockButDontZero(OUT Dynamic_Mem_Allocator* DynamAllocator, OUT Memory_Block* BlockToFree, Mem_Region_Type Region) -> void
{
    BlockToFree->IsFree = true;

    if (BlockToFree->nextBlock)
    {
        if (BlockToFree->nextBlock->IsFree)
        {
            BlockToFree->Size += BlockToFree->nextBlock->Size;
            list_remove(DynamAllocator->FreeBlocks, BlockToFree->nextBlock, NULL);

            if (BlockToFree->nextBlock->nextBlock)
                BlockToFree->nextBlock = BlockToFree->nextBlock->nextBlock;
            else
                BlockToFree->nextBlock = nullptr;

            BlockToFree->nextBlock->Size = 0;
        };

        if (BlockToFree->prevBlock->IsFree)
        {
            BlockToFree->prevBlock->Size += BlockToFree->Size;

            if (BlockToFree->nextBlock)
                BlockToFree->prevBlock = BlockToFree->nextBlock;
            else
                BlockToFree->prevBlock = nullptr;

            BlockToFree->Size = 0;

            return;
        }
    }
    else
    {
        FreeSize(&DynamAllocator->MemRegions[Region], BlockToFree->Size);
        BlockToFree->prevBlock->nextBlock = nullptr;
    }

    list_add(DynamAllocator->FreeBlocks, BlockToFree);
};

local_func auto
InitDynamAllocator(Dynamic_Mem_Allocator* DynamAllocator, Mem_Region_Type Region) -> void
{
    list_new(&DynamAllocator->FreeBlocks);

    DynamAllocator->FilledBlocks = NewList();

    ui16 BlockSize = 8;
    ui16 TotalSize = sizeof(Memory_Block) + BlockSize;
    Memory_Block* InitialBlock = (Memory_Block*)AllocSize(&DynamAllocator->MemRegions[Region], TotalSize);

    InitialBlock->Size = BlockSize;
    InitialBlock->IsFree = false;
    InitialBlock->data = GetDataFromBlock(InitialBlock);

    AddToEnd(DynamAllocator->FilledBlocks, InitialBlock);
};

auto 
_MallocType(Dynamic_Mem_Allocator* DynamAllocator, sizet Size, Mem_Region_Type Region) -> void*
{
    BGZ_ASSERT(Size <= DynamAllocator->MemRegions[Region].Size);

    void* Result{nullptr};

    if(Size > 0)
    {
        Memory_Block* MemBlock = GetFirstFreeBlockOfSize(Size, DynamAllocator->FreeBlocks);

        //No free blocks found
        if (!MemBlock)
        {
            MemBlock = AppendNewFilledBlock(DynamAllocator, Size, Region);

            Result = MemBlock->data;
            return Result;
        }
        else
        {
            sizet SizeDiff = ReSizeAndMarkAsInUse(MemBlock, Size, DynamAllocator->FreeBlocks);

            if (SizeDiff > sizeof(Memory_Block))
            {
                Memory_Block* NewBlock = SplitBlock(MemBlock, SizeDiff);
                NewBlock->IsFree = true;
                list_add(DynamAllocator->FreeBlocks, NewBlock);
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
        BGZ_ASSERT(Block->Size == Size);
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
            sizet SizeDiff = ReSizeAndMarkAsInUse(BlockToRealloc, NewSize, DynamAllocator->FreeBlocks);

            if(SizeDiff > sizeof(Memory_Block))
            {
                Memory_Block* NewBlock = SplitBlock(BlockToRealloc, SizeDiff);
                NewBlock->IsFree = true;
                list_add(DynamAllocator->FreeBlocks, NewBlock);
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

        *MemToFree = nullptr;
    };
};
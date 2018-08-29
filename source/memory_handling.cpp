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
GetBlockHeader(void* Ptr) -> Block_Header*
{
    Block_Header *BlockHeader{};
    BlockHeader = (Block_Header*)(((ui8*)Ptr) - (sizeof(Block_Header)));

    return BlockHeader;
};

local_func auto
GetBlockData(Block_Header* Header) -> void*
{
    BGZ_ASSERT(Header->Size != 0);

    void* BlockData = (void *)(Header + 1);

    return BlockData;
};

local_func auto
SplitBlock(OUT void* BlockToSplit, ui64 Size, OUT List* FreeList) -> void
{
    Block_Header* BlockHeader = GetBlockHeader(BlockToSplit);

    sizet SizeDiff = BlockHeader->Size - Size;
    BlockHeader->Size = Size;
    BlockHeader->IsFree = false;

    if (SizeDiff > sizeof(Block_Header))
    {
        void* NewBlock = (((ui8*)(BlockToSplit)) + ((BlockHeader->Size - 1) + (sizeof(Block_Header))));
        Block_Header* NewBlockHeader = GetBlockHeader(NewBlock);

        NewBlockHeader->Size = SizeDiff;
        NewBlockHeader->IsFree = true;
        NewBlockHeader->prevBlock = BlockToSplit;
        NewBlockHeader->nextBlock = BlockHeader->nextBlock;
        BlockHeader->nextBlock = NewBlock;

        list_add(FreeList, NewBlock);
    }
    else
    {
        BlockHeader->Size += SizeDiff;
    };
};

local_func auto
GetFirstFreeBlockOfSize(ui64 Size, List* FreeList) -> void*
{
    void* Result{};

    ListIter FreeListIter{};
    void* MemBlock{};
    list_iter_init(&FreeListIter, FreeList);
    list_get_at(FreeList, FreeListIter.index, &MemBlock);
    list_iter_next(&FreeListIter, &MemBlock);

    for (ui32 BlockIndex{0}; BlockIndex < FreeList->size; ++BlockIndex)
    {
        if (MemBlock)
        {
            Block_Header* BlockHeader = GetBlockHeader(MemBlock);
            if (BlockHeader->IsFree && BlockHeader->Size > Size)
            {
                Result = MemBlock;
                return Result;
            }
        }

        list_iter_next(&FreeListIter, &MemBlock);
    };

    //No free blocks found
    return nullptr;
};

local_func auto
AppendNewFilledBlock(OUT Dynamic_Mem_Allocator* DynamAllocator, ui64 Size, Mem_Region_Type Region) -> void*
{
    ui64 TotalSize = sizeof(Block_Header) + Size;
    Block_Header* BlockHeader = (Block_Header*)AllocSize(&DynamAllocator->MemRegions[Region], TotalSize);

    BlockHeader->Size = Size;
    BlockHeader->IsFree = false;

    GetLastElem(DynamAllocator->FilledBlocks, &BlockHeader->prevBlock);

    void* NewBlock = GetBlockData(BlockHeader);
    GetBlockHeader(BlockHeader->prevBlock)->nextBlock = NewBlock;

    AddToEnd(DynamAllocator->FilledBlocks, NewBlock);

    return NewBlock;
};

local_func auto
FreeBlockButDontZero(OUT Dynamic_Mem_Allocator* DynamAllocator, OUT void* BlockToFree, Mem_Region_Type Region) -> void
{
    Block_Header* BlockHeader = GetBlockHeader(BlockToFree);

    BlockHeader->IsFree = true;

    if (BlockHeader->nextBlock)
    {
        if (GetBlockHeader(BlockHeader->nextBlock)->IsFree)
        {
            BlockHeader->Size += GetBlockHeader(BlockHeader->nextBlock)->Size;
            list_remove(DynamAllocator->FreeBlocks, BlockHeader->nextBlock, NULL);

            if (GetBlockHeader(BlockHeader->nextBlock)->nextBlock)
                BlockHeader->nextBlock = GetBlockHeader(BlockHeader->nextBlock)->nextBlock;
            else
                BlockHeader->nextBlock = nullptr;

            GetBlockHeader(BlockHeader->nextBlock)->Size = 0;
        };

        if (GetBlockHeader(BlockHeader->prevBlock)->IsFree)
        {
            GetBlockHeader(BlockHeader->prevBlock)->Size += BlockHeader->Size;

            if (BlockHeader->nextBlock)
                BlockHeader->prevBlock = BlockHeader->nextBlock;
            else
                BlockHeader->prevBlock = nullptr;

            BlockHeader->Size = 0;

            return;
        }
    }
    else
    {
        FreeSize(&DynamAllocator->MemRegions[Region], BlockHeader->Size);
        GetBlockHeader(BlockHeader->prevBlock)->nextBlock = nullptr;
    }

    list_add(DynamAllocator->FreeBlocks, BlockToFree);
};

local_func auto
InitDynamAllocator(Dynamic_Mem_Allocator* DynamAllocator, Mem_Region_Type Region) -> void
{
    list_new(&DynamAllocator->FreeBlocks);

    DynamAllocator->FilledBlocks = NewList();

    ui16 BlockSize = 8;
    ui16 TotalSize = sizeof(Block_Header) + BlockSize;
    Block_Header* InitialBlockHeader = (Block_Header*)AllocSize(&DynamAllocator->MemRegions[Region], TotalSize);

    InitialBlockHeader->Size = BlockSize;
    InitialBlockHeader->IsFree = false;

    void* InitialBlock = GetBlockData(InitialBlockHeader);

    AddToEnd(DynamAllocator->FilledBlocks, InitialBlock);
};

auto 
_MallocType(Dynamic_Mem_Allocator* DynamAllocator, sizet Size, Mem_Region_Type Region) -> void*
{
    BGZ_ASSERT(Size <= DynamAllocator->MemRegions[Region].Size);

    void* Result{nullptr};

    if(Size > 0)
    {
        void *MemBlock = GetFirstFreeBlockOfSize(Size, DynamAllocator->FreeBlocks);

        //No free blocks found
        if (!MemBlock)
        {
            MemBlock = AppendNewFilledBlock(DynamAllocator, Size, Region);
        }
        else
        {
            //Remove Size check from SplitBlock Func to out here
            SplitBlock(MemBlock, Size, DynamAllocator->FreeBlocks);
            list_remove(DynamAllocator->FreeBlocks, MemBlock, NULL);

            Result = MemBlock;
            return Result;
        };

        Result = MemBlock;
        return MemBlock;
    };

    return Result;
};

auto
_CallocType(Dynamic_Mem_Allocator* DynamAllocator, sizet Size, Mem_Region_Type Region) -> void*
{
    BGZ_ASSERT(Size <= DynamAllocator->MemRegions[Region].Size);

    void* MemBlock = _MallocType(DynamAllocator, Size, Region);

    if(MemBlock)
    {
        Block_Header *BlockHeader = GetBlockHeader(MemBlock);
        memset(MemBlock, 0, BlockHeader->Size);
    };

    return MemBlock;
};

auto
_ReAlloc(Dynamic_Mem_Allocator* DynamAllocator, void* BlockToRealloc, ui64 Size, Mem_Region_Type Region) -> void*
{
    BGZ_ASSERT((DynamAllocator->MemRegions[Region].UsedAmount - Size) > Size);

    if(BlockToRealloc)
    {
        Block_Header* BlockHeader = GetBlockHeader(BlockToRealloc);

        if (Size < BlockHeader->Size)
        {
            SplitBlock(BlockToRealloc, Size, DynamAllocator->FreeBlocks);
        }
        else
        {
            FreeBlockButDontZero(DynamAllocator, BlockToRealloc, Region);
            void* NewBlock = AppendNewFilledBlock(DynamAllocator, Size, Region);

            memcpy(NewBlock, BlockToRealloc, Size);

            return NewBlock;
        };
    }
    else
    {
        BlockToRealloc = AppendNewFilledBlock(DynamAllocator, Size, Region);
    };

    return BlockToRealloc;
};

auto
_DeAlloc(Dynamic_Mem_Allocator* DynamAllocator, ui64** MemToFree, Mem_Region_Type Region) -> void
{
    if (*MemToFree) 
    {
        BGZ_ASSERT(*MemToFree > DynamAllocator->MemRegions[Region].BaseAddress && *MemToFree < DynamAllocator->MemRegions[Region].EndAddress);

        Block_Header *BlockHeader = GetBlockHeader(*MemToFree);

        FreeBlockButDontZero(DynamAllocator, *MemToFree, Region);

        *MemToFree = nullptr;
    };
};
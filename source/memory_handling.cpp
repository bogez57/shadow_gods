#pragma once
#include <string.h>
#include "memory_handling.h"
#include "list.h"

/*
    TODO: 
    1.) Split MallocType funcion between MallocSize and MallocType
    2.) Maybe get rid of FilledBlocks linked list data structure and use array instead?
    3.) Use better wording to distinguish when I want to refer to memory header and when to a block
    4.) Right now my DeAlloc func is 0'ing the memory. Maybe move this to a calloc function like C does so freeing doesn't
    cost any more than it needs to and you can choose when to use calloc vs malloc (initialized vs uninitialized)
*/

#define PushSize(MemPartition, Size) _PushSize(MemPartition, Size)
auto
_PushSize(Memory_Partition* MemPartition, sizet Size) -> void*
{
    BGZ_ASSERT((MemPartition->UsedAmount + Size) <= MemPartition->Size);
    void* Result = MemPartition->BaseAddress + MemPartition->UsedAmount;
    MemPartition->UsedAmount += (Size);

    return Result;
};

#define PushType(MemPartition, Type, Count) (Type*)_PushType(MemPartition, sizeof(Type), Count)
auto
_PushType(Memory_Partition* MemPartition, ui64 Size, sizet Count) -> void*
{
    BGZ_ASSERT((MemPartition->UsedAmount + Size) <= MemPartition->Size);
    void* Result = MemPartition->BaseAddress + MemPartition->UsedAmount;
    MemPartition->UsedAmount += (Size * Count);

    return Result;
};

#define FreeSize(MemPartition, Size) _FreeSize(MemPartition, Size)
auto
_FreeSize(Memory_Partition* MemPartition, sizet SizeToFree) -> void
{
    BGZ_ASSERT(SizeToFree < MemPartition->Size || SizeToFree < MemPartition->UsedAmount);

    MemPartition->UsedAmount -= SizeToFree;
};

local_func auto
SplitBlock(OUT Memory_Header* BlockToSplit, sizet Size, OUT List* FreeList) -> auto
{
    struct ChangedMemoryLayout {Memory_Header* BlockToSplit{}; List* FreeList{};};
    ChangedMemoryLayout Result{};
    Result.BlockToSplit = BlockToSplit;
    Result.FreeList = FreeList;

    sizet SizeDiff = Result.BlockToSplit->Size - Size;
    Result.BlockToSplit->Size = Size;
    Result.BlockToSplit->IsFree = false;

    if (SizeDiff > sizeof(Memory_Header))
    {
        Memory_Header *NextBlock = (Memory_Header *)(((ui8 *)(Result.BlockToSplit + 1)) + (Result.BlockToSplit->Size));
        NextBlock->Size = SizeDiff;
        NextBlock->IsFree = true;
        NextBlock->prevBlock = Result.BlockToSplit;
        NextBlock->nextBlock = Result.BlockToSplit->nextBlock;
        Result.BlockToSplit->nextBlock = NextBlock;

        list_add(Result.FreeList, NextBlock);
    }
    else
    {
        Result.BlockToSplit->Size += SizeDiff;
    };

    return Result;
};

local_func auto
GetBlockHeader(void* Ptr) -> Memory_Header*
{
    Memory_Header *BlockHeader{};
    BlockHeader = (Memory_Header*)((ui8*)Ptr - sizeof(Memory_Header));

    return BlockHeader;
};

local_func auto
GetFirstFreeBlockOfSize(sizet Size, List* FreeList) -> Memory_Header*
{
    Memory_Header* Result{};

    ListIter FreeListIter{};
    Memory_Header* BlockHeader{};
    list_iter_init(&FreeListIter, FreeList);
    list_get_at(FreeList, FreeListIter.index, &(void*)BlockHeader);

    for (ui32 BlockIndex{0}; BlockIndex < FreeList->size; ++BlockIndex)
    {
        if (BlockHeader)
        {
            if (BlockHeader->IsFree && BlockHeader->Size > Size)
            {
                Result = BlockHeader;
                return Result;
            }
        }

        list_iter_next(&FreeListIter, &(void *)BlockHeader);
    };

    //No free blocks found
    return nullptr;
};

local_func auto
SwapLists(Memory_Header* BlockHeader, List* FromList, List* ToList) -> auto
{
    BGZ_ASSERT(CC_OK == list_remove(FromList, BlockHeader, NULL));
    list_add(ToList, &BlockHeader);
};

local_func auto
AppendNewFilledBlock(OUT Memory_Partition* MemPartition, sizet Size) -> Memory_Header*
{
    ui64 TotalSize = sizeof(Memory_Header) + Size;
    void* NewBlock = PushSize(MemPartition, TotalSize);

    Memory_Header* BlockHeader = (Memory_Header*)NewBlock;
    BlockHeader->Size = Size;
    BlockHeader->IsFree = false;

    list_get_last(MemPartition->FilledBlocks, &(void*)BlockHeader->prevBlock);
    BlockHeader->prevBlock->nextBlock = BlockHeader;
    list_add(MemPartition->FilledBlocks, BlockHeader);

    return BlockHeader;
};

local_func auto
FreeBlockButDontZero(OUT Memory_Partition* MemPartition, OUT Memory_Header* BlockHeaderToFree) -> void
{
    list_remove(MemPartition->FilledBlocks, BlockHeaderToFree, NULL);
    BlockHeaderToFree->IsFree = true;

    if (BlockHeaderToFree->nextBlock)
    {
        if (BlockHeaderToFree->nextBlock->IsFree)
        {
            BlockHeaderToFree->Size += BlockHeaderToFree->nextBlock->Size;
            BlockHeaderToFree->nextBlock->Size = 0;
            list_remove(MemPartition->FreeBlocks, BlockHeaderToFree->nextBlock, NULL);

            if (BlockHeaderToFree->nextBlock->nextBlock)
                BlockHeaderToFree->nextBlock = BlockHeaderToFree->nextBlock->nextBlock;
            else
                BlockHeaderToFree->nextBlock = nullptr;
        };

        if (BlockHeaderToFree->prevBlock->IsFree)
        {
            BlockHeaderToFree->prevBlock->Size += BlockHeaderToFree->Size;
            BlockHeaderToFree->Size = 0;

            if (BlockHeaderToFree->nextBlock)
                BlockHeaderToFree->prevBlock = BlockHeaderToFree->nextBlock;
            else
                BlockHeaderToFree->prevBlock = nullptr;

            return;
        }
    }
    else
    {
        FreeSize(MemPartition, BlockHeaderToFree->Size);
        BlockHeaderToFree->prevBlock->nextBlock = nullptr;
    }

    list_add(MemPartition->FreeBlocks, BlockHeaderToFree);
};

local_func auto
InitMemPartition(OUT Memory_Partition* MemPartition, ui32 SizeToReserve, ui64* StartingAddress) -> void 
{
    MemPartition->BaseAddress = StartingAddress;
    MemPartition->EndAddress = MemPartition->BaseAddress + SizeToReserve;
    MemPartition->Size = SizeToReserve; 
    MemPartition->UsedAmount = 0;

    list_new(&MemPartition->FreeBlocks);
    list_new(&MemPartition->FilledBlocks);

    Memory_Header* InitialBlock = PushType(MemPartition, Memory_Header, 1);
    InitialBlock->Size = 0;
    InitialBlock->IsFree = false;

    list_add_last(MemPartition->FilledBlocks, InitialBlock);
};

auto 
_MallocType(Memory_Partition* MemPartition, sizet Size) -> ui64*
{
    BGZ_ASSERT(Size <= MemPartition->Size);

    ui64* Result{nullptr};

    Memory_Header* BlockHeader = GetFirstFreeBlockOfSize(Size, MemPartition->FreeBlocks);

    //No free blocks found
    if(!BlockHeader)
    {
        BlockHeader = AppendNewFilledBlock(MemPartition, Size);
    }
    else
    {
        auto [UpdatedBlockHeader, UpdatedFreeBlocks] = SplitBlock(BlockHeader, Size, MemPartition->FreeBlocks);
        SwapLists(UpdatedBlockHeader, UpdatedFreeBlocks, MemPartition->FilledBlocks);

        return (ui64*)(UpdatedBlockHeader + 1);
    };

    return (ui64*)(BlockHeader + 1);
};

auto
_ReAlloc(Memory_Partition* MemPartition, void* BlockToRealloc, sizet Size) -> void*
{
    Memory_Header* BlockHeader = GetBlockHeader(BlockToRealloc);

    if(BlockToRealloc)
    {
        if (Size < BlockHeader->Size)
        {
            SplitBlock(BlockHeader, Size, MemPartition->FreeBlocks);
        }
        else
        {
            FreeBlockButDontZero(MemPartition, BlockHeader);
            BlockHeader = AppendNewFilledBlock(MemPartition, Size);
            memcpy((BlockHeader + 1), BlockToRealloc, Size);

            memset(BlockToRealloc, 0, BlockHeader->Size);
        };
    }
    else
    {
        BlockHeader = AppendNewFilledBlock(MemPartition, Size);
    };

    return (void*)BlockHeader;
};

auto
_DeAlloc(Memory_Partition* MemPartition, ui64** MemToFree) -> void
{
    if (*MemToFree) 
    {
        BGZ_ASSERT(*MemToFree > MemPartition->BaseAddress && *MemToFree < MemPartition->EndAddress);

        Memory_Header *BlockHeader = GetBlockHeader(*MemToFree);
        memset(*MemToFree, 0, BlockHeader->Size);

        FreeBlockButDontZero(MemPartition, BlockHeader);

        *MemToFree = nullptr;
    };
};
#pragma once
#include <string.h>
#include "memory_handling.h"
#include "list.h"

/*
    TODO: 
    1.) Maybe get rid of FilledBlocks linked list data structure and use array instead?
    2.) Use better wording to distinguish when I want to refer to memory header and when to a block
    3.) Right now my DeAlloc func is 0'ing the memory. Maybe move this to a calloc function like C does so freeing doesn't
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
_FreeSize(Memory_Partition* MemPartition, ui64 SizeToFree) -> void
{
    BGZ_ASSERT(SizeToFree < MemPartition->Size || SizeToFree < MemPartition->UsedAmount);

    MemPartition->UsedAmount -= SizeToFree;
};

local_func auto
SplitBlock(OUT Memory_Header* BlockToSplit, ui64 Size, OUT List* FreeList) -> void
{
    sizet SizeDiff = BlockToSplit->Size - Size;
    BlockToSplit->Size = Size;
    BlockToSplit->IsFree = false;

    if (SizeDiff > sizeof(Memory_Header))
    {
        Memory_Header *NextBlock = (Memory_Header *)(((ui8 *)(BlockToSplit + 1)) + (BlockToSplit->Size - 1));
        NextBlock->Size = SizeDiff;
        NextBlock->IsFree = true;
        NextBlock->prevBlock = BlockToSplit;
        NextBlock->nextBlock = BlockToSplit->nextBlock;
        BlockToSplit->nextBlock = NextBlock;

        list_add(FreeList, NextBlock);
    }
    else
    {
        BlockToSplit->Size += SizeDiff;
    };
};

local_func auto
GetBlockHeader(void* Ptr) -> Memory_Header*
{
    Memory_Header *BlockHeader{};
    BlockHeader = (Memory_Header*)((ui8*)Ptr - sizeof(Memory_Header));

    return BlockHeader;
};

local_func auto
RemoveHeader(Memory_Header* MemHeader) -> void*
{
    void* ActualBeginningOfBlockMem = (void*)(MemHeader + 1);

    return ActualBeginningOfBlockMem;
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

    Memory_Header* BlockHeader; 
    for (ui32 BlockIndex{0}; BlockIndex < FreeList->size; ++BlockIndex)
    {
        if (MemBlock)
        {
            BlockHeader = (Memory_Header*)MemBlock;
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
SwapLists(Memory_Header* BlockHeader, List* FromList, List* ToList) -> auto
{
    BGZ_ASSERT(CC_OK == list_remove(FromList, BlockHeader, NULL));
    list_add(ToList, &BlockHeader);
};

local_func auto
AppendNewFilledBlock(OUT Memory_Partition* MemPartition, ui64 Size) -> void*
{
    ui64 TotalSize = sizeof(Memory_Header) + Size;
    void* NewBlock = PushSize(MemPartition, TotalSize);

    Memory_Header* BlockHeader = (Memory_Header*)NewBlock;
    BlockHeader->Size = Size;
    BlockHeader->IsFree = false;

    list_get_last(MemPartition->FilledBlocks, &(void*)BlockHeader->prevBlock);
    BlockHeader->prevBlock->nextBlock = BlockHeader;

    list_add(MemPartition->FilledBlocks, NewBlock);

    return NewBlock;
};

local_func auto
FreeBlockButDontZero(OUT Memory_Partition* MemPartition, OUT void* BlockToFree) -> void
{
    list_remove(MemPartition->FilledBlocks, BlockToFree, NULL);
    Memory_Header* BlockHeader = (Memory_Header*)BlockToFree;

    BlockHeader->IsFree = true;

    if (BlockHeader->nextBlock)
    {
        if (BlockHeader->nextBlock->IsFree)
        {
            BlockHeader->Size += BlockHeader->nextBlock->Size;
            BlockHeader->nextBlock->Size = 0;
            list_remove(MemPartition->FreeBlocks, BlockHeader->nextBlock, NULL);

            if (BlockHeader->nextBlock->nextBlock)
                BlockHeader->nextBlock = BlockHeader->nextBlock->nextBlock;
            else
                BlockHeader->nextBlock = nullptr;
        };

        if (BlockHeader->prevBlock->IsFree)
        {
            BlockHeader->prevBlock->Size += BlockHeader->Size;
            BlockHeader->Size = 0;

            if (BlockHeader->nextBlock)
                BlockHeader->prevBlock = BlockHeader->nextBlock;
            else
                BlockHeader->prevBlock = nullptr;

            return;
        }
    }
    else
    {
        FreeSize(MemPartition, BlockHeader->Size);
        BlockHeader->prevBlock->nextBlock = nullptr;
    }

    list_add(MemPartition->FreeBlocks, BlockToFree);
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
_MallocType(Memory_Partition* MemPartition, sizet Size) -> void*
{
    BGZ_ASSERT(Size <= MemPartition->Size);

    ui64* Result{nullptr};

    void* MemBlock = GetFirstFreeBlockOfSize(Size, MemPartition->FreeBlocks);

    Memory_Header* BlockHeader = (Memory_Header*)MemBlock;
    //No free blocks found
    if(!MemBlock)
    {
        MemBlock = AppendNewFilledBlock(MemPartition, Size);
        BlockHeader = (Memory_Header*)MemBlock;
    }
    else
    {
        SplitBlock(BlockHeader, Size, MemPartition->FreeBlocks);
        SwapLists(BlockHeader, MemPartition->FreeBlocks, MemPartition->FilledBlocks);

        MemBlock = RemoveHeader(BlockHeader);
        return MemBlock;
    };

    MemBlock = RemoveHeader(BlockHeader);
    return MemBlock;
};

auto
_ReAlloc(Memory_Partition* MemPartition, void* BlockToRealloc, ui64 Size) -> void*
{
    if(BlockToRealloc)
    {
        Memory_Header* BlockHeader = GetBlockHeader(BlockToRealloc);
        BlockToRealloc = (void*)BlockHeader;

        if (Size < BlockHeader->Size)
        {
            SplitBlock(BlockHeader, Size, MemPartition->FreeBlocks);
        }
        else
        {
            FreeBlockButDontZero(MemPartition, BlockToRealloc);
            void* NewBlock = AppendNewFilledBlock(MemPartition, Size);
            Memory_Header* NewBlockHeader = (Memory_Header*)NewBlock;

            memcpy((NewBlockHeader + 1), BlockToRealloc, Size);

            memset(BlockToRealloc, 0, BlockHeader->Size);

            NewBlock = RemoveHeader(NewBlockHeader);
            return NewBlock;
        };
    }
    else
    {
        BlockToRealloc = AppendNewFilledBlock(MemPartition, Size);
    };

    return BlockToRealloc;
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
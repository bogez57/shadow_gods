#pragma once
#include <string.h>
#include "memory_handling.h"
#include "list.h"

/*
    TODO: 
    1.) Maybe get rid of FilledBlocks linked list data structure and use array instead?
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
GetBlockHeader(void* Ptr) -> Block_Header*
{
    Block_Header *BlockHeader{};
    BlockHeader = (Block_Header*)((ui8*)Ptr - sizeof(Block_Header));

    return BlockHeader;
};

local_func auto
RemoveHeader(void* MemBlock) -> void*
{
    Block_Header* MemHeader = (Block_Header*)MemBlock;
    void* ActualBeginningOfBlockMem = (void*)(MemHeader + 1);

    return ActualBeginningOfBlockMem;
};

local_func auto
SplitBlock(OUT void* BlockToSplit, ui64 Size, OUT List* FreeList) -> void
{
    Block_Header* BlockHeader = (Block_Header*)BlockToSplit;
    sizet SizeDiff = BlockHeader->Size - Size;
    BlockHeader->Size = Size;
    BlockHeader->IsFree = false;

    if (SizeDiff > sizeof(Block_Header))
    {
        void* NextBlock = (((ui8 *)(RemoveHeader(BlockToSplit))) + (BlockHeader->Size - 1));
        Block_Header* NextBlockHeader = (Block_Header*)NextBlock;

        NextBlockHeader ->Size = SizeDiff;
        NextBlockHeader ->IsFree = true;
        NextBlockHeader ->prevBlock = BlockHeader;
        NextBlockHeader ->nextBlock = BlockHeader->nextBlock;
        BlockHeader->nextBlock = NextBlockHeader;

        list_add(FreeList, NextBlock);
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

    Block_Header* BlockHeader; 
    for (ui32 BlockIndex{0}; BlockIndex < FreeList->size; ++BlockIndex)
    {
        if (MemBlock)
        {
            BlockHeader = (Block_Header*)MemBlock;
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
SwapLists(void* BlockToSwap, List* FromList, List* ToList) -> auto
{
    BGZ_ASSERT(CC_OK == list_remove(FromList, BlockToSwap, NULL));
    list_add(ToList, &BlockToSwap);
};

local_func auto
AppendNewFilledBlock(OUT Memory_Partition* MemPartition, ui64 Size) -> void*
{
    ui64 TotalSize = sizeof(Block_Header) + Size;
    void* NewBlock = PushSize(MemPartition, TotalSize);

    Block_Header* BlockHeader = (Block_Header*)NewBlock;
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
    Block_Header* BlockHeader = (Block_Header*)BlockToFree;

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

    Block_Header* InitialBlock = PushType(MemPartition, Block_Header, 1);
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

    Block_Header* BlockHeader = (Block_Header*)MemBlock;
    //No free blocks found
    if(!MemBlock)
    {
        MemBlock = AppendNewFilledBlock(MemPartition, Size);
    }
    else
    {
        SplitBlock(MemBlock, Size, MemPartition->FreeBlocks);
        SwapLists(MemBlock, MemPartition->FreeBlocks, MemPartition->FilledBlocks);

        MemBlock = RemoveHeader(MemBlock);
        return MemBlock;
    };

    MemBlock = RemoveHeader(MemBlock);
    return MemBlock;
};

auto
_ReAlloc(Memory_Partition* MemPartition, void* BlockToRealloc, ui64 Size) -> void*
{
    if(BlockToRealloc)
    {
        Block_Header* BlockHeader = GetBlockHeader(BlockToRealloc);
        BlockToRealloc = (void*)BlockHeader;

        if (Size < BlockHeader->Size)
        {
            SplitBlock(BlockHeader, Size, MemPartition->FreeBlocks);
        }
        else
        {
            FreeBlockButDontZero(MemPartition, BlockToRealloc);
            void* NewBlock = AppendNewFilledBlock(MemPartition, Size);
            Block_Header* NewBlockHeader = (Block_Header*)NewBlock;

            memcpy(RemoveHeader(NewBlock), BlockToRealloc, Size);

            memset(BlockToRealloc, 0, BlockHeader->Size);

            NewBlock = RemoveHeader(NewBlock);
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

        Block_Header *BlockHeader = GetBlockHeader(*MemToFree);
        memset(*MemToFree, 0, BlockHeader->Size);

        FreeBlockButDontZero(MemPartition, BlockHeader);

        *MemToFree = nullptr;
    };
};
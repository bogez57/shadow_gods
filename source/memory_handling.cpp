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
SwapLists(void* BlockToSwap, List* FromList, List* ToList) -> auto
{
    BGZ_ASSERT(CC_OK == list_remove(FromList, BlockToSwap, NULL));
    list_add(ToList, BlockToSwap);
};

local_func auto
AppendNewFilledBlock(OUT Memory_Partition* MemPartition, ui64 Size) -> void*
{
    ui64 TotalSize = sizeof(Block_Header) + Size;
    Block_Header* BlockHeader = (Block_Header*)PushSize(MemPartition, TotalSize);

    BlockHeader->Size = Size;
    BlockHeader->IsFree = false;


    list_get_last(MemPartition->FilledBlocks, &BlockHeader->prevBlock);

    void* NewBlock = GetBlockData(BlockHeader);
    GetBlockHeader(BlockHeader->prevBlock)->nextBlock = NewBlock;

    list_add(MemPartition->FilledBlocks, NewBlock);

    return NewBlock;
};

local_func auto
FreeBlockButDontZero(OUT Memory_Partition* MemPartition, OUT void* BlockToFree) -> void
{
    list_remove(MemPartition->FilledBlocks, BlockToFree, NULL);
    Block_Header* BlockHeader = GetBlockHeader(BlockToFree);

    BlockHeader->IsFree = true;

    if (BlockHeader->nextBlock)
    {
        if (GetBlockHeader(BlockHeader->nextBlock)->IsFree)
        {
            BlockHeader->Size += GetBlockHeader(BlockHeader->nextBlock)->Size;
            list_remove(MemPartition->FreeBlocks, BlockHeader->nextBlock, NULL);

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
        FreeSize(MemPartition, BlockHeader->Size);
        GetBlockHeader(BlockHeader->prevBlock)->nextBlock = nullptr;
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

    ui16 BlockSize = 8;
    ui16 TotalSize = sizeof(Block_Header) + BlockSize;
    Block_Header* InitialBlockHeader = (Block_Header*)PushSize(MemPartition, TotalSize);

    InitialBlockHeader->Size = BlockSize;
    InitialBlockHeader->IsFree = false;

    void* InitialBlock = GetBlockData(InitialBlockHeader);
    list_add_last(MemPartition->FilledBlocks, InitialBlock);
};

auto 
_MallocType(Memory_Partition* MemPartition, sizet Size) -> void*
{
    BGZ_ASSERT(Size <= MemPartition->Size);

    void* Result{nullptr};

    if(Size > 0)
    {
        void *MemBlock = GetFirstFreeBlockOfSize(Size, MemPartition->FreeBlocks);

        //No free blocks found
        if (!MemBlock)
        {
            MemBlock = AppendNewFilledBlock(MemPartition, Size);
        }
        else
        {
            SplitBlock(MemBlock, Size, MemPartition->FreeBlocks);
            SwapLists(MemBlock, MemPartition->FreeBlocks, MemPartition->FilledBlocks);

            Result = MemBlock;
            return Result;
        };

        Result = MemBlock;
        return MemBlock;
    };

    return Result;
};

auto
_CallocType(Memory_Partition* MemPartition, sizet Size) -> void*
{
    BGZ_ASSERT(Size <= MemPartition->Size);

    void* MemBlock = _MallocType(MemPartition, Size);

    if(MemBlock)
    {
        Block_Header *BlockHeader = GetBlockHeader(MemBlock);
        memset(MemBlock, 0, BlockHeader->Size);
    };

    return MemBlock;
};

auto
_ReAlloc(Memory_Partition* MemPartition, void* BlockToRealloc, ui64 Size) -> void*
{
    BGZ_ASSERT((MemPartition->UsedAmount - Size) > Size);

    if(BlockToRealloc)
    {
        Block_Header* BlockHeader = GetBlockHeader(BlockToRealloc);

        if (Size < BlockHeader->Size)
        {
            SplitBlock(BlockToRealloc, Size, MemPartition->FreeBlocks);
        }
        else
        {
            FreeBlockButDontZero(MemPartition, BlockToRealloc);
            void* NewBlock = AppendNewFilledBlock(MemPartition, Size);

            memcpy(NewBlock, BlockToRealloc, Size);

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

        FreeBlockButDontZero(MemPartition, *MemToFree);

        *MemToFree = nullptr;
    };
};
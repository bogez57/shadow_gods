#pragma once
#include <string.h>
#include "list.h"

/*
    TODO: 
    1.) How to void out the dellocated pointer so that it does not still point to the memory address after
    being deallocated
*/

struct Memory_Header
{
    b IsFree{true};
    sizet Size{0};
    Memory_Header* nextBlock{nullptr};
    Memory_Header* prevBlock{nullptr};
};

struct Memory_Chunk
{
    ui64* BaseAddress{nullptr};
    ui64* EndAddress{nullptr};
    ui32 Size{0};
    ui64 UsedAmount{0};
    List* FreeBlocks{};
    List* FilledBlocks{};
};

#define PushSize(MemoryChunk, Size) _PushSize(MemoryChunk, Size)
auto
_PushSize(Memory_Chunk* MemoryChunk, sizet Size) -> void*
{
    BGZ_ASSERT((MemoryChunk->UsedAmount + Size) <= MemoryChunk->Size);
    void* Result = MemoryChunk->BaseAddress + MemoryChunk->UsedAmount;
    MemoryChunk->UsedAmount += (Size);

    return Result;
};

#define PushType(MemoryChunk, Type, Count) (Type*)_PushType(MemoryChunk, sizeof(Type), Count)
auto
_PushType(Memory_Chunk* MemoryChunk, ui64 Size, sizet Count) -> void*
{
    BGZ_ASSERT((MemoryChunk->UsedAmount + Size) <= MemoryChunk->Size);
    void* Result = MemoryChunk->BaseAddress + MemoryChunk->UsedAmount;
    MemoryChunk->UsedAmount += (Size * Count);

    return Result;
};

#define FreeSize(MemoryChunk, Size) _FreeSize(MemoryChunk, Size)
auto
_FreeSize(Memory_Chunk* MemoryChunk, sizet SizeToFree) -> void
{
    BGZ_ASSERT(SizeToFree < MemoryChunk->Size || SizeToFree < MemoryChunk->UsedAmount);

    MemoryChunk->UsedAmount -= SizeToFree;
};

local_func auto
SplitBlock(Memory_Header* BlockToSplit, sizet Size, List* FreeList) -> void
{
    sizet SizeDiff = BlockToSplit->Size - Size;
    BlockToSplit->Size = Size;
    BlockToSplit->IsFree = false;

    if (SizeDiff > sizeof(Memory_Header))
    {
        Memory_Header *NextBlock = (Memory_Header *)(((ui8 *)(BlockToSplit + 1)) + (BlockToSplit->Size));
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
AppendNewFilledBlock(Memory_Chunk* MemoryChunk, sizet Size) -> Memory_Header*
{
    ui64 TotalSize = sizeof(Memory_Header) + Size;
    void* NewBlock = PushSize(MemoryChunk, TotalSize);

    Memory_Header* BlockHeader = (Memory_Header*)NewBlock;
    BlockHeader->Size = Size;
    BlockHeader->IsFree = false;

    list_get_last(MemoryChunk->FilledBlocks, &(void*)BlockHeader->prevBlock);
    BlockHeader->prevBlock->nextBlock = BlockHeader;
    list_add(MemoryChunk->FilledBlocks, BlockHeader);

    return BlockHeader;
};

local_func auto
FreeBlockButDontZero(Memory_Chunk* MemoryChunk, Memory_Header* BlockHeaderToFree) -> void
{
    list_remove(MemoryChunk->FilledBlocks, BlockHeaderToFree, NULL);
    BlockHeaderToFree->IsFree = true;

    if (BlockHeaderToFree->nextBlock)
    {
        if (BlockHeaderToFree->nextBlock->IsFree)
        {
            BlockHeaderToFree->Size += BlockHeaderToFree->nextBlock->Size;
            BlockHeaderToFree->nextBlock->Size = 0;
            list_remove(MemoryChunk->FreeBlocks, BlockHeaderToFree->nextBlock, NULL);

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
        FreeSize(MemoryChunk, BlockHeaderToFree->Size);
        BlockHeaderToFree->prevBlock->nextBlock = nullptr;
    }

    list_add(MemoryChunk->FreeBlocks, BlockHeaderToFree);
};

local_func auto
InitMemoryChunk(Memory_Chunk* MemoryChunk, ui32 SizeToReserve, ui64* StartingAddress) -> void
{
    MemoryChunk->BaseAddress = StartingAddress;
    MemoryChunk->EndAddress = MemoryChunk->BaseAddress + SizeToReserve;
    MemoryChunk->Size = SizeToReserve; 
    MemoryChunk->UsedAmount = 0;

    list_new(&MemoryChunk->FreeBlocks);
    list_new(&MemoryChunk->FilledBlocks);

    Memory_Header* InitialBlock = PushType(MemoryChunk, Memory_Header, 1);
    InitialBlock->Size = 0;
    InitialBlock->IsFree = false;

    list_add_last(MemoryChunk->FilledBlocks, InitialBlock);
};

#define MyMalloc(MemoryChunk, Size, Count) _MyMalloc(MemoryChunk, Size * Count)
auto 
_MyMalloc(Memory_Chunk* MemoryChunk, sizet Size) -> ui64*
{
    BGZ_ASSERT(Size <= MemoryChunk->Size);

    ui64* Result{nullptr};

    ListIter FreeIter{};
    list_iter_init(&FreeIter, MemoryChunk->FreeBlocks);

    Memory_Header* BlockHeader{};
    list_get_at(MemoryChunk->FreeBlocks, FreeIter.index, &(void*)BlockHeader);

    for (ui32 BlockIndex{0}; BlockIndex < MemoryChunk->FreeBlocks->size; ++BlockIndex)
    {
        if (BlockHeader)
        {
            if (BlockHeader->IsFree && BlockHeader->Size > Size)
            {
                list_remove(MemoryChunk->FreeBlocks, BlockHeader, NULL);
                list_add(MemoryChunk->FilledBlocks, BlockHeader);

                SplitBlock(BlockHeader, Size, MemoryChunk->FreeBlocks);

                Result = (ui64 *)(BlockHeader + 1);
                return Result;
            }
        }

        list_iter_next(&FreeIter, &(void*)BlockHeader);
    };

    //If no right sized free blocks
    BlockHeader = AppendNewFilledBlock(MemoryChunk, Size);

    return (ui64*)(BlockHeader + 1);
};

#define MyReAlloc(MemoryChunk, Ptr, Type, Count) (Type*)_MyReAlloc(MemoryChunk, Ptr, sizeof(Type) * Count)
auto
_MyReAlloc(Memory_Chunk* MemoryChunk, void* BlockToRealloc, sizet Size) -> void*
{
    Memory_Header* BlockHeader = GetBlockHeader(BlockToRealloc);

    if(BlockToRealloc)
    {
        if (Size < BlockHeader->Size)
        {
            SplitBlock(BlockHeader, Size, MemoryChunk->FreeBlocks);
        }
        else
        {
            FreeBlockButDontZero(MemoryChunk, BlockHeader);
            BlockHeader = AppendNewFilledBlock(MemoryChunk, Size);
            memcpy((BlockHeader + 1), BlockToRealloc, Size);

            memset(BlockToRealloc, 0, BlockHeader->Size);
        };
    }
    else
    {
        BlockHeader = AppendNewFilledBlock(MemoryChunk, Size);
    };

    return (void*)BlockHeader;
};

#define MyDeAlloc(MemoryChunk, PtrToMemory) _MyDeAlloc(MemoryChunk, (ui64*)PtrToMemory)
auto
_MyDeAlloc(Memory_Chunk* MemoryChunk, ui64* MemToFree) -> void
{
    if (MemToFree) 
    {
        BGZ_ASSERT(MemToFree > MemoryChunk->BaseAddress && MemToFree < MemoryChunk->EndAddress);

        Memory_Header *BlockHeader = GetBlockHeader(MemToFree);
        memset(MemToFree, 0, BlockHeader->Size);

        FreeBlockButDontZero(MemoryChunk, BlockHeader);
    };
};
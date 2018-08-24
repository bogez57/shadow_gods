#pragma once
#include <string.h>
#include "list.h"

#define FIXED_BLOCK_SIZE 500

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

///////////////////////////////////////////

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
                sizet SizeDiff = BlockHeader->Size - Size;
                BlockHeader->Size = Size;
                BlockHeader->IsFree = false;

                list_remove(MemoryChunk->FreeBlocks, BlockHeader, NULL);
                list_add(MemoryChunk->FilledBlocks, BlockHeader);

                if (SizeDiff > sizeof(Memory_Header))
                {
                    Memory_Header *NextBlock = (Memory_Header*)((ui8*)BlockHeader + BlockHeader->Size);
                    NextBlock->Size = SizeDiff;
                    NextBlock->IsFree = true;
                    NextBlock->prevBlock = BlockHeader;
                    NextBlock->nextBlock = BlockHeader->nextBlock;
                    BlockHeader->nextBlock = NextBlock;

                    list_add(MemoryChunk->FreeBlocks, NextBlock);
                }
                else
                {
                    BlockHeader->Size += SizeDiff;
                }

                Result = (ui64 *)(BlockHeader + 1);
                return Result;
            }
        }

        list_iter_next(&FreeIter, &(void*)BlockHeader);
    };

    //If no right sized free blocks
    ui64 TotalSize = sizeof(Memory_Header) + Size;
    void* NewBlock = PushSize(MemoryChunk, TotalSize);

    BlockHeader = (Memory_Header*)NewBlock;
    BlockHeader->Size = Size;
    BlockHeader->IsFree = false;

    list_get_last(MemoryChunk->FilledBlocks, &(void*)BlockHeader->prevBlock);
    BlockHeader->prevBlock->nextBlock = BlockHeader;
    list_add(MemoryChunk->FilledBlocks, BlockHeader);

    return (ui64*)(BlockHeader + 1);
};

#define MyDeAlloc(MemoryChunk, PtrToMemory) _MyDeAlloc(MemoryChunk, (ui64*)PtrToMemory)
auto
_MyDeAlloc(Memory_Chunk* MemoryChunk, ui64* MemToFree) -> void
{
    if (MemToFree && MemToFree > MemoryChunk->BaseAddress && MemToFree < MemoryChunk->EndAddress)
    {
        Memory_Header *BlockHeader{};
        BlockHeader = (Memory_Header *)MemToFree - 1;

        list_remove(MemoryChunk->FilledBlocks, BlockHeader, NULL);
        BlockHeader->IsFree = true;

        //If not the last block in the memory chunk
        if (BlockHeader->nextBlock)
        {
            if (BlockHeader->nextBlock->IsFree)
            {
                BlockHeader->Size += BlockHeader->nextBlock->Size;
                BlockHeader->nextBlock->Size = 0;
                list_remove(MemoryChunk->FreeBlocks, BlockHeader->nextBlock, NULL);

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
            FreeSize(MemoryChunk, BlockHeader->Size);
            BlockHeader->prevBlock->nextBlock = nullptr;
        }

        list_add(MemoryChunk->FreeBlocks, BlockHeader);
    };
};
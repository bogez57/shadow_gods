#pragma once
#include <string.h>
#include "list.h"

#define FIXED_BLOCK_SIZE 500

struct Memory_Block
{
    b IsFree{true};
    ui32 Size{0};
    ui64* BaseAddress{nullptr};
    Memory_Block* nextBlock{nullptr};
    Memory_Block* prevBlock{nullptr};
};

struct Memory_Chunk
{
    ui64* BaseAddress{nullptr};
    ui64* EndAddress{nullptr};
    ui32 Size{0};
    ui64 UsedAmount{0};
    Memory_Block MemBlocks[10000] = {};
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

///////////////////////////////////////////

local_func auto
InitMemoryChunk(Memory_Chunk* MemoryChunk, ui32 SizeToReserve, ui64* StartingAddress) -> void
{
    MemoryChunk->BaseAddress = StartingAddress;
    MemoryChunk->EndAddress = MemoryChunk->BaseAddress + SizeToReserve;
    MemoryChunk->Size = SizeToReserve; 
    MemoryChunk->UsedAmount = 0;

    for(ui32 BlockIndex{0}; BlockIndex < ArrayCount(MemoryChunk->MemBlocks); ++BlockIndex)
    {
        MemoryChunk->MemBlocks[BlockIndex].Size = 0;
        MemoryChunk->MemBlocks[BlockIndex].IsFree = true;
        MemoryChunk->MemBlocks[BlockIndex].BaseAddress = (ui64 *)PushSize(MemoryChunk, FIXED_BLOCK_SIZE);

        if(BlockIndex != ArrayCount(MemoryChunk->MemBlocks))
            MemoryChunk->MemBlocks[BlockIndex].nextBlock = &MemoryChunk->MemBlocks[BlockIndex + 1];
        if(BlockIndex != 0)
            MemoryChunk->MemBlocks[BlockIndex].prevBlock = &MemoryChunk->MemBlocks[BlockIndex - 1];
        if(BlockIndex == 0)
            MemoryChunk->MemBlocks[BlockIndex].Size = MemoryChunk->Size;
    };
};

#define MyMalloc(MemoryChunk, Size, Count) _MyMalloc(MemoryChunk, Size * Count)
auto 
_MyMalloc(Memory_Chunk* MemoryChunk, ui32 Size) -> ui64*
{
    BGZ_ASSERT(Size <= MemoryChunk->Size);

    for(ui32 BlockIndex{0}; BlockIndex < ArrayCount(MemoryChunk->MemBlocks); ++BlockIndex)
    {
        Memory_Block* Block = &MemoryChunk->MemBlocks[BlockIndex];
        if(Block->IsFree)
        {
            if(Block->Size > Size)
            {
                ui32 SizeDiff = Block->Size - Size;
                Block->Size = Size;
                Block->IsFree = false;

                ui32 Multiple = FIXED_BLOCK_SIZE;
                Block->Size = RoundUp(Block->Size, Multiple);

                if(Block->nextBlock->IsFree)
                {
                    Block->nextBlock->Size = RoundDown(SizeDiff, Multiple);
                };

                return Block->BaseAddress;
            };
        };
    };

    //No free blocks left
    ui64* Result{nullptr};
    return Result;
};

//Memory_Block* MemoryBlockToFree = (Memory_Block*)(MemToFree - sizeof(Memory_Block));
#define MyDeAlloc(MemoryChunk, PtrToMemory) _MyDeAlloc(MemoryChunk, (ui64**)PtrToMemory)
auto
_MyDeAlloc(Memory_Chunk* MemoryChunk, ui64** MemToFree) -> void
{
    BGZ_ASSERT(MemToFree);

    for(ui32 BlockIndex{0}; BlockIndex < ArrayCount(MemoryChunk->MemBlocks); ++BlockIndex)
    {
        if(MemoryChunk->MemBlocks[BlockIndex].BaseAddress == *MemToFree)
        {
            Memory_Block* Block = &MemoryChunk->MemBlocks[BlockIndex];
            memset(Block->BaseAddress, 0, Block->Size);
            Block->IsFree = true;

            if(Block->nextBlock->IsFree)
            {
                Block->Size += Block->nextBlock->Size;
                Block->nextBlock->Size = 0;
            };

            if(Block->prevBlock->IsFree)
            {
                Block->prevBlock->Size += Block->Size;
                Block->Size = 0;
            };

            *MemToFree = nullptr;

            return;
        };
    };
};
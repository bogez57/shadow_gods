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
    ui64 Size{0};
    ui64 UsedAmount{0};
    Memory_Block MemBlocks[10000] = {};
    List* FreeList;
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
InitMemoryChunk(Memory_Chunk* MemoryChunk, sizet SizeToReserve, ui64* StartingAddress) -> void
{
    MemoryChunk->BaseAddress = StartingAddress;
    MemoryChunk->EndAddress = MemoryChunk->BaseAddress + SizeToReserve;
    MemoryChunk->Size = SizeToReserve; 
    MemoryChunk->UsedAmount = 0;
    list_new(&MemoryChunk->FreeList);

    for(ui32 BlockIndex{0}; BlockIndex < ArrayCount(MemoryChunk->MemBlocks); ++BlockIndex)
    {
        MemoryChunk->MemBlocks[BlockIndex].Size = FIXED_BLOCK_SIZE;
        MemoryChunk->MemBlocks[BlockIndex].IsFree = true;
        MemoryChunk->MemBlocks[BlockIndex].BaseAddress = (ui64 *)PushSize(MemoryChunk, FIXED_BLOCK_SIZE);

        if(BlockIndex != ArrayCount(MemoryChunk->MemBlocks))
            MemoryChunk->MemBlocks[BlockIndex].nextBlock = &MemoryChunk->MemBlocks[BlockIndex + 1];

        if(BlockIndex != 0)
            MemoryChunk->MemBlocks[BlockIndex].prevBlock = &MemoryChunk->MemBlocks[BlockIndex - 1];

        list_add(MemoryChunk->FreeList, &MemoryChunk->MemBlocks[BlockIndex]);
    };
};

#define MyMalloc(MemoryChunk, Size, Count) _MyMalloc(MemoryChunk, Size, Count)
auto 
_MyMalloc(Memory_Chunk* MemoryChunk, ui32 Size, ui32 Count) -> ui64*
{
    BGZ_ASSERT(Size <= FIXED_BLOCK_SIZE);

    ListIter Iterator;
    list_iter_init(&Iterator, MemoryChunk->FreeList);

    Memory_Block* MemBlock{};
    for(ui32 BlockIndex{0}; BlockIndex < MemoryChunk->FreeList->size; ++BlockIndex)
    {
        list_iter_next(&Iterator, &(void*)MemBlock);

        if (MemBlock->IsFree)
        {
            MemBlock->IsFree = false;

            return MemBlock->BaseAddress;
        }
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
            memset(MemoryChunk->MemBlocks[BlockIndex].BaseAddress, 0, MemoryChunk->MemBlocks[BlockIndex].Size);
            MemoryChunk->MemBlocks[BlockIndex].IsFree = true;

            list_add(MemoryChunk->FreeList, &MemoryChunk->MemBlocks[BlockIndex]);

            *MemToFree = nullptr;

            return;
        };
    };
};
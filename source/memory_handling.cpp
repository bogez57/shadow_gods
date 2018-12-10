#pragma once

#include "memory_handling.h"
#include <assert.h>
#include <cstring>

#define ASSERT(x) assert(x)

struct Memory_Region
{
    void* BaseAddress;
    void* EndAddress;
    i64 UsedAmount;
    i64 Size;
};

Memory_Region mainMemoryRegion;

#define AllocType(Type, Count) (Type*)_AllocType(sizeof(Type), Count)
#define AllocSize(Size) _AllocSize(Size)
#define FreeSize(Size) _FreeSize(Size)

void* PointerAddition(void* baseAddress, ui64 amountToAdvancePointer)
{
    void* newAddress {};

    newAddress = ((ui8*)baseAddress) + amountToAdvancePointer;

    return newAddress;
};

void InitApplicationMemory(Application_Memory* appMemory, ui64 sizeOfMemory, void* memoryStartAddress)
{
    ui32 sizeOfPermanentStorage = 6000000;
    appMemory->SizeOfPermanentStorage = sizeOfPermanentStorage;
    appMemory->SizeOfTemporaryStorage = sizeOfMemory - (ui64)sizeOfPermanentStorage;
    appMemory->TotalSize = sizeOfMemory;
    appMemory->PermanentStorage = memoryStartAddress;
    appMemory->TemporaryStorage = ((ui8*)appMemory->PermanentStorage + appMemory->SizeOfPermanentStorage);
};

void InitDynamAllocator();

void CreateRegionFromMemory(Application_Memory* Memory, i64 size)
{
    mainMemoryRegion.BaseAddress = PointerAddition(Memory->TemporaryStorage, Memory->TemporaryStorageUsed);
    mainMemoryRegion.EndAddress = PointerAddition(mainMemoryRegion.BaseAddress, (size - 1));
    mainMemoryRegion.Size = size;
    mainMemoryRegion.UsedAmount = 0;

    Memory->TemporaryStorageUsed += size;

    InitDynamAllocator();
};

auto _AllocType(i64 size, i64 Count) -> void*
{
    ASSERT((mainMemoryRegion.UsedAmount + size) <= mainMemoryRegion.Size);
    void* Result = PointerAddition(mainMemoryRegion.BaseAddress, mainMemoryRegion.UsedAmount);
    mainMemoryRegion.UsedAmount += (size * Count);

    return Result;
};

auto _AllocSize(i64 size) -> void*
{
    ASSERT((mainMemoryRegion.UsedAmount + size) <= mainMemoryRegion.Size);
    void* Result = PointerAddition(mainMemoryRegion.BaseAddress, mainMemoryRegion.UsedAmount);
    mainMemoryRegion.UsedAmount += (size);

    return Result;
};

auto _FreeSize(i64 sizeToFree) -> void
{
    ASSERT(sizeToFree < mainMemoryRegion.Size);
    ASSERT(sizeToFree < mainMemoryRegion.UsedAmount);

    mainMemoryRegion.UsedAmount -= sizeToFree;
};

/******************************************************* 
   Dynamic Allocator
********************************************************/

/*
    TODO: 
    1.) Maybe get rid of Blocks linked list data structure and use array instead?
    2.) Alignment?
*/

struct Memory_Block
{
    b IsFree { true };
    i64 Size { 0 };
    void* data { nullptr };
    Memory_Block* nextBlock { nullptr };
    Memory_Block* prevBlock { nullptr };

    void FreeBlockAndMergeIfNecessary();
};

void* GetDataFromBlock(Memory_Block* Header)
{
    ASSERT(Header->Size != 0);

    void* BlockData = (void*)(Header + 1);

    return BlockData;
};

struct Dynamic_Mem_Allocator
{
    Memory_Block* head;
    Memory_Block* tail;
    ui32 AmountOfBlocks;
};

static Dynamic_Mem_Allocator dynamAllocator;

Memory_Block* ConvertDataToMemoryBlock(void* Ptr)
{
    Memory_Block* BlockHeader {};
    BlockHeader = (Memory_Block*)(((ui8*)Ptr) - (sizeof(Memory_Block)));
    BlockHeader->data = Ptr;

    return BlockHeader;
};

void InitDynamAllocator()
{
    dynamAllocator.AmountOfBlocks = 0;

    ui16 BlockSize = 8;
    ui16 TotalSize = sizeof(Memory_Block) + BlockSize;
    Memory_Block* InitialBlock = (Memory_Block*)AllocSize(TotalSize);

    InitialBlock->Size = BlockSize;
    InitialBlock->IsFree = false;
    InitialBlock->data = GetDataFromBlock(InitialBlock);
    InitialBlock->nextBlock = nullptr;
    InitialBlock->prevBlock = nullptr;

    dynamAllocator.head = InitialBlock;
    dynamAllocator.tail = InitialBlock;
};

Memory_Block* SplitBlock(Memory_Block* BlockToSplit, sizet SizeOfNewBlock)
{
    ASSERT(BlockToSplit->data);
    ASSERT(SizeOfNewBlock > sizeof(Memory_Block));

    Memory_Block* NewBlock = (Memory_Block*)((ui8*)(BlockToSplit) + (BlockToSplit->Size - 1) + (sizeof(Memory_Block)));

    NewBlock->Size = SizeOfNewBlock;
    NewBlock->data = GetDataFromBlock(NewBlock);
    NewBlock->prevBlock = BlockToSplit;
    NewBlock->nextBlock = BlockToSplit->nextBlock;
    BlockToSplit->nextBlock = NewBlock;

    ++dynamAllocator.AmountOfBlocks;

    return NewBlock;
};

sizet ReSizeAndMarkAsInUse(Memory_Block* BlockToResize, sizet NewSize)
{
    sizet SizeDiff = BlockToResize->Size - NewSize;
    BlockToResize->Size = NewSize;
    BlockToResize->IsFree = false;

    return SizeDiff;
};

Memory_Block* GetFirstFreeBlockOfSize(i64 Size)
{
    ASSERT(dynamAllocator.head);

    Memory_Block* Result {};
    Memory_Block* MemBlock = dynamAllocator.head;

    //Not first or last block
    if (MemBlock->nextBlock != nullptr)
    {
        for (ui32 BlockIndex { 0 }; BlockIndex < dynamAllocator.AmountOfBlocks; ++BlockIndex)
        {
            if (MemBlock->IsFree && MemBlock->Size > Size)
            {
                Result = MemBlock;
                return Result;
            }
            else
            {
                MemBlock = MemBlock->nextBlock;
            }
        };
    };

    //No free blocks found
    return nullptr;
};

void Memory_Block::FreeBlockAndMergeIfNecessary() //TODO: split up this function? What to do with more than 1 param that needs modified?
{
    this->IsFree = true;

    //If not the last block
    if (this->nextBlock)
    {
        { //Merge block with next and/or previous blocks
            if (this->nextBlock->IsFree)
            {
                this->Size += this->nextBlock->Size;
                this->nextBlock->Size = 0;

                if (this->nextBlock->nextBlock)
                {
                    this->nextBlock = this->nextBlock->nextBlock;
                }
                else
                {
                    this->nextBlock = nullptr;
                };

                --dynamAllocator.AmountOfBlocks;
            };

            if (this->prevBlock->IsFree)
            {
                this->prevBlock->Size += this->Size;

                if (this->nextBlock)
                {
                    this->prevBlock->nextBlock = this->nextBlock;
                }
                else
                {
                    this->prevBlock->nextBlock = nullptr;
                };

                this->Size = 0;

                --dynamAllocator.AmountOfBlocks;
            };
        };
    }
    else
    {
        FreeSize(this->Size);
        this->prevBlock->nextBlock = nullptr;
        dynamAllocator.tail = this->prevBlock;
        --dynamAllocator.AmountOfBlocks;
    };
};

Memory_Block* AppendNewBlockAndMarkInUse(i64 Size)
{
    i64 TotalSize = sizeof(Memory_Block) + Size;
    Memory_Block* NewBlock = (Memory_Block*)AllocSize(TotalSize);

    NewBlock->Size = Size;
    NewBlock->IsFree = false;
    NewBlock->data = GetDataFromBlock(NewBlock);

    NewBlock->prevBlock = dynamAllocator.tail;
    NewBlock->nextBlock = nullptr;
    dynamAllocator.tail->nextBlock = NewBlock;
    dynamAllocator.tail = NewBlock;

    ++dynamAllocator.AmountOfBlocks;

    return NewBlock;
};

void* _MallocType(i64 Size)
{
    ASSERT(dynamAllocator.head);
    //ASSERT(Size <= (globalMemHandler->memRegions[DYNAMIC].Size - globalMemHandler->memRegions[DYNAMIC].UsedAmount), "Not enough memory left for dynmaic memory allocation!");

    void* Result { nullptr };

    if (Size > 0)
    {
        Memory_Block* MemBlock = GetFirstFreeBlockOfSize(Size);

        //No free blocks found
        if (!MemBlock)
        {
            MemBlock = AppendNewBlockAndMarkInUse(Size);

            Result = MemBlock->data;
            return Result;
        }
        else
        {
            sizet SizeDiff = ReSizeAndMarkAsInUse(MemBlock, Size);

            if (SizeDiff > sizeof(Memory_Block))
            {
                Memory_Block* NewBlock = SplitBlock(MemBlock, SizeDiff);
                NewBlock->IsFree = true;
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

void* _CallocType(i64 Size)
{
    ASSERT(dynamAllocator.head);
    //ASSERT(Size <= (globalMemHandler->memRegions[DYNAMIC].Size - globalMemHandler->memRegions[DYNAMIC].UsedAmount), "Not enough memory left for dynmaic memory allocation!");

    void* MemBlockData = _MallocType(Size);

    if (MemBlockData)
    {
        Memory_Block* Block = ConvertDataToMemoryBlock(MemBlockData);
        ASSERT(Block->data);

        memset(MemBlockData, 0, Block->Size);
    };

    return MemBlockData;
};

void* _ReAlloc(void* DataToRealloc, i64 NewSize)
{
    ASSERT(dynamAllocator.head);
    //ASSERT((globalMemHandler->memRegions[DYNAMIC].Size - globalMemHandler->memRegions[DYNAMIC].UsedAmount) > NewSize, "Not enough room left in memory region!");

    Memory_Block* BlockToRealloc;
    if (DataToRealloc)
    {
        BlockToRealloc = ConvertDataToMemoryBlock(DataToRealloc);

        if (NewSize < BlockToRealloc->Size)
        {
            sizet SizeDiff = ReSizeAndMarkAsInUse(BlockToRealloc, NewSize);

            if (SizeDiff > sizeof(Memory_Block))
            {
                Memory_Block* NewBlock = SplitBlock(BlockToRealloc, SizeDiff);
                NewBlock->IsFree = true;
            }
            else
            {
                BlockToRealloc->Size += SizeDiff;
            };
        }
        else
        {
            void* newBlockData = _MallocType(NewSize);

            memcpy(newBlockData, BlockToRealloc->data, BlockToRealloc->Size);
            memset(BlockToRealloc->data, 0, BlockToRealloc->Size); //TODO: Remove if speed becomes an issue;

            BlockToRealloc->FreeBlockAndMergeIfNecessary();

            return newBlockData;
        };
    }
    else
    {
        DataToRealloc = _MallocType(NewSize);
    };

    return DataToRealloc;
};

void _DeAlloc(void** MemToFree)
{
    if (*MemToFree)
    {
        //ASSERT(*MemToFree > globalMemHandler->memRegions[DYNAMIC].BaseAddress && *MemToFree < globalMemHandler->memRegions[DYNAMIC].EndAddress, "Ptr to free not within dynmaic memory region!");

        Memory_Block* Block = ConvertDataToMemoryBlock(*MemToFree);

        Block->FreeBlockAndMergeIfNecessary();
        memset(*MemToFree, 0, Block->Size); //TODO: Remove if speed becomes an issue;

        *MemToFree = nullptr;
    };
};

#pragma once

#include "memory_handling.h"
#include <assert.h>
#include <cstring>

#define ASSERT(x) assert(x)

#define AllocSize(MemRegionIdentifier, Size) _AllocSize(MemRegionIdentifier, Size)
#define FreeSize(MemRegionIdentifier, Size) _FreeSize(MemRegionIdentifier, Size)

//TODO: Alignment
void* PointerAddition(void* baseAddress, ui64 amountToAdvancePointer)
{
    void* newAddress {};

    newAddress = ((ui8*)baseAddress) + amountToAdvancePointer;

    return newAddress;
};

static Application_Memory* appMemory {};

void InitApplicationMemory(Application_Memory* userDefinedAppMemoryStruct)
{
    appMemory = userDefinedAppMemoryStruct;
};

void InitApplicationMemory(Application_Memory* userDefinedAppMemoryStruct, ui64 sizeOfMemory, ui32 sizeOfPermanentStore, void* memoryStartAddress)
{
    appMemory = userDefinedAppMemoryStruct;

    ui32 sizeOfPermanentStorage = sizeOfPermanentStore;
    appMemory->SizeOfPermanentStorage = sizeOfPermanentStorage;
    appMemory->SizeOfTemporaryStorage = sizeOfMemory - (ui64)sizeOfPermanentStorage;
    appMemory->TotalSize = sizeOfMemory;
    appMemory->PermanentStorage = memoryStartAddress;
    appMemory->TemporaryStorage = ((ui8*)appMemory->PermanentStorage + appMemory->SizeOfPermanentStorage);
    appMemory->regionCount = 0;
};

Dynamic_Mem_Allocator InitDynamAllocator(i32 memRegionIdentifier);

i32 CreateRegionFromMemory(Application_Memory* appMemory, i64 size)
{
    appMemory->Initialized = true;

    Memory_Region* memRegion = &appMemory->regions[appMemory->regionCount];
    memRegion->BaseAddress = PointerAddition(appMemory->TemporaryStorage, appMemory->TemporaryStorageUsed);
    memRegion->EndAddress = PointerAddition(memRegion->BaseAddress, (size - 1));
    memRegion->Size = size;
    memRegion->UsedAmount = 0;
    memRegion->dynamAllocator = InitDynamAllocator(appMemory->regionCount);

    appMemory->TemporaryStorageUsed += size;

    return appMemory->regionCount++;
};

auto _AllocSize(i32 MemRegionIdentifier, i64 size) -> void*
{
    ASSERT((appMemory->regions[MemRegionIdentifier].UsedAmount + size) <= appMemory->regions[MemRegionIdentifier].Size);
    void* Result = PointerAddition(appMemory->regions[MemRegionIdentifier].BaseAddress, appMemory->regions[MemRegionIdentifier].UsedAmount);
    appMemory->regions[MemRegionIdentifier].UsedAmount += (size);

    return Result;
};

auto _FreeSize(i32 MemRegionIdentifier, i64 sizeToFree) -> void
{
    ASSERT(sizeToFree < appMemory->regions[MemRegionIdentifier].Size);
    ASSERT(sizeToFree < appMemory->regions[MemRegionIdentifier].UsedAmount);

    appMemory->regions[MemRegionIdentifier].UsedAmount -= sizeToFree;
};

/******************************************************* 
   Dynamic Allocator
********************************************************/

/*
    TODO: 
    1.) Maybe get rid of Blocks linked list data structure and use array instead?
    2.) Alignment?
*/

void* GetDataFromBlock(Memory_Block* Header)
{
    ASSERT(Header->Size != 0);

    void* BlockData = (void*)(Header + 1);

    return BlockData;
};

Memory_Block* ConvertDataToMemoryBlock(void* Ptr)
{
    Memory_Block* BlockHeader {};
    BlockHeader = (Memory_Block*)(((ui8*)Ptr) - (sizeof(Memory_Block)));
    BlockHeader->data = Ptr;

    return BlockHeader;
};

Dynamic_Mem_Allocator InitDynamAllocator(i32 memRegionIdentifier)
{
    Dynamic_Mem_Allocator dynamAllocator;

    dynamAllocator.AmountOfBlocks = 0;

    ui16 BlockSize = 8;
    ui16 TotalSize = sizeof(Memory_Block) + BlockSize;
    Memory_Block* InitialBlock = (Memory_Block*)AllocSize(memRegionIdentifier, TotalSize);

    InitialBlock->Size = BlockSize;
    InitialBlock->IsFree = false;
    InitialBlock->data = GetDataFromBlock(InitialBlock);
    InitialBlock->nextBlock = nullptr;
    InitialBlock->prevBlock = nullptr;

    dynamAllocator.head = InitialBlock;
    dynamAllocator.tail = InitialBlock;

    return dynamAllocator;
};

Memory_Block* SplitBlock(i32 memRegionIdentifier, Memory_Block* BlockToSplit, sizet SizeOfNewBlock)
{
    ASSERT(BlockToSplit->data);
    ASSERT(SizeOfNewBlock > sizeof(Memory_Block));

    Memory_Block* NewBlock = (Memory_Block*)((ui8*)(BlockToSplit) + (BlockToSplit->Size - 1) + (sizeof(Memory_Block)));

    NewBlock->Size = SizeOfNewBlock;
    NewBlock->data = GetDataFromBlock(NewBlock);
    NewBlock->prevBlock = BlockToSplit;
    NewBlock->nextBlock = BlockToSplit->nextBlock;
    BlockToSplit->nextBlock = NewBlock;

    ++appMemory->regions[memRegionIdentifier].dynamAllocator.AmountOfBlocks;

    return NewBlock;
};

sizet ReSizeAndMarkAsInUse(Memory_Block* BlockToResize, sizet NewSize)
{
    sizet SizeDiff = BlockToResize->Size - NewSize;
    BlockToResize->Size = NewSize;
    BlockToResize->IsFree = false;

    return SizeDiff;
};

Memory_Block* GetFirstFreeBlockOfSize(i32 memRegionIdentifier, i64 Size)
{
    ASSERT(appMemory->regions[memRegionIdentifier].dynamAllocator.head);

    Memory_Block* Result {};
    Memory_Block* MemBlock = appMemory->regions[memRegionIdentifier].dynamAllocator.head;

    //Not first or last block
    if (MemBlock->nextBlock != nullptr)
    {
        for (ui32 BlockIndex { 0 }; BlockIndex < appMemory->regions[memRegionIdentifier].dynamAllocator.AmountOfBlocks; ++BlockIndex)
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

void Memory_Block::FreeBlockAndMergeIfNecessary(i32 memRegionIdentifier) //TODO: split up this function? What to do with more than 1 param that needs modified?
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

                --appMemory->regions[memRegionIdentifier].dynamAllocator.AmountOfBlocks;
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

                --appMemory->regions[memRegionIdentifier].dynamAllocator.AmountOfBlocks;
            };
        };
    }
    else
    {
        FreeSize(memRegionIdentifier, this->Size);
        this->prevBlock->nextBlock = nullptr;
        appMemory->regions[memRegionIdentifier].dynamAllocator.tail = this->prevBlock;
        --appMemory->regions[memRegionIdentifier].dynamAllocator.AmountOfBlocks;
    };
};

Memory_Block* AppendNewBlockAndMarkInUse(i32 memRegionIdentifier, i64 Size)
{
    i64 TotalSize = sizeof(Memory_Block) + Size;
    Memory_Block* NewBlock = (Memory_Block*)AllocSize(memRegionIdentifier, TotalSize);

    NewBlock->Size = Size;
    NewBlock->IsFree = false;
    NewBlock->data = GetDataFromBlock(NewBlock);

    NewBlock->prevBlock = appMemory->regions[memRegionIdentifier].dynamAllocator.tail;
    NewBlock->nextBlock = nullptr;
    appMemory->regions[memRegionIdentifier].dynamAllocator.tail->nextBlock = NewBlock;
    appMemory->regions[memRegionIdentifier].dynamAllocator.tail = NewBlock;

    ++appMemory->regions[memRegionIdentifier].dynamAllocator.AmountOfBlocks;

    return NewBlock;
};

void* _MallocSize(i32 memRegionIdentifier, i64 Size)
{
    ASSERT(appMemory->regions[memRegionIdentifier].dynamAllocator.head);
    //ASSERT(Size <= (globalMemHandler->memRegions[DYNAMIC].Size - globalMemHandler->memRegions[DYNAMIC].UsedAmount), "Not enough memory left for dynmaic memory allocation!");

    void* Result { nullptr };

    if (Size > 0)
    {
        Memory_Block* MemBlock = GetFirstFreeBlockOfSize(memRegionIdentifier, Size);

        //No free blocks found
        if (!MemBlock)
        {
            MemBlock = AppendNewBlockAndMarkInUse(memRegionIdentifier, Size);

            Result = MemBlock->data;
            return Result;
        }
        else
        {
            sizet SizeDiff = ReSizeAndMarkAsInUse(MemBlock, Size);

            if (SizeDiff > sizeof(Memory_Block))
            {
                Memory_Block* NewBlock = SplitBlock(memRegionIdentifier, MemBlock, SizeDiff);
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

void* _CallocSize(i32 memRegionIdentifier, i64 Size)
{
    ASSERT(appMemory->regions[memRegionIdentifier].dynamAllocator.head);
    //ASSERT(Size <= (globalMemHandler->memRegions[DYNAMIC].Size - globalMemHandler->memRegions[DYNAMIC].UsedAmount), "Not enough memory left for dynmaic memory allocation!");

    void* MemBlockData = _MallocSize(memRegionIdentifier, Size);

    if (MemBlockData)
    {
        Memory_Block* Block = ConvertDataToMemoryBlock(MemBlockData);
        ASSERT(Block->data);

        memset(MemBlockData, 0, Block->Size);
    };

    return MemBlockData;
};

void* _ReAlloc(i32 memRegionIdentifier, void* DataToRealloc, i64 NewSize)
{
    ASSERT(appMemory->regions[memRegionIdentifier].dynamAllocator.head);
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
                Memory_Block* NewBlock = SplitBlock(memRegionIdentifier, BlockToRealloc, SizeDiff);
                NewBlock->IsFree = true;
            }
            else
            {
                BlockToRealloc->Size += SizeDiff;
            };
        }
        else
        {
            void* newBlockData = _MallocSize(memRegionIdentifier, NewSize);

            memcpy(newBlockData, BlockToRealloc->data, BlockToRealloc->Size);
            memset(BlockToRealloc->data, 0, BlockToRealloc->Size); //TODO: Remove if speed becomes an issue;

            BlockToRealloc->FreeBlockAndMergeIfNecessary(memRegionIdentifier);

            return newBlockData;
        };
    }
    else
    {
        DataToRealloc = _MallocSize(memRegionIdentifier, NewSize);
    };

    return DataToRealloc;
};

void _DeAlloc(i32 memRegionIdentifier, void** MemToFree)
{
    if (*MemToFree)
    {
        //ASSERT(*MemToFree > globalMemHandler->memRegions[DYNAMIC].BaseAddress && *MemToFree < globalMemHandler->memRegions[DYNAMIC].EndAddress, "Ptr to free not within dynmaic memory region!");

        Memory_Block* Block = ConvertDataToMemoryBlock(*MemToFree);

        Block->FreeBlockAndMergeIfNecessary(memRegionIdentifier);
        memset(*MemToFree, 0, Block->Size); //TODO: Remove if speed becomes an issue;

        *MemToFree = nullptr;
    };
};

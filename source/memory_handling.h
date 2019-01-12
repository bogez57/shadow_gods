#ifndef MEMHANDLING_INCLUDE_H
#define MEMHANDLING_INCLUDE_H

#include <stdint.h>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef i32 b32;
typedef bool b;

typedef uint8_t ui8;
typedef uint16_t ui16;
typedef uint32_t ui32;
typedef uint64_t ui64;

typedef size_t sizet;

typedef float f32;
typedef double f64;

struct _Memory_Block
{
    b IsFree { true };
    i64 Size { 0 };
    void* data { nullptr };
    _Memory_Block* nextBlock { nullptr };
    _Memory_Block* prevBlock { nullptr };
};

struct _Dynamic_Mem_Allocator
{
    _Memory_Block* head;
    _Memory_Block* tail;
    ui32 AmountOfBlocks;
};

struct Memory_Region
{
    void* BaseAddress;
    void* EndAddress;
    i64 UsedAmount;
    i64 Size;
    _Dynamic_Mem_Allocator dynamAllocator;
};

struct Application_Memory
{
    bool Initialized { false };

    void* PermanentStorage { nullptr };
    void* TemporaryStorage { nullptr };

    i32 SizeOfPermanentStorage {};
    i64 SizeOfTemporaryStorage {};

    i64 TemporaryStorageUsed {};
    i64 TotalSize {};
    Memory_Region regions[10];
    i32 regionCount {};
};

void InitApplicationMemory(Application_Memory* userDefinedAppMemoryStruct);
void InitApplicationMemory(Application_Memory* userDefinedAppMemoryStruct, ui64 sizeOfMemory, ui32 sizeOfPermanentStore, void* memoryStartAddress);

i32 CreateRegionFromMemory(Application_Memory* Memory, i64 size);

//Prototypes so I can call below macros
void* _MallocSize(i32, i64);
void* _CallocSize(i32, i64);
void* _ReAlloc(i32, void*, i64);
void _DeAlloc(i32, void**);

#define MallocType(MemRegionIdentifier, Type, Count) (Type*)_MallocSize(MemRegionIdentifier, ((sizeof(Type)) * (Count)))
#define MallocSize(MemRegionIdentifier, Size) _MallocSize(MemRegionIdentifier, (Size))
#define CallocType(MemRegionIdentifier, Type, Count) (Type*)_CallocSize(MemRegionIdentifier, ((sizeof(Type)) * (Count)))
#define CallocSize(MemRegionIdentifier, Type, Count) _CallocSize(MemRegionIdentifier, (Size))
#define ReAllocType(MemRegionIdentifier, Ptr, Type, Count) (Type*)_ReAlloc(MemRegionIdentifier, Ptr, sizeof(Type) * Count)
#define ReAllocSize(MemRegionIdentifier, Ptr, Size) _ReAlloc(MemRegionIdentifier, Ptr, Size)
#define DeAlloc(MemRegionIdentifier, PtrToMemory) _DeAlloc(MemRegionIdentifier, (void**)&PtrToMemory)

#endif

#ifdef MEMHANDLING_IMPL

#include <assert.h>
#include <cstring>

#define ASSERT(x) assert(x)

//TODO: Alignment
void* _PointerAddition(void* baseAddress, ui64 amountToAdvancePointer)
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

_Dynamic_Mem_Allocator InitDynamAllocator(i32 memRegionIdentifier);

i32 CreateRegionFromMemory(Application_Memory* appMemory, i64 size)
{
    ASSERT(size < appMemory->SizeOfTemporaryStorage);
    ASSERT((size + appMemory->TemporaryStorageUsed) < appMemory->SizeOfTemporaryStorage);

    appMemory->Initialized = true;

    Memory_Region* memRegion = &appMemory->regions[appMemory->regionCount];
    memRegion->BaseAddress = _PointerAddition(appMemory->TemporaryStorage, appMemory->TemporaryStorageUsed);
    memRegion->EndAddress = _PointerAddition(memRegion->BaseAddress, (size - 1));
    memRegion->Size = size;
    memRegion->UsedAmount = 0;
    memRegion->dynamAllocator = InitDynamAllocator(appMemory->regionCount);

    appMemory->TemporaryStorageUsed += size;

    return appMemory->regionCount++;
};

#define AllocSize(MemRegionIdentifier, Size) _AllocSize(MemRegionIdentifier, Size)
#define FreeSize(MemRegionIdentifier, Size) _FreeSize(MemRegionIdentifier, Size)

auto _AllocSize(i32 MemRegionIdentifier, i64 size) -> void*
{
    ASSERT((appMemory->regions[MemRegionIdentifier].UsedAmount + size) <= appMemory->regions[MemRegionIdentifier].Size);
    void* Result = _PointerAddition(appMemory->regions[MemRegionIdentifier].BaseAddress, appMemory->regions[MemRegionIdentifier].UsedAmount);
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

void* _GetDataFromBlock(_Memory_Block* Header)
{
    ASSERT(Header->Size != 0);

    void* BlockData = (void*)(Header + 1);

    return BlockData;
};

_Memory_Block* _ConvertDataToMemoryBlock(void* Ptr)
{
    _Memory_Block* BlockHeader {};
    BlockHeader = (_Memory_Block*)(((ui8*)Ptr) - (sizeof(_Memory_Block)));
    BlockHeader->data = Ptr;

    return BlockHeader;
};

_Dynamic_Mem_Allocator InitDynamAllocator(i32 memRegionIdentifier)
{
    _Dynamic_Mem_Allocator dynamAllocator;

    dynamAllocator.AmountOfBlocks = 0;

    ui16 BlockSize = 8;
    ui16 TotalSize = sizeof(_Memory_Block) + BlockSize;
    _Memory_Block* InitialBlock = (_Memory_Block*)AllocSize(memRegionIdentifier, TotalSize);

    InitialBlock->Size = BlockSize;
    InitialBlock->IsFree = false;
    InitialBlock->data = _GetDataFromBlock(InitialBlock);
    InitialBlock->nextBlock = nullptr;
    InitialBlock->prevBlock = nullptr;

    dynamAllocator.head = InitialBlock;
    dynamAllocator.tail = InitialBlock;

    return dynamAllocator;
};

_Memory_Block* _SplitBlock(i32 memRegionIdentifier, _Memory_Block* BlockToSplit, sizet SizeOfNewBlock)
{
    ASSERT(BlockToSplit->data);
    ASSERT(SizeOfNewBlock > sizeof(_Memory_Block));

    _Memory_Block* NewBlock = (_Memory_Block*)((ui8*)(BlockToSplit) + (BlockToSplit->Size - 1) + (sizeof(_Memory_Block)));

    NewBlock->Size = SizeOfNewBlock;
    NewBlock->data = _GetDataFromBlock(NewBlock);
    NewBlock->prevBlock = BlockToSplit;
    NewBlock->nextBlock = BlockToSplit->nextBlock;
    BlockToSplit->nextBlock = NewBlock;

    ++appMemory->regions[memRegionIdentifier].dynamAllocator.AmountOfBlocks;

    return NewBlock;
};

sizet _ReSizeAndMarkAsInUse(_Memory_Block* BlockToResize, sizet NewSize)
{
    sizet SizeDiff = BlockToResize->Size - NewSize;
    BlockToResize->Size = NewSize;
    BlockToResize->IsFree = false;

    return SizeDiff;
};

_Memory_Block* _GetFirstFreeBlockOfSize(i32 memRegionIdentifier, i64 Size)
{
    ASSERT(appMemory->regions[memRegionIdentifier].dynamAllocator.head);

    _Memory_Block* Result {};
    _Memory_Block* MemBlock = appMemory->regions[memRegionIdentifier].dynamAllocator.head;

    if (MemBlock != appMemory->regions[memRegionIdentifier].dynamAllocator.tail)
    {
        for (ui32 BlockIndex { 0 }; BlockIndex < appMemory->regions[memRegionIdentifier].dynamAllocator.AmountOfBlocks; ++BlockIndex)
        {
            if (MemBlock->IsFree && MemBlock->Size >= Size)
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

void _FreeBlockAndMergeIfNecessary(_Memory_Block* blockToFree, i32 memRegionIdentifier) //TODO: split up this function? What to do with more than 1 param that needs modified?
{
    blockToFree->IsFree = true;

    if (blockToFree != appMemory->regions[memRegionIdentifier].dynamAllocator.tail)
    {
        { //Merge block with next and/or previous blocks
            if (blockToFree->nextBlock->IsFree)
            {
                blockToFree->Size += blockToFree->nextBlock->Size;
                blockToFree->nextBlock->Size = 0;

                if (blockToFree->nextBlock->nextBlock)
                {
                    blockToFree->nextBlock = blockToFree->nextBlock->nextBlock;
                }
                else
                {
                    blockToFree->nextBlock = nullptr;
                };

                --appMemory->regions[memRegionIdentifier].dynamAllocator.AmountOfBlocks;
            };

            if (blockToFree->prevBlock->IsFree)
            {
                blockToFree->prevBlock->Size += blockToFree->Size;

                if (blockToFree->nextBlock)
                {
                    blockToFree->prevBlock->nextBlock = blockToFree->nextBlock;
                }
                else
                {
                    blockToFree->prevBlock->nextBlock = nullptr;
                };

                blockToFree->Size = 0;

                --appMemory->regions[memRegionIdentifier].dynamAllocator.AmountOfBlocks;
            };
        };
    }
    else
    {
        blockToFree->prevBlock->nextBlock = nullptr;
        i64 sizeToFree = sizeof(_Memory_Block) + blockToFree->Size;
        appMemory->regions[memRegionIdentifier].dynamAllocator.tail = blockToFree->prevBlock;
        --appMemory->regions[memRegionIdentifier].dynamAllocator.AmountOfBlocks;
        blockToFree->Size = 0;
        FreeSize(memRegionIdentifier, sizeToFree);
    };
};

_Memory_Block* _AppendNewBlockAndMarkInUse(i32 memRegionIdentifier, i64 Size)
{
    i64 TotalSize = sizeof(_Memory_Block) + Size;
    _Memory_Block* NewBlock = (_Memory_Block*)AllocSize(memRegionIdentifier, TotalSize);

    NewBlock->Size = Size;
    NewBlock->IsFree = false;
    NewBlock->data = _GetDataFromBlock(NewBlock);

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
        _Memory_Block* MemBlock = _GetFirstFreeBlockOfSize(memRegionIdentifier, Size);

        //No free blocks found
        if (!MemBlock)
        {
            MemBlock = _AppendNewBlockAndMarkInUse(memRegionIdentifier, Size);

            Result = MemBlock->data;
            return Result;
        }
        else
        {
            sizet SizeDiff = _ReSizeAndMarkAsInUse(MemBlock, Size);

            if (SizeDiff > sizeof(_Memory_Block))
            {
                _Memory_Block* NewBlock = _SplitBlock(memRegionIdentifier, MemBlock, SizeDiff);
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
        _Memory_Block* Block = _ConvertDataToMemoryBlock(MemBlockData);
        ASSERT(Block->data);

        memset(MemBlockData, 0, Block->Size);
    };

    return MemBlockData;
};

void* _ReAlloc(i32 memRegionIdentifier, void* DataToRealloc, i64 NewSize)
{
    ASSERT(appMemory->regions[memRegionIdentifier].dynamAllocator.head);
    //ASSERT((globalMemHandler->memRegions[DYNAMIC].Size - globalMemHandler->memRegions[DYNAMIC].UsedAmount) > NewSize, "Not enough room left in memory region!");

    _Memory_Block* BlockToRealloc;
    if (DataToRealloc)
    {
        BlockToRealloc = _ConvertDataToMemoryBlock(DataToRealloc);

        if (NewSize < BlockToRealloc->Size)
        {
            sizet SizeDiff = _ReSizeAndMarkAsInUse(BlockToRealloc, NewSize);

            if (SizeDiff > sizeof(_Memory_Block))
            {
                _Memory_Block* NewBlock = _SplitBlock(memRegionIdentifier, BlockToRealloc, SizeDiff);
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

            _FreeBlockAndMergeIfNecessary(BlockToRealloc, memRegionIdentifier);

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

        _Memory_Block* Block = _ConvertDataToMemoryBlock(*MemToFree);

        memset(*MemToFree, 0, Block->Size); //TODO: Remove if speed becomes an issue;
        *MemToFree = nullptr;

        _FreeBlockAndMergeIfNecessary(Block, memRegionIdentifier);
    };
};

#endif // MEMHANDLING_IMPL
#ifndef DYNAMIC_ALLOCATOR_H
#define DYNAMIC_ALLOCATOR_H

#include <assert.h>

#define ASSERT(x) assert(x)

void InitDynamAllocator(s32 memRegionIdentifier);

//Prototypes so I can call below macros
//void* _MallocSize(s32, s64);
//void* _CallocSize(s32, s64);
//void* _ReAlloc(s32, void*, s64);
//void _DeAlloc(s32, void**);
#define MallocType(MemRegionIdentifier, Type, Count) /*(Type*)_MallocSize(MemRegionIdentifier, ((sizeof(Type)) * (Count)))*/ (Type*)globalPlatformServices->Malloc(((sizeof(Type)) * (Count)))
#define MallocSize(MemRegionIdentifier, Size) /*_MallocSize(MemRegionIdentifier, (Size))*/ globalPlatformServices->Malloc(Size)
#define CallocType(MemRegionIdentifier, Type, Count) /*(Type*)_CallocSize(MemRegionIdentifier, ((sizeof(Type)) * (Count)))*/ (Type*)globalPlatformServices->Calloc(1, ((sizeof(Type)) * (Count)))
#define CallocSize(MemRegionIdentifier, Size) /*_CallocSize(MemRegionIdentifier, (Size))*/ globalPlatformServices->Calloc(1, (Size))
#define ReAllocType(MemRegionIdentifier, Ptr, Type, Count) /*(Type*)_ReAlloc(MemRegionIdentifier, Ptr, (sizeof(Type)) * (Count))*/ (Type*)globalPlatformServices->Realloc(Ptr, (sizeof(Type)) * (Count))
#define ReAllocSize(MemRegionIdentifier, Ptr, Size) /*_ReAlloc(MemRegionIdentifier, Ptr, Size)*/ globalPlatformServices->Realloc(Ptr, Size)
#define DeAlloc(MemRegionIdentifier, PtrToMemory) /*_DeAlloc(MemRegionIdentifier, (void**)&PtrToMemory)*/ globalPlatformServices->Free((void*)PtrToMemory)

#endif

#ifdef DYNAMIC_ALLOCATOR_IMPL

/*
    TODO:
    1.) Logic is pretty broken. Need to do proper testing before use
    2.) Maybe get rid of Blocks linked list data structure and use array instead?
    3.) Alignment?
    4.) delete the multiple dynamica allocators part, don't think I need that.
*/


#if 0
struct _Memory_Block
{
    bool IsFree { true };
    s64 Size {};
    void* data { nullptr };
    _Memory_Block* nextBlock { nullptr };
    _Memory_Block* prevBlock { nullptr };
};

struct _Dynamic_Allocator
{
    _Memory_Block* head;
    _Memory_Block* tail;
    s32 AmountOfBlocks;
};

_Dynamic_Allocator dynamAllocators[10] = {};

void* _GetDataFromBlock(_Memory_Block* Header)
{
    ASSERT(Header->Size != 0);
    
    void* BlockData = (void*)(Header + 1);
    
    return BlockData;
};

void InitDynamAllocator(s32 memRegionIdentifier)
{
    ASSERT(appMemory->partitions[memRegionIdentifier].allocatorType == DYNAMIC);
    
    dynamAllocators[memRegionIdentifier].AmountOfBlocks = 0;
    
    u16 BlockSize = 8;
    u16 TotalSize = sizeof(_Memory_Block) + BlockSize;
    _Memory_Block* InitialBlock = (_Memory_Block*)_AllocSize(memRegionIdentifier, TotalSize);
    
    InitialBlock->Size = BlockSize;
    InitialBlock->IsFree = false;
    InitialBlock->data = _GetDataFromBlock(InitialBlock);
    InitialBlock->nextBlock = nullptr;
    InitialBlock->prevBlock = nullptr;
    
    dynamAllocators[memRegionIdentifier].head = InitialBlock;
    dynamAllocators[memRegionIdentifier].tail = InitialBlock;
};

_Memory_Block* _ConvertDataToMemoryBlock(void* Ptr)
{
    _Memory_Block* BlockHeader {};
    BlockHeader = (_Memory_Block*)(((u8*)Ptr) - (sizeof(_Memory_Block)));
    
    return BlockHeader;
};

_Memory_Block* _SplitBlock(s32 memRegionIdentifier, _Memory_Block* BlockToSplit, sizet SizeOfNewBlock)
{
    ASSERT(BlockToSplit->data);
    ASSERT(SizeOfNewBlock > sizeof(_Memory_Block));
    
    _Memory_Block* NewBlock = (_Memory_Block*)((u8*)(BlockToSplit) + (BlockToSplit->Size) + (sizeof(_Memory_Block)));
    
    NewBlock->Size = SizeOfNewBlock - sizeof(_Memory_Block);
    NewBlock->data = _GetDataFromBlock(NewBlock);
    NewBlock->prevBlock = BlockToSplit;
    NewBlock->nextBlock = BlockToSplit->nextBlock;
    BlockToSplit->nextBlock->prevBlock = NewBlock;
    BlockToSplit->nextBlock = NewBlock;
    
    ++dynamAllocators[memRegionIdentifier].AmountOfBlocks;
    
    return NewBlock;
};

sizet _ReSizeAndMarkAsInUse(_Memory_Block* BlockToResize, sizet NewSize)
{
    sizet SizeDiff = BlockToResize->Size - NewSize;
    BlockToResize->Size = NewSize;
    BlockToResize->IsFree = false;
    
    return SizeDiff;
};

_Memory_Block* _GetFirstFreeBlockOfSize(s32 memRegionIdentifier, s64 Size)
{
    ASSERT(dynamAllocators[memRegionIdentifier].head);
    
    _Memory_Block* Result {};
    _Memory_Block* MemBlock = dynamAllocators[memRegionIdentifier].head;
    
    if (MemBlock != dynamAllocators[memRegionIdentifier].tail)
    {
        for (s32 BlockIndex { 0 }; BlockIndex < dynamAllocators[memRegionIdentifier].AmountOfBlocks; ++BlockIndex)
        {
            if (MemBlock->IsFree && MemBlock->Size >= Size)
            {
                Result = MemBlock;
                return Result;
            }
            else
            {
                if(!MemBlock->nextBlock)
                    return nullptr;
                MemBlock = MemBlock->nextBlock;
            }
        };
    };
    
    //No free blocks found
    return nullptr;
};

void _FreeBlockAndMergeIfNecessary(_Memory_Block* blockToFree, s32 memRegionIdentifier) //TODO: split up this function? What to do with more than 1 param that needs modified?
{
    blockToFree->IsFree = true;
    
    if (blockToFree != dynamAllocators[memRegionIdentifier].tail)
    {
        { //Merge block with next and/or previous blocks
            if (blockToFree->nextBlock->IsFree)
            {
                blockToFree->Size += blockToFree->nextBlock->Size;
                blockToFree->nextBlock->Size = 0;
                
                if (blockToFree->nextBlock != dynamAllocators[memRegionIdentifier].tail)
                {
                    blockToFree->nextBlock->nextBlock->prevBlock = blockToFree;
                    blockToFree->nextBlock = blockToFree->nextBlock->nextBlock;
                }
                else
                {
                    blockToFree->nextBlock = nullptr;
                    dynamAllocators[memRegionIdentifier].tail = blockToFree;
                };
                
                --dynamAllocators[memRegionIdentifier].AmountOfBlocks;
            };
            
            if (blockToFree->prevBlock->IsFree)
            {
                blockToFree->prevBlock->Size += blockToFree->Size;
                
                if (blockToFree != dynamAllocators[memRegionIdentifier].tail)
                {
                    blockToFree->prevBlock->nextBlock = blockToFree->nextBlock;
                    blockToFree->nextBlock->prevBlock = blockToFree->prevBlock;
                }
                else
                {
                    blockToFree->prevBlock->nextBlock = nullptr;
                    dynamAllocators[memRegionIdentifier].tail = blockToFree->prevBlock;
                };
                
                blockToFree->Size = 0;
                
                --dynamAllocators[memRegionIdentifier].AmountOfBlocks;
            };
        };
    }
    else
    {
        blockToFree->prevBlock->nextBlock = nullptr;
        s64 sizeToFree = sizeof(_Memory_Block) + blockToFree->Size;
        dynamAllocators[memRegionIdentifier].tail = blockToFree->prevBlock;
        --dynamAllocators[memRegionIdentifier].AmountOfBlocks;
        blockToFree->Size = 0;
        _FreeSize(memRegionIdentifier, sizeToFree);
    };
};

_Memory_Block* _AppendNewBlockAndMarkInUse(s32 memRegionIdentifier, s64 Size)
{
    s64 TotalSize = sizeof(_Memory_Block) + Size;
    _Memory_Block* NewBlock = (_Memory_Block*)_AllocSize(memRegionIdentifier, TotalSize);
    
    NewBlock->Size = Size;
    NewBlock->IsFree = false;
    NewBlock->data = _GetDataFromBlock(NewBlock);
    
    NewBlock->prevBlock = dynamAllocators[memRegionIdentifier].tail;
    NewBlock->nextBlock = nullptr;
    
    dynamAllocators[memRegionIdentifier].tail->nextBlock = NewBlock;
    dynamAllocators[memRegionIdentifier].tail = NewBlock;
    
    ++dynamAllocators[memRegionIdentifier].AmountOfBlocks;
    
    return NewBlock;
};

void* _MallocSize(s32 memRegionIdentifier, s64 Size)
{
    ASSERT(appMemory->partitions[memRegionIdentifier].allocatorType == DYNAMIC);
    ASSERT(dynamAllocators[memRegionIdentifier].head);//Is memory region identifier valid?
    ASSERT(Size <= (appMemory->partitions[memRegionIdentifier].Size - appMemory->partitions[memRegionIdentifier].UsedAmount)); //Not enough memory left for dynmaic memory allocation!
    
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

void* _CallocSize(s32 memRegionIdentifier, s64 Size)
{
    ASSERT(appMemory->partitions[memRegionIdentifier].allocatorType == DYNAMIC);
    ASSERT(dynamAllocators[memRegionIdentifier].head);//Is memory region identifier valid?
    ASSERT(Size <= (appMemory->partitions[memRegionIdentifier].Size - appMemory->partitions[memRegionIdentifier].UsedAmount));
    
    void* MemBlockData = _MallocSize(memRegionIdentifier, Size);
    
    if (MemBlockData)
    {
        _Memory_Block* Block = _ConvertDataToMemoryBlock(MemBlockData);
        ASSERT(Block->data);
        
        memset(MemBlockData, 0, Block->Size);
    };
    
    return MemBlockData;
};

void* _ReAlloc(s32 memRegionIdentifier, void* DataToRealloc, s64 NewSize)
{
    ASSERT(appMemory->partitions[memRegionIdentifier].allocatorType == DYNAMIC);
    ASSERT(dynamAllocators[memRegionIdentifier].head);//Is memory region identifier valid?
    ASSERT((appMemory->partitions[memRegionIdentifier].Size - appMemory->partitions[memRegionIdentifier].UsedAmount)); //Not enough room left in memory region!
    
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

void _DeAlloc(s32 memRegionIdentifier, void** MemToFree)
{
    ASSERT(appMemory->partitions[memRegionIdentifier].allocatorType == DYNAMIC);
    
    if (*MemToFree)
    {
        ASSERT(*MemToFree > appMemory->partitions[memRegionIdentifier].BaseAddress && *MemToFree < appMemory->partitions[memRegionIdentifier].EndAddress); //Ptr to free not within dynmaic memory region! Are you trying to free from correct region?
        
        _Memory_Block* Block = _ConvertDataToMemoryBlock(*MemToFree);
        
        memset(*MemToFree, 0, Block->Size); //TODO: Remove if speed becomes an issue;
        *MemToFree = nullptr;
        
        _FreeBlockAndMergeIfNecessary(Block, memRegionIdentifier);
    };
};

#endif //DYNAMIC_ALLOCATOR_IMPL
#endif
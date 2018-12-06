#pragma once

/*** Dynamic Allocator ***/

struct Memory_Block;

struct Dynamic_Mem_Allocator
{
    Memory_Block* head;
    Memory_Block* tail;
    ui32 AmountOfBlocks;
    i32 memoryRegionIdentifier;
};

struct Memory_Block
{
    b IsFree { true };
    i64 Size { 0 };
    void* data { nullptr };
    Memory_Block* nextBlock { nullptr };
    Memory_Block* prevBlock { nullptr };

    void FreeBlockAndMergeIfNecessary(OUT Dynamic_Mem_Allocator* DynamAllocator);
};

void InitDynamAllocator_1(Dynamic_Mem_Allocator* dynamAllocator);

#define MallocType(Type, Count) (Type*)_MallocType(&dynamicAllocator, ((sizeof(Type)) * (Count)))
#define MallocSize(Size) _MallocType(&dynamicAllocator, (Size))
#define CallocType(Type, Count) (Type*)_CallocType(&dynamicAllocator, ((sizeof(Type)) * (Count)))
#define CallocSize(Type, Count) _CallocType(&dynamicAllocator, (Size))
#define ReAlloc(Ptr, Type, Count) (Type*)_ReAlloc(&dynamicAllocator, Ptr, sizeof(Type) * Count)
#define DeAlloc(PtrToMemory) _DeAlloc(&dynamicAllocator, (i64**)&PtrToMemory)

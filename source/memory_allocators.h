#pragma once

/*** Dynamic Allocator ***/

struct Memory_Block;

struct Dynamic_Mem_Allocator
{
    Memory_Block* head;
    Memory_Block* tail;
    ui32 AmountOfBlocks;
};

struct Memory_Block
{
    b IsFree { true };
    sizet Size { 0 };
    void* data { nullptr };
    Memory_Block* nextBlock { nullptr };
    Memory_Block* prevBlock { nullptr };

    void FreeBlockAndMergeIfNecessary(OUT Dynamic_Mem_Allocator* DynamAllocator);
};

auto CreateAndInitDynamAllocator() -> Dynamic_Mem_Allocator;

#define MallocType(Type, Count) (Type*)_MallocType(&globalMemHandler->dynamAllocator, ((sizeof(Type)) * (Count)))
#define MallocSize(Size) _MallocType(&globalMemHandler->dynamicAllocator, (Size))
#define CallocType(Type, Count) (Type*)_CallocType(&globalMemHandler->dynamAllocator, ((sizeof(Type)) * (Count)))
#define CallocSize(Type, Count) _CallocType(&globalMemHandler->dynamAllocator, (Size))
#define ReAlloc(Ptr, Type, Count) (Type*)_ReAlloc(&globalMemHandler->dynamAllocator, Ptr, sizeof(Type) * Count)
#define DeAlloc(PtrToMemory) _DeAlloc(&globalMemHandler->dynamAllocator, (ui64**)&PtrToMemory)

/*** Linear Allocator ***/

struct Linear_Mem_Allocator
{
};

#define PushSize(Size) _PushType(&globalMemHandler->LinearAllocator, (Size))
#define PushType(Type, Count) (Type*)_PushType(&globalMemHandler->LinearAllocator, ((sizeof(Type)) * (Count)))
#define PopSize(Size) _PopSize(&globalMemHandler->LinearAllocator, (Size))

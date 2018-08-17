#pragma once

struct Memory_Block
{
    b IsFree{true};

    sizet Size{0};
    void* BaseAddress{nullptr};
    Memory_Block* nextBlock{nullptr};
    Memory_Block* prevBlock{nullptr};
};

struct Memory_Chunk
{
    ui64* BaseAddress{nullptr};
    ui64* EndAddress{nullptr};
    ui64 Size{0};
    ui64 UsedAmount{0};
    Memory_Block* MemBlocks[1000] = {};
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

//TODO: Right now this only works if nothing new was pushed onto the memory chunk between the previous allocation 
//and this allocation. If there was then this will override it
#define RePushType(MemoryChunk, Ptr, Type, Count) (Type*)_RePushType(MemoryChunk, Ptr, sizeof(Type), Count)
auto
_RePushType(Memory_Chunk* MemoryChunk, void* NewPtr, ui64 Size, sizet Count) -> void*
{
    BGZ_ASSERT(1 == 0)//TODO: Remove eventually - Only here so I know when this is called since I don't think it will work with spine yet
    BGZ_ASSERT((MemoryChunk->UsedAmount + Size) <= MemoryChunk->Size);
    BGZ_ASSERT(NewPtr > MemoryChunk->BaseAddress && NewPtr < MemoryChunk->EndAddress);

    void* Result{nullptr};
    return Result;
};

#define Free(MemoryChunk, VALUE) _Free(MemoryChunk, (void*)VALUE)
auto
_Free(Memory_Chunk* MemoryChunk, void* ptrToValue) -> void
{
};

auto
SpineFree() -> void
{
    //This function is used in spine's extension.h file. Since I'm not quite sure yet how I want to deallocate things from a memory 
    //chunk I'm just going to let spine allocate things and never free them for now. 
};

///////////////////////////////////////////

local_func auto
InitMemoryChunk(Memory_Chunk* MemoryChunk, sizet SizeToReserve, ui64* StartingAddress) -> void
{
    MemoryChunk->BaseAddress = StartingAddress;
    MemoryChunk->EndAddress = MemoryChunk->BaseAddress + SizeToReserve;
    MemoryChunk->Size = SizeToReserve; 
    MemoryChunk->UsedAmount = 0;    
};

#define MyMalloc(MemoryChunk, Size, Count) _MyMalloc(MemoryChunk, Size, Count)
auto 
_MyMalloc(Memory_Chunk* MemoryChunk, sizet Size, ui32 Count) -> void*
{
    void* Result{nullptr};

    if(!MemoryChunk->MemBlocks[0])
    {
        MemoryChunk->MemBlocks[0] = PushType(MemoryChunk, Memory_Block, 1);
        MemoryChunk->MemBlocks[0]->BaseAddress = PushSize(MemoryChunk, Size);
        MemoryChunk->MemBlocks[0]->Size = Size;
        MemoryChunk->MemBlocks[0]->IsFree = false;
        MemoryChunk->MemBlocks[0]->nextBlock = PushType(MemoryChunk, Memory_Block, 1);
        MemoryChunk->MemBlocks[0]->nextBlock->prevBlock = MemoryChunk->MemBlocks[0];
        MemoryChunk->MemBlocks[1] = MemoryChunk->MemBlocks[0]->nextBlock;
        MemoryChunk->MemBlocks[1]->IsFree = true;

        return Result = MemoryChunk->MemBlocks[0]->BaseAddress;
    }
    else
    {
        for (ui32 BlockIndex{1}; BlockIndex < ArrayCount(MemoryChunk->MemBlocks); ++BlockIndex)
        {
            if(MemoryChunk->MemBlocks[BlockIndex]->IsFree)
            {
                MemoryChunk->MemBlocks[BlockIndex]->BaseAddress = PushSize(MemoryChunk, Size);
                MemoryChunk->MemBlocks[BlockIndex]->Size = Size;
                MemoryChunk->MemBlocks[BlockIndex]->IsFree = false;

                if(!MemoryChunk->MemBlocks[BlockIndex]->nextBlock)
                {
                    MemoryChunk->MemBlocks[BlockIndex]->nextBlock = PushType(MemoryChunk, Memory_Block, 1);
                    MemoryChunk->MemBlocks[BlockIndex]->nextBlock->prevBlock = MemoryChunk->MemBlocks[0];
                    MemoryChunk->MemBlocks[BlockIndex + 1] = MemoryChunk->MemBlocks[BlockIndex]->nextBlock;
                    MemoryChunk->MemBlocks[BlockIndex + 1]->IsFree = true;
                };

                return Result = MemoryChunk->MemBlocks[BlockIndex]->BaseAddress;
            };
        };
    };

    return Result;
};

auto
MyDeAlloc(Memory_Chunk* MemoryChunk, void* PtrToFree) -> void
{
};
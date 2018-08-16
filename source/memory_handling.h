#define PushSize(MemoryChunk, Size) _PushSize(MemoryChunk, Size)
auto
_PushSize(Memory_Chunk* MemoryChunk, sizet Size)
{
    BGZ_ASSERT((MemoryChunk->UsedAmount + Size) <= MemoryChunk->Size);
    void* Result = MemoryChunk->UsedAddress + MemoryChunk->UsedAmount;
    MemoryChunk->UsedAmount += (Size);

    return Result;
};

#define PushType(MemoryChunk, Type, Count) (Type*)_PushType(MemoryChunk, sizeof(Type), Count)
auto
_PushType(Memory_Chunk* MemoryChunk, ui64 Size, sizet Count) -> void*
{
    BGZ_ASSERT((MemoryChunk->UsedAmount + Size) <= MemoryChunk->Size);
    void* Result = MemoryChunk->UsedAddress + MemoryChunk->UsedAmount;
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

    MemoryChunk->UsedAmount -= (MemoryChunk->UsedAddress - (ui64*)NewPtr);
    MemoryChunk->UsedAddress = (ui64*)NewPtr;
    Result = MemoryChunk->UsedAddress;
    MemoryChunk->UsedAmount += (Size + Count);

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


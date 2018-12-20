#pragma once

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

struct Memory_Block
{
    b IsFree { true };
    i64 Size { 0 };
    void* data { nullptr };
    Memory_Block* nextBlock { nullptr };
    Memory_Block* prevBlock { nullptr };

    void FreeBlockAndMergeIfNecessary(i32 memRegionIdentifier);
};

struct Dynamic_Mem_Allocator
{
    Memory_Block* head;
    Memory_Block* tail;
    ui32 AmountOfBlocks;
};

struct Memory_Region
{
    void* BaseAddress;
    void* EndAddress;
    i64 UsedAmount;
    i64 Size;
    Dynamic_Mem_Allocator dynamAllocator;
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

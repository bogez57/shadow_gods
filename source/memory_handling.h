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

void* _MallocType(i32, i64);
void* _CallocType(i32, i64);
void* _ReAlloc(i32, void*, i64);
void _DeAlloc(i32, void**);

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
    bool IsInitialized { false };

    void* PermanentStorage { nullptr };
    void* TemporaryStorage { nullptr };

    ui32 SizeOfPermanentStorage {};
    ui64 SizeOfTemporaryStorage {};

    ui64 TemporaryStorageUsed {};
    ui64 TotalSize {};
    Memory_Region regions[10];
    i32 regionCount {};
};

void SupplyMemoryStructAddress(Application_Memory* userDefinedAppMemoryStruct);
void InitApplicationMemory(ui64 sizeOfMemory, ui32 sizeOfPermanentStore, void* memoryStartAddress);
i32 CreateRegionFromMemory(Application_Memory* Memory, i64 size);

#define MallocType(MemRegionIdentifier, Type, Count) (Type*)_MallocType(MemRegionIdentifier, ((sizeof(Type)) * (Count)))
#define MallocSize(MemRegionIdentifier, Size) _MallocType(MemRegionIdentifier, (Size))
#define CallocType(MemRegionIdentifier, Type, Count) (Type*)_CallocType(MemRegionIdentifier, ((sizeof(Type)) * (Count)))
#define CallocSize(MemRegionIdentifier, Type, Count) _CallocType(MemRegionIdentifier, (Size))
#define ReAlloc(MemRegionIdentifier, Ptr, Type, Count) (Type*)_ReAlloc(MemRegionIdentifier, Ptr, sizeof(Type) * Count)
#define DeAlloc(MemRegionIdentifier, PtrToMemory) _DeAlloc(MemRegionIdentifier, (void**)&PtrToMemory)

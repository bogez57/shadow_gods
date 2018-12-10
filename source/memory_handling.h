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

struct Application_Memory
{
    bool IsInitialized { false };

    void* PermanentStorage { nullptr };
    void* TemporaryStorage { nullptr };

    ui32 SizeOfPermanentStorage {};
    ui64 SizeOfTemporaryStorage {};

    ui64 TemporaryStorageUsed {};
    ui64 TotalSize {};
};

void* _MallocType(i64);
void* _CallocType(i64);
void* _ReAlloc(void*, i64);
void _DeAlloc(void**);

void InitApplicationMemory(Application_Memory* appMemory, ui64 sizeOfMemory, void* memoryStartAddress);
void CreateRegionFromMemory(Application_Memory* Memory, i64 size);

#define MallocType(Type, Count) (Type*)_MallocType(((sizeof(Type)) * (Count)))
#define MallocSize(Size) _MallocType((Size))
#define CallocType(Type, Count) (Type*)_CallocType(((sizeof(Type)) * (Count)))
#define CallocSize(Type, Count) _CallocType((Size))
#define ReAlloc(Ptr, Type, Count) (Type*)_ReAlloc(Ptr, sizeof(Type) * Count)
#define DeAlloc(PtrToMemory) _DeAlloc((void**)&PtrToMemory)

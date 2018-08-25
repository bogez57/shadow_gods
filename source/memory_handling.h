#pragma once
#include "list.h"

struct Memory_Header
{
    b IsFree{true};
    sizet Size{0};
    Memory_Header* nextBlock{nullptr};
    Memory_Header* prevBlock{nullptr};
};

struct Memory_Chunk
{
    ui64* BaseAddress{nullptr};
    ui64* EndAddress{nullptr};
    ui32 Size{0};
    ui64 UsedAmount{0};
    List* FreeBlocks{};
    List* FilledBlocks{};
};

#define MyMalloc(Size, Count) _MyMalloc(&GlobalGameState->DynamicMem, Size * Count)
#define MyReAlloc(Ptr, Type, Count) (Type*)_MyReAlloc(&GlobalGameState->DynamicMem, Ptr, sizeof(Type) * Count)
#define MyDeAlloc(PtrToMemory) _MyDeAlloc(&GlobalGameState->DynamicMem, (ui64**)PtrToMemory)


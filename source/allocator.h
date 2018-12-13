#pragma once

#include "atomic_types.h"

class Allocator
{
public:
    Allocator() {};

    Allocator(const Allocator&) {};
    ~Allocator() {};

    Allocator& operator=(const Allocator&) = delete;

    virtual void* Allocate(ui64 size) = 0;
    virtual void* ReAllocate(void* ptr, ui64 size) = 0;
    virtual void DeAllocate(void* ptr) = 0;

    int memRegionID;
};

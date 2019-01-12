#pragma once

class Allocator
{
public:
    Allocator() {};

    Allocator(const Allocator&) {};
    ~Allocator() {};

    Allocator& operator=(const Allocator&) = delete;

    virtual void* Allocate(i64 size) = 0;
    virtual void* ReAllocate(void* ptr, i64 size) = 0;
    virtual void DeAllocate(void* ptr) = 0;

    int memRegionID;
};

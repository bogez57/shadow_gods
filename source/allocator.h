#include "atomic_types.h"

class Allocator
{
public:
    Allocator() = {};
    Allocator(const Allocator&) = {};
    ~Allocator() = {};

    Allocator& operator=() = delete;

    virtual void* Allocate(ui64 size) = 0;
    virtual void* ReAllocate(ui64 size) = 0;
    virtual void DeAllocate(void* ptr) = 0;
}
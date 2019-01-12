#include "allocator.h"

class Dynamic_Allocator : public Allocator
{
public:
    Dynamic_Allocator(i32 memRegionID)
    {
        Allocator::memRegionID = memRegionID;
    };

    ~Dynamic_Allocator() = default;

    Dynamic_Allocator(const Dynamic_Allocator&) = delete;
    void operator=(const Dynamic_Allocator&) = delete;

    void* Allocate(i64 size) override
    {
        return MallocSize(Allocator::memRegionID, size);
    };

    void* ReAllocate(void* ptr, i64 size) override
    {
        return ReAllocSize(Allocator::memRegionID, ptr, size);
    };

    void DeAllocate(void* ptr) override
    {
        DeAlloc(Allocator::memRegionID, ptr);
    };
};
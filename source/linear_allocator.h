#include "allocator.h"

//TODO: Need to do an alloc of memory up front and then do some pointer manipulation within the allocator
class Linear_Allocator : public Allocator
{
public:
    void Init();
    ~Linear_Allocator() = default;

    Linear_Allocator(const Linear_Allocator&) = delete;
    void operator=(const Linear_Allocator&) = delete;

    void Init(i64 totalPossibleSizeOfAllocator)
    {
        MallocSize(Allocator::memRegionID, totalPossibleSizeOfAllocator);
    };

    void* Allocate(i64 size) override
    {
    };

    void DeAllocate(void* ptr) override
    {
    };
};
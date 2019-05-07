#include "allocator.h"
#include "boagz/error_handling.h"

//TODO: Need to do an alloc of memory up front and then do some pointer manipulation within the allocator
class Linear_Allocator : public Allocator
{
public:
    Linear_Allocator() = default;
    ~Linear_Allocator() = default;
    Linear_Allocator(const Linear_Allocator&) = delete;
    void operator=(const Linear_Allocator&) = delete;

    void Init(i64 totalPossibleSizeOfAllocator)
    {
        totalPossibleSizeOfAllocator -= 48;
        this->baseAddress = MallocSize(Allocator::memRegionID, totalPossibleSizeOfAllocator);
        this->size = totalPossibleSizeOfAllocator;
        this->usedAmount = 0;
    };

    void* Allocate(i64 size) override
    {
        BGZ_ASSERT((this->usedAmount + size) <= this->size, "Not enough space in linear allocator to allocate requested size");

        void* memoryPointer = ((ui8*)this->baseAddress) + this->usedAmount;
        this->usedAmount += (size);

        return memoryPointer;
    };

    void* ReAllocate(void*, i64 ) override
    {
        InvalidCodePath;

        void* incorrect{};
        return incorrect;
    };

    void DeAllocate(void** ptr) override
    {
        this->usedAmount = 0;
    };

private:
    void* baseAddress;
    i64 usedAmount;
    i64 size;
};
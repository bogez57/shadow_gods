#include "allocator.h"
#include "memory_handling.h"

class Linear_Allocator : public Allocator
{
public:
    Linear_Allocator() = default;
    ~Linear_Allocator() = default;

    Linear_Allocator(const Linear_Allocator&) = delete;

    void operator=(const Linear_Allocator&) = delete;

    void* Allocate(ui64 size) override {};

    void* ReAllocate(ui64 size) override {};

    void DeAllocate(void* ptr) override {

    };
}
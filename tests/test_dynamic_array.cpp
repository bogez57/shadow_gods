#include "catch.hpp"

#include "source/allocator.h"
#include "source/dynamic_array.h"

class Dynamic_Allocator : public Allocator
{
public:
    Dynamic_Allocator() = default;
    ~Dynamic_Allocator() = default;

    Dynamic_Allocator(const Dynamic_Allocator&) = delete;
    void operator=(const Dynamic_Allocator&) = delete;

    void* Allocate(i64 size) override
    {
        return malloc((sizet)size);
    };

    void* ReAllocate(void* ptr, i64 size) override
    {
        return realloc(ptr, (sizet)size);
    };

    void DeAllocate(void* ptr) override
    {
        free(ptr);
    };
};

SCENARIO("Nothing")
{
    GIVEN("WE HAVE BALLS")
    {
        WHEN("We contruct a dynamic array with a certain amount of elements and an allocator")
        {
            Dynamic_Allocator allocator;

            Dynam_Array<i64> myArray { 100, &allocator };

            REQUIRE(myArray.size == 0);
            REQUIRE(myArray.capacity == 100);
        }
    }
}
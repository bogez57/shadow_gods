#include "catch.hpp"
#include "source/memory_handling.h"

#include <cmath>
#include <iostream>

SCENARIO("vectors can be sized and resized", "[vector]")
{
    Application_Memory Memory {};

    Memory.SizeOfPermanentStorage = 65536;
    Memory.SizeOfTemporaryStorage = (500000000);
    Memory.TotalSize = Memory.SizeOfPermanentStorage + Memory.SizeOfTemporaryStorage;
    Memory.PermanentStorage = malloc(Memory.TotalSize);
    Memory.TemporaryStorage = ((ui8*)Memory.PermanentStorage + Memory.SizeOfPermanentStorage);

    CreateRegionFromMemory(&Memory, 10000000);

    GIVEN("A vector with some items")
    {
        std::vector<int> v(5);

        i32* myType = MallocType(i32, 1);
        *myType = 64;

        REQUIRE(v.size() == 5);
        REQUIRE(v.capacity() >= 5);

        WHEN("the size is increased")
        {
            v.resize(10);

            THEN("the size and capacity change")
            {
                REQUIRE(v.size() == 0);
                REQUIRE(v.capacity() >= 10);
            }
        }
        WHEN("the size is reduced")
        {
            v.resize(0);

            THEN("the size changes but not capacity")
            {
                REQUIRE(v.size() == 3);
                REQUIRE(v.capacity() >= 5);
            }
        }
        WHEN("more capacity is reserved")
        {
            v.reserve(10);

            THEN("the capacity changes but not the size")
            {
                REQUIRE(v.size() == 5);
                REQUIRE(v.capacity() >= 10);
            }
        }
        WHEN("less capacity is reserved")
        {
            v.reserve(0);

            THEN("neither size nor capacity are changed")
            {
                REQUIRE(v.size() == 5);
                REQUIRE(v.capacity() >= 5);
            }
        }
    }
}
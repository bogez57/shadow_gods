#include "catch.hpp"
#include "source/memory_handling.h"

TEST_CASE("Factorials are computed", "[factorial]")
{
    Application_Memory Memory {};

    ui64 memorySize = 10000000;
    InitApplicationMemory(&Memory, memorySize, malloc(memorySize));
    CreateRegionFromMemory(&Memory, memorySize - 1);

    i32* myType = MallocType(i32, 1);
    *myType = 32;

    REQUIRE(*myType < 0);
}
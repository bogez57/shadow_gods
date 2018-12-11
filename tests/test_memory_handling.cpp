#include "catch.hpp"
#include "source/memory_handling.h"

SCENARIO("Memory regions can be created")
{
    GIVEN("We call OS to allocate memory and initialize our memory struct")
    {
        Application_Memory appMemory;

        SupplyMemoryStructAddress(&appMemory);

        ui64 memorySize = 10000000;
        InitApplicationMemory(memorySize, 100000, malloc(memorySize));

        REQUIRE(appMemory.TotalSize == memorySize);

        WHEN("we create our memory region")
        {
        }
    }
}
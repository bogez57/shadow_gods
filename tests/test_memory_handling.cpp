#include "catch.hpp"
#include "source/memory_handling.h"

SCENARIO("Memory regions can be created")
{
    GIVEN("We call OS to allocate memory and initialize our memory struct")
    {
        Application_Memory appMemory;
        ui64 memorySize = 10000000;
        InitApplicationMemory(&appMemory, memorySize, malloc(memorySize));

        REQUIRE(appMemory.TotalSize == 10000000);
        REQUIRE(appMemory.PermanentStorage);

        WHEN("we create our memory region")
        {
            CreateRegionFromMemory(&appMemory, memorySize);
            REQUIRE(appMemory.IsInitialized == true);
        }
    }
}
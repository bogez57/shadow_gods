#include "catch.hpp"
#include "source/memory_handling.h"

TEST_CASE("Factorials are computed", "[factorial]")
{
    Application_Memory Memory {};

    Memory.SizeOfPermanentStorage = 65536;
    Memory.SizeOfTemporaryStorage = (500000000);
    Memory.TotalSize = Memory.SizeOfPermanentStorage + Memory.SizeOfTemporaryStorage;
    Memory.PermanentStorage = malloc(Memory.TotalSize);
    Memory.TemporaryStorage = ((ui8*)Memory.PermanentStorage + Memory.SizeOfPermanentStorage);

    CreateRegionFromMemory(&Memory, 10000000);

    InitDynamAllocator(3000000);

    i32* myType = MallocType(i32, 1);
    *myType = 32;

    REQUIRE(*myType < 0);
}
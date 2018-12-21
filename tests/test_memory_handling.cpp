#include "catch.hpp"
#include "source/atomic_types.h"
#include "source/memory_handling.h"

SCENARIO("Memory regions can be created")
{
    GIVEN("We call OS to allocate memory and initialize our application memory struct")
    {
        Application_Memory appMemory;

        ui64 memorySize = Megabytes(100);
        i32 permanentMemorySize = Megabytes(5);
        InitApplicationMemory(&appMemory, memorySize, permanentMemorySize, malloc(memorySize));

        REQUIRE(appMemory.TotalSize == memorySize);
        REQUIRE(appMemory.PermanentStorage);
        REQUIRE(appMemory.TemporaryStorage);
        REQUIRE(appMemory.TemporaryStorageUsed == 0);

        WHEN("we create one memory region from the allocated memory block")
        {
            i32 Region1ID = CreateRegionFromMemory(&appMemory, Megabytes(7));

            REQUIRE(appMemory.Initialized);
            REQUIRE(appMemory.regions[Region1ID].BaseAddress);

            THEN("We can allocate memory onto region")
            {
                i32* ptr = MallocType(Region1ID, i32, 1);

                REQUIRE(ptr);

                THEN("We can suplly a value to the handle")
                {
                    *ptr = 32;

                    REQUIRE(*ptr == 32);
                }

                THEN("We can deallocate that memory which sets handle to null")
                {
                    DeAlloc(Region1ID, ptr);

                    REQUIRE(ptr == nullptr);
                }

                THEN("We deallocating and reallocating 1st element onto my heap, we will reallocate onto the same memory address")
                {
                    ui64 originalPtrAddress = (ui64)ptr;

                    DeAlloc(Region1ID, ptr);
                    i32* newPtr = MallocType(Region1ID, i32, 1);

                    ui64 newPtrAddress = (ui64)newPtr;

                    REQUIRE(originalPtrAddress == newPtrAddress);
                }
            }
        }

        WHEN("We try and create multiple memory regions from that memory block")
        {
            i32 Region1ID = CreateRegionFromMemory(&appMemory, Megabytes(10));
            i32 Region2ID = CreateRegionFromMemory(&appMemory, Megabytes(10));
            i32 Region3ID = CreateRegionFromMemory(&appMemory, Megabytes(10));

            REQUIRE(Region1ID == 0);
            REQUIRE(Region2ID == 1);
            REQUIRE(Region3ID == 2);
            REQUIRE(appMemory.regionCount == 3);

            THEN("we have multiple valid memory regions within the memory block")
            {
                i32* region1Allocation = MallocType(Region1ID, i32, 1);
                i32* region2Allocation = MallocType(Region2ID, i32, 1);
                i32* region3Allocation = MallocType(Region3ID, i32, 1);

                REQUIRE(region1Allocation);
                REQUIRE(region2Allocation);
                REQUIRE(region3Allocation);
            }

            THEN("We have multiple, separate memory regions with correct address ranges for which to allocate on")
            {
                i64 region1Range = (i64)appMemory.regions[Region1ID].EndAddress - (i64)appMemory.regions[Region1ID].BaseAddress;
                i64 region2Range = (i64)appMemory.regions[Region2ID].EndAddress - (i64)appMemory.regions[Region2ID].BaseAddress;
                i64 region3Range = (i64)appMemory.regions[Region3ID].EndAddress - (i64)appMemory.regions[Region3ID].BaseAddress;

                REQUIRE(region1Range == (Megabytes(10) - 1)); //Subtract 1 since addresses start from a 0 index
                REQUIRE(region2Range == (Megabytes(10) - 1));
                REQUIRE(region3Range == (Megabytes(10) - 1));
            }

            THEN("We have multiple, separate memory regions for which to allocate on")
            {
                i32* region1Allocation = MallocType(Region1ID, i32, 1);
                i32* region2Allocation = MallocType(Region2ID, i32, 1);
                i32* region3Allocation = MallocType(Region3ID, i32, 1);

                i64 region1Address = (i64)region1Allocation;
                i64 region2Address = (i64)region2Allocation;
                i64 region3Address = (i64)region3Allocation;

                REQUIRE(region1Address != region2Address);
                REQUIRE(region1Address != region3Address);
                REQUIRE(region2Address != region3Address);
            }
        }
    }
}
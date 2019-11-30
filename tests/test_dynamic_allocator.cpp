#include "atomic_types.h"
#include "catch.hpp"
#include "memory_handling.h"
#include "dynamic_allocator.h"

SCENARIO("Memory Paritions can be created")
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

        WHEN("we create one memory parition from the allocated memory block")
        {
            i32 partition1_ID = CreatePartitionFromMemoryBlock(&appMemory, Megabytes(7), DYNAMIC);
            InitDynamAllocator(partition1_ID);

            REQUIRE(appMemory.Initialized);
            REQUIRE(appMemory.partitions[partition1_ID].BaseAddress);

            THEN("We can allocate memory onto region")
            {
                i32* ptr = MallocType(partition1_ID, i32, 1);

                REQUIRE(ptr);

                THEN("We can suplly a value to the handle")
                {
                    *ptr = 32;

                    REQUIRE(*ptr == 32);
                }

                THEN("We can deallocate that memory which sets handle to null")
                {
                    DeAlloc(partition1_ID, ptr);

                    REQUIRE(ptr == nullptr);
                }

                THEN("We deallocating and reallocating 1st element onto my heap, we will reallocate onto the same memory address")
                {
                    ui64 originalPtrAddress = (ui64)ptr;

                    DeAlloc(partition1_ID, ptr);
                    i32* newPtr = MallocType(partition1_ID, i32, 1);

                    ui64 newPtrAddress = (ui64)newPtr;

                    REQUIRE(originalPtrAddress == newPtrAddress);
                }
            }
        }

        WHEN("We try and create multiple memory partitions from that memory block")
        {
            i32 partition1_ID = CreatePartitionFromMemoryBlock(&appMemory, Megabytes(10), DYNAMIC);
            i32 partition2_ID = CreatePartitionFromMemoryBlock(&appMemory, Megabytes(10), DYNAMIC);
            i32 partition3_ID = CreatePartitionFromMemoryBlock(&appMemory, Megabytes(10), DYNAMIC);
            InitDynamAllocator(partition1_ID);
            InitDynamAllocator(partition2_ID);
            InitDynamAllocator(partition3_ID);

            REQUIRE(partition1_ID == 0);
            REQUIRE(partition2_ID == 1);
            REQUIRE(partition3_ID == 2);
            REQUIRE(appMemory.partitionCount == 3);

            THEN("we have multiple valid memory partitions within the memory block")
            {
                i32* partition1Allocation = MallocType(partition1_ID, i32, 1);
                i32* partition2Allocation = MallocType(partition2_ID, i32, 1);
                i32* partition3Allocation = MallocType(partition3_ID, i32, 1);

                REQUIRE(partition1Allocation);
                REQUIRE(partition2Allocation);
                REQUIRE(partition3Allocation);
            }

            THEN("We have multiple, separate memory paritions with correct address ranges for which to allocate on")
            {
                i64 region1Range = (i64)appMemory.partitions[partition1_ID].EndAddress - (i64)appMemory.partitions[partition1_ID].BaseAddress;
                i64 region2Range = (i64)appMemory.partitions[partition2_ID].EndAddress - (i64)appMemory.partitions[partition2_ID].BaseAddress;
                i64 region3Range = (i64)appMemory.partitions[partition3_ID].EndAddress - (i64)appMemory.partitions[partition3_ID].BaseAddress;

                REQUIRE(region1Range == (Megabytes(10) - 1)); //Subtract 1 since addresses start from a 0 index
                REQUIRE(region2Range == (Megabytes(10) - 1));
                REQUIRE(region3Range == (Megabytes(10) - 1));
            }

            THEN("We have multiple, separate memory partitions for which to allocate on")
            {
                i32* partition1Allocation = MallocType(partition1_ID, i32, 1);
                i32* partition2Allocation = MallocType(partition2_ID, i32, 1);
                i32* partition3Allocation = MallocType(partition3_ID, i32, 1);

                i64 partition1Address = (i64)partition1Allocation;
                i64 partition2Address = (i64)partition2Allocation;
                i64 partition3Address = (i64)partition3Allocation;

                REQUIRE(partition1Address != partition2Address);
                REQUIRE(partition1Address != partition3Address);
                REQUIRE(partition2Address != partition3Address);
            }
        }
    }
}
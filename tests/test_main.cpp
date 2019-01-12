#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"

#define ATOMIC_TYPES_IMPL
#include "source/atomic_types.h"
#define MEMHANDLING_IMPL
#include "source/memory_handling.h"
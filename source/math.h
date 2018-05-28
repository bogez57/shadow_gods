#pragma once

#include "types.h"
#include <math.h>//TODO: Remove and replace with own, faster platform specific implementations

inline float32
AbsoluteVal(float32 Value)
{
    float32 Result = (float32)fabs(Value);
    return Result;
}


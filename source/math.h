#pragma once

#include "types.h"
#include <math.h>//TODO: Remove and replace with own, faster platform specific implementations

inline float32
AbsoluteVal(float32 Value)
{
    float32 Result = (float32)fabs(Value);
    return Result;
}

inline float32
Sin(float Angle)
{
    float32 RealNumber = sinf(Angle);
    return RealNumber;
};

inline float32
Cos(float Angle)
{
    float32 RealNumber = cosf(Angle);
    return RealNumber;
};

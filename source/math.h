#pragma once

#include "types.h"
#include <math.h>//TODO: Remove and replace with own, faster platform specific implementations

#define PI 3.141592f

inline float32
AbsoluteVal(float32 Value)
{
    float32 Result = (float32)fabs(Value);
    return Result;
}

inline float32
Sin(float AngleInRadians)
{
    float32 RealNumber = sinf(AngleInRadians);
    return RealNumber;
};

inline float32
Cos(float AngleInRadians)
{
    float32 RealNumber = cosf(AngleInRadians);
    return RealNumber;
};

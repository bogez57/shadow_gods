#pragma once

#include "types.h"
#include <math.h>//TODO: Remove and replace with own, faster platform specific implementations

#define PI 3.141592f

inline f32
AbsoluteVal(f32 Value)
{
    f32 Result = (f32)fabs(Value);
    return Result;
}

inline f32
Sin(float AngleInRadians)
{
    f32 RealNumber = sinf(AngleInRadians);
    return RealNumber;
};

inline f32
Cos(float AngleInRadians)
{
    f32 RealNumber = cosf(AngleInRadians);
    return RealNumber;
};

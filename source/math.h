#pragma once

#include "atomic_types.h"
#include <math.h> //TODO: Remove and replace with own, faster platform specific implementations

#define PI 3.1415926535897932385f

inline f32
AbsoluteVal(f32 Value)
{
    f32 Result = (f32)fabs(Value);
    return Result;
}

inline v2f
AbsoluteVal(v2f Value)
{
    v2f Result { (f32)fabs(Value.x), (f32)fabs(Value.y) };
    return Result;
}

inline f32
Sin(f32 AngleInRadians)
{
    f32 RealNumber = sinf(AngleInRadians);
    return RealNumber;
};

inline f32
Cos(f32 AngleInRadians)
{
    f32 RealNumber = cosf(AngleInRadians);
    return RealNumber;
};

inline f64
Sqrt(f64 Number)
{
    Number = sqrt(Number);
    return Number;
};

inline sizet
RoundUp(sizet NumToRound, sizet Multiple)
{
    if (Multiple == 0)
        return NumToRound;

    sizet Remainder = NumToRound % Multiple;
    if (Remainder == 0)
        return NumToRound;

    return NumToRound + Multiple - Remainder;
};

inline sizet
RoundDown(sizet NumToRound, sizet Multiple)
{
    if (Multiple == 0)
        return NumToRound;

    sizet Remainder = NumToRound % Multiple;
    if (Remainder == 0)
        return NumToRound;

    return NumToRound - Remainder;
};
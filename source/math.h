#pragma once

#include "atomic_types.h"
#include <math.h>//TODO: Remove and replace with own, faster platform specific implementations

#define PI 3.1415926535897932385f

inline f32
AbsoluteVal(f32 Value)
{
    f32 Result = (f32)fabs(Value);
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

inline ui32
RoundUp(ui32 NumToRound, ui32 Multiple)
{
    if (Multiple == 0)
        return NumToRound;

    ui32 Remainder = NumToRound % Multiple; 
    if(Remainder == 0)
        return NumToRound;
    
    return NumToRound + Multiple - Remainder;
};

inline ui32
RoundDown(ui32 NumToRound, ui32 Multiple)
{
    if (Multiple == 0)
        return NumToRound;

    ui32 Remainder = NumToRound % Multiple; 
    if(Remainder == 0)
        return NumToRound;
    
    return NumToRound - Remainder;
};
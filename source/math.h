#pragma once

#include <math.h> //TODO: Remove and replace with own, faster platform specific implementations

#define PI 3.1415926535897932385f

inline i32
AbsoluteVal(i32 Value)
{
    i32 Result = abs(Value);
    return Result;
}

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
Radians(f32 angleInDegrees)
{
    f32 angleInRadians = angleInDegrees * (PI / 180.0f);

    return angleInRadians;
};

inline f32
SinInRadians(f32 AngleInRadians)
{
    f32 RealNumber = sinf(AngleInRadians);
    return RealNumber;
};

inline f32
CosInRadians(f32 AngleInRadians)
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

inline v2f
PerpendicularOp(v2f A)
{
    v2f Result = {-A.y, A.x};
    return(Result);
}

inline f32 
DotProduct(v2f A, v2f B)
{
    f32 Result = A.x*B.x + A.y*B.y;

    return(Result);
}
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

inline f32
Clamp(f32 value, f32 min, f32 max)
{
    f32 result = value;

    if(result < min)
    {
        result = min;
    }
    else if(result > max)
    {
        result = max;
    }

    return(result);
}

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

inline f32
Floor(f32 floatToFloor)
{
    f32 result = floorf(floatToFloor);
    return result;
};

inline i32
FloorF32ToI32(f32 floatToFloor)
{
    i32 result = (i32)floorf(floatToFloor);
    return result;
};

inline i32
CeilF32ToI32(f32 floatToCeil)
{
    i32 result = (i32)ceilf(floatToCeil);
    return result;
};

inline v2f
PerpendicularOp(v2f A)
{
    v2f Result = {-A.y, A.x};
    return(Result);
}

inline f32 
DotProduct(v2f a, v2f b)
{
    f32 result = a.x*b.x + a.y*b.y;

    return(result);
}

inline f32 
MagnitudeSqd(v2f a)
{
    f32 result = DotProduct(a,a);

    return(result);
}

inline f32
Lerp(f32 a, f32 b, f32 t)
{
    f32 result = (1.0f - t)*a + t*b;

    return(result);
}

inline v4f
Lerp(v4f a, v4f b, f32 t)
{
    v4f result = (1.0f - t)*a + t*b;

    return(result);
}
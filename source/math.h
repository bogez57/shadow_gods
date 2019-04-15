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
Degrees(f32 angleInRadians)
{
    f32 angleInDegrees = (180.0f / PI) * angleInRadians;
    return angleInDegrees;
};

inline f32
SinR(f32 angleInRadians)
{
    f32 result = sinf(angleInRadians);
    return result;
};

inline f32
InvSinR(f32 angleInRadians)
{
    f32 result = asinf(angleInRadians);
    return result;
};

inline f32
CosR(f32 AngleInRadians)
{
    f32 result = cosf(AngleInRadians);
    return result;
};

inline f32
InvCosR(f32 angleInRadians)
{
    f32 result = acosf(angleInRadians);
    return result;
};

inline f32
TanR(f32 AngleInRadians)
{
    f32 result = tanf(AngleInRadians);
    return result;
};

inline f32
InvTanR(f32 value)
{
    f32 result = atanf(value);
    return result;
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
Sqrt(f64 number)
{
    number = sqrt(number);
    return number;
};

inline f32
Sqrt(f32 number)
{
    number = sqrtf(number);
    return number;
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
DotProduct(v3f a, v3f b)
{
    f32 result = a.x*b.x + a.y*b.y + a.z*b.z;

    return(result);
}

inline v2f
Hadamard(v2f a, v2f b)
{
    v2f result = {a.x*b.x, a.y*b.y};

    return(result);
}

inline v3f
Hadamard(v3f a, v3f b)
{
    v3f result = {a.x*b.x, a.y*b.y, a.z*b.z};

    return(result);
}

inline f32 
MagnitudeSqd(v2f a)
{
    f32 result = DotProduct(a,a);

    return(result);
}

inline f32 
MagnitudeSqd(v3f a)
{
    f32 result = DotProduct(a,a);

    return(result);
}

inline f32
Magnitude(v2f a)
{
    f32 result = Sqrt(MagnitudeSqd(a));
    return(result);
}

inline f32
Magnitude(v3f a)
{
    f32 result = Sqrt(MagnitudeSqd(a));
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

inline void
Normalize(v2f&& a)
{
    a *= (1.0f / Magnitude(a));
};

inline void
Normalize(v3f&& a)
{
    a *= (1.0f / Magnitude(a));
};
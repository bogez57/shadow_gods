#pragma once

#include <math.h> //TODO: Remove and replace with own, faster platform specific implementations

#define PI 3.1415926535897932385f

inline f32
Max(f32 x, f32 y)
{
    f32 result = fmaxf(x, y);
    return result;
};

inline f32
Min(f32 x, f32 y)
{
    f32 result = fminf(x, y);
    return result;
};

inline f32
Mod(f32 x, f32 y)
{
    f32 result = fmodf(x, y);
    return result;
};

inline void
AbsoluteVal(i32&& Value)
{
    Value = abs(Value);
}

inline void
AbsoluteVal(f32&& Value)
{
    Value = (f32)fabs(Value);
}

inline void
AbsoluteVal(v2&& Value)
{
    Value = { (f32)fabs(Value.x), (f32)fabs(Value.y) };
}

inline f32
ToRadians(f32 angleInDegrees)
{
    f32 angleInRadians = angleInDegrees * (PI / 180.0f);
    return angleInRadians;
};

inline f32
ToDegrees(f32 angleInRadians)
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
SinD(f32 angleInDegrees)
{
    f32 angleInRadians = ToRadians(angleInDegrees);
    f32 result = sinf(angleInRadians);
    
    return result;
};

inline f32
InvSinD(f32 angleInDegrees)
{
    f32 angleInRadians = ToRadians(angleInDegrees);
    f32 result = asinf(angleInRadians);
    
    return result;
};

inline f32
CosD(f32 angleInDegrees)
{
    f32 angleInRadians = ToRadians(angleInDegrees);
    f32 result = cosf(angleInRadians);
    
    return result;
};

inline f32
InvCosD(f32 angleInDegrees)
{
    f32 angleInRadians = ToRadians(angleInDegrees);
    f32 result = acosf(angleInRadians);
    
    return result;
};

inline f32
TanD(f32 angleInDegrees)
{
    f32 angleInRadians = ToRadians(angleInDegrees);
    f32 result = tanf(angleInRadians);
    
    return result;
};

inline f32
InvTanD(f32 angleInDegrees)
{
    f32 angleInRadians = ToRadians(angleInDegrees);
    f32 result = atanf(angleInRadians);
    
    return result;
};


inline void
Clamp(f32&& value, f32 min, f32 max)
{
    if(value < min)
    {
        value = min;
    }
    else if(value > max)
    {
        value = max;
    }
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

inline v2
PerpendicularOp(v2 A)
{
    v2 Result = {-A.y, A.x};
    return(Result);
}

inline f32
DotProduct(v2 a, v2 b)
{
    f32 result = a.x*b.x + a.y*b.y;
    
    return(result);
}

inline f32
DotProduct(v3 a, v3 b)
{
    f32 result = a.x*b.x + a.y*b.y + a.z*b.z;
    
    return(result);
}

inline v2
Hadamard(v2 a, v2 b)
{
    v2 result = {a.x*b.x, a.y*b.y};
    
    return(result);
}

inline v3
Hadamard(v3 a, v3 b)
{
    v3 result = {a.x*b.x, a.y*b.y, a.z*b.z};
    
    return(result);
}

inline f32
CrossProduct(v2 a, v2 b)
{
    return a.x * b.y - a.y * b.x;
}

inline f32
MagnitudeSqd(v2 a)
{
    f32 result = DotProduct(a,a);
    
    return(result);
}

inline f32
MagnitudeSqd(v3 a)
{
    f32 result = DotProduct(a,a);
    
    return(result);
}

inline f32
Magnitude(v2 a)
{
    f32 result = Sqrt(MagnitudeSqd(a));
    return(result);
}

inline f32
Magnitude(v3 a)
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

inline v4
Lerp(v4 a, v4 b, f32 t)
{
    v4 result = (1.0f - t)*a + t*b;
    
    return(result);
}

inline v2
Lerp(v2 a, v2 b, f32 t)
{
    v2 result = (1.0f - t)*a + t*b;
    
    return(result);
}

inline void
Normalize(v2&& a)
{
    a *= (1.0f / Magnitude(a));
};

inline void
Normalize(v3&& a)
{
    a *= (1.0f / Magnitude(a));
};

local_func
void ConvertNegativeToPositiveAngle_Radians(f32&& radianAngle)
{
    f32 circumferenceInRadians = 2*PI;
    radianAngle = Mod(radianAngle, circumferenceInRadians);
    if (radianAngle < 0) radianAngle += circumferenceInRadians;
};

local_func
void ConvertPositiveToNegativeAngle_Radians(f32&& radianAngle)
{
    if(radianAngle == 0.0f)
    {
        radianAngle = -6.28f;
    }
    else
    {
        f32 unitCircleCircumferenceInRadians = 2*PI;
        radianAngle = Mod(radianAngle, unitCircleCircumferenceInRadians);
        if (radianAngle > 0) radianAngle -= unitCircleCircumferenceInRadians;
    }
};

local_func
void ConvertToCorrectPositiveRadian(f32&& angle)
{
    f32 unitCircleCircumferenceInRadians = 2*PI;
    angle = Mod(angle, unitCircleCircumferenceInRadians);
};

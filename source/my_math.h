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
AbsoluteVal(s32&& Value)
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

inline s32
FloorF32ToI32(f32 floatToFloor)
{
    s32 result = (s32)floorf(floatToFloor);
    return result;
};

inline s32
CeilF32ToI32(f32 floatToCeil)
{
    s32 result = (s32)ceilf(floatToCeil);
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

Mat4x4 XRotation(f32 Angle)
{
    f32 c = CosR(Angle);
    f32 s = SinR(Angle);
    
    Mat4x4 R =
    {
        {
            {1, 0, 0, 0},
            {0, c,-s, 0},
            {0, s, c, 0},
            {0, 0, 0, 1}
        },
    };
    
    return(R);
}

inline Mat4x4
YRotation(f32 Angle)
{
    f32 c = CosR(Angle);
    f32 s = SinR(Angle);
    
    Mat4x4 R =
    {
        {
            { c, 0, s, 0},
            { 0, 1, 0, 0},
            {-s, 0, c, 0},
            { 0, 0, 0, 1}
        },
    };
    
    return(R);
}

inline Mat4x4
ZRotation(f32 Angle)
{
    f32 c = CosR(Angle);
    f32 s = SinR(Angle);
    
    Mat4x4 result =
    {
        {
            {c,-s, 0, 0},
            {s, c, 0, 0},
            {0, 0, 1, 0},
            {0, 0, 0, 1}
        },
    };
    
    return(result);
}

inline Mat4x4 Scale(v3 scale)
{
    v3 s = scale;
    Mat4x4 result =
    {
        {
            {s.x, 0,   0,   0},
            {0,   s.y, 0,   0},
            {0,   0,   s.z, 0},
            {0,   0,   0,   1}
        },
    };
    
    return result;
};

local_func Mat4x4 Translate(Mat4x4 A, v4 T)
{
    Mat4x4 result = A;
    
    result.elem[0][3] += T.x;
    result.elem[1][3] += T.y;
    result.elem[2][3] += T.z;
    
    return result;
};

local_func Mat4x4 ProduceWorldTransformMatrix(v3 translation, v3 rotation, v3 scale)
{
    Mat4x4 result{};
    
    ConvertToCorrectPositiveRadian($(rotation.x));
    ConvertToCorrectPositiveRadian($(rotation.y));
    ConvertToCorrectPositiveRadian($(rotation.z));
    
    Mat4x4 xRotMatrix = XRotation(rotation.x);
    Mat4x4 yRotMatrix = YRotation(rotation.y);
    Mat4x4 zRotMatrix = ZRotation(rotation.z);
    Mat4x4 fullRotMatrix = xRotMatrix * yRotMatrix * zRotMatrix;
    
    result = Translate(fullRotMatrix, v4{translation, 1.0f});
    
    return result;
};

local_func Mat4x4 ProduceCameraTransformMatrix(v3 xAxis, v3 yAxis, v3 zAxis, v3 vecToTransform)
{
    Mat4x4 result = RowPicture3x3(xAxis, yAxis, zAxis);
    v4 vecToTransform_4d {vecToTransform, 1.0f};
    result = Translate(result, -(result*vecToTransform_4d));
    
    return result;
};

local_func Mat4x4 ProduceProjectionTransformMatrix_UsingFOV(f32 FOV_inDegrees, f32 aspectRatio, f32 nearPlane, f32 farPlane)
{
    f32 fov = ToRadians(FOV_inDegrees);
    f32 tanHalfFov = TanR(fov / 2.0f);
    f32 xScale = 1.0f / (tanHalfFov * aspectRatio);
    f32 yScale = 1.0f / tanHalfFov;
    
    f32 a = (-farPlane - nearPlane) / (nearPlane - farPlane);
    f32 b = (2.0f * farPlane * nearPlane) / (nearPlane - farPlane);
    
    Mat4x4 result =
    {
        {
            {xScale, 0,      0,  0},
            {  0,    yScale, 0,  0},
            {  0,    0,      a,  b},
            {  0,    0,      1,  0}
        },
    };
    
    return result;
};

inline Mat4x4 IdentityMatrix()
{
    Mat4x4 R =
    {
        {{1, 0, 0, 0},
            {0, 1, 0, 0},
            {0, 0, 1, 0},
            {0, 0, 0, 1}},
    };
    
    return(R);
}

inline Mat4x4
Transpose(Mat4x4 A)
{
    Mat4x4 R;
    
    for(int j = 0; j <= 3; ++j)
    {
        for(int i = 0; i <= 3; ++i)
        {
            R.elem[j][i] = A.elem[i][j];
        }
    }
    
    return(R);
}
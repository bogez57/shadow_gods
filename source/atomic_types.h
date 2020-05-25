#ifndef ATOMIC_TYPES_INCLUDE
#define ATOMIC_TYPES_INCLUDE

#include <stdint.h>
#include <utility>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef i32 b32;
typedef bool b;

typedef uint8_t ui8;
typedef uint16_t ui16;
typedef uint32_t ui32;
typedef uint64_t ui64;
typedef uintptr_t uintptr;

typedef size_t sizet;

typedef float f32;
typedef double f64;

#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value) * 1024LL)
#define Gigabytes(Value) (Megabytes(Value) * 1024LL)
#define Terabytes(Value) (Gigabytes(Value) * 1024LL)

#define local_persist static
#define local_func static
#define global_variable static
#define $(var) std::move(var) //Use this for compiler enforced out parameter
#define NOT !

#define Align16(Value) ((Value + 15) & ~15)

//Should I just template things??
typedef union v2
{
#ifdef __cplusplus
    v2() = default;
    v2 (f32 x, f32 y) : x(x), y(y){};
#endif
    
    struct
    {
        f32 x, y;
    };
    
    struct
    {
        f32 u, v;
    };
    
    struct
    {
        f32 width, height;
    };
    
    f32 elem[2];
    
#ifdef __cplusplus
    inline f32 &operator[](const int &index)
    {
        return elem[index];
    }
#endif
} v2;

typedef union v3
{
#ifdef __cplusplus
    v3() = default;
    v3 (f32 x, f32 y, f32 z) : x(x), y(y), z(z){};
    v3 (v2 vec2, f32 z) : xy(vec2), z(z){};
#endif
    
    struct
    {
        f32 x, y, z;
    };
    
    struct
    {
        f32 u, v, w;
    };
    
    struct
    {
        f32 r, g, b;
    };
    
    struct
    {
        v2 xy;
        f32 ignored0_;
    };
    
    struct
    {
        f32 ignored1_;
        v2 yz;
    };
    
    struct
    {
        v2 uv;
        f32 ignored2_;
    };
    
    struct
    {
        f32 ignored3_;
        v2 vw;
    };
    
    f32 elem[3];
    
#ifdef __cplusplus
    inline f32 &operator[](const int &Index)
    {
        return elem[Index];
    }
#endif
} v3;

typedef union v4
{
#ifdef __cplusplus
    v4() = default;
    v4(v2 a, v2 b) : xy(a), zw(b){};
    v4(v3 a, f32 w) : xyz(a), w(w){};
#endif
    struct
    {
        union
        {
            v3 xyz;
            struct
            {
                f32 x, y, z;
            };
        };
        
        f32 w;
    };
    
    struct
    {
        union
        {
            v3 rgb;
            struct
            {
                f32 r, g, b;
            };
        };
        
        f32 a;
    };
    
    struct
    {
        v2 xy;
        f32 ignored0_;
        f32 ignored1_;
    };
    
    struct
    {
        f32 ignored2_;
        v2 yz;
        f32 ignored3_;
    };
    
    struct
    {
        f32 ignored4_;
        f32 ignored5_;
        v2 zw;
    };
    
    f32 elem[4];
    
#ifdef __cplusplus
    inline f32 &operator[](const int &index)
    {
        return elem[index];
    }
#endif
} v4;

struct mat4x4
{
    //These are stored ROW MAJOR - elem[ROW][COLUMN]!!!
    f32 elem[4][4];
};

struct mat3x3
{
    //These are stored ROW MAJOR - elem[ROW][COLUMN]!!!
    f32 elem[3][3];
};

struct mat2x2
{
    //These are stored ROW MAJOR - elem[ROW][COLUMN]!!!
    f32 elem[2][2];
};

inline mat4x4 IdentityMatrix();
local_func v2 CastV2IToV2F(v2i vecToCast);

//Other v2's I might use. Torn on whether or not I should template things but I think for 90 percent of what I'm using vectors for floats should be what I want
struct v2i
{
    v2i() = default;
    v2i(i32 x, i32 y);
    
    union
    {
        i32 Elem[2];
        struct
        {
            i32 x, y;
        };
        struct
        {
            i32 width, height;
        };
        struct
        {
            i32 u, v;
        };
    };
};

struct v3i
{
    v3i() = default;
    v3i(i32 x, i32 y, i32 z);
    
    union
    {
        i32 Elem[3];
        struct
        {
            i32 x, y, z;
        };
        
        struct
        {
            i32 r, g, b;
        };
    };
};

struct v4i
{
    v4i() = default;
    v4i(i32 x, i32 y, i32 z, i32 w);
    
    union
    {
        i32 Elem[4];
        struct
        {
            i32 x, y, z, w;
        };
        
        struct
        {
            i32 r, g, b, a;
        };
    };
};

struct v4ui32
{
    v4ui32() = default;
    v4ui32(ui32 x, ui32 y, ui32 z, ui32 w);
    
    union
    {
        i32 Elem[4];
        struct
        {
            ui32 x, y, z, w;
        };
        
        struct
        {
            ui32 r, g, b, a;
        };
    };
};

#endif

#ifdef ATOMIC_TYPES_IMPL

v2 CastV2IToV2F(v2i vecToCast)
{
    v2 result{};
    
    result.x = (f32)vecToCast.x;
    result.y = (f32)vecToCast.y;
    
    return result;
}

v2i CastV2FToV2I(v2 vecToCast)
{
    v2i result{};
    
    result.x = (i32)vecToCast.x;
    result.y = (i32)vecToCast.y;
    
    return result;
};

inline b
operator==(v2 a, v2 B)
{
    b result { false };
    
    if (a.x == B.x && a.y == B.y)
        result = true;
    
    return result;
};

inline b
operator!=(v2 a, v2 B)
{
    b result { false };
    
    if (a.x != B.x || a.y != B.y)
        result = true;
    
    return result;
};

inline v2
operator*(f32 a, v2 B)
{
    v2 result;
    
    result.x = a * B.x;
    result.y = a * B.y;
    
    return (result);
}

inline v2
operator*(v2 B, f32 a)
{
    v2 result = a * B;
    
    return (result);
}

inline v2&
operator*=(v2& B, f32 a)
{
    B = a * B;
    
    return (B);
}

inline v2
operator/(v2 b, f32 a)
{
    v2 result;
    result.x = b.x / a;
    result.y = b.y / a;
    
    return result;
}

inline v2
operator+(v2 a, v2 B)
{
    v2 result;
    
    result.x = a.x + B.x;
    result.y = a.y + B.y;
    
    return (result);
}

inline v2
operator+(v2 a, f32 B)
{
    v2 result;
    
    result.x = a.x + B;
    result.y = a.y + B;
    
    return (result);
}

inline v2&
operator+=(v2& a, v2 B)
{
    a = a + B;
    
    return (a);
}

inline v2&
operator+=(v2& a, f32 B)
{
    a.x = a.x + B;
    a.y = a.y + B;
    
    return (a);
}

inline v2&
operator-=(v2& a, f32 B)
{
    a.x = a.x - B;
    a.y = a.y - B;
    
    return (a);
}

inline v2&
operator-=(v2& a, v2 B)
{
    a.x = a.x - B.x;
    a.y = a.y - B.y;
    
    return (a);
}

inline v2
operator-(v2 a, v2 B)
{
    v2 result;
    
    result.x = a.x - B.x;
    result.y = a.y - B.y;
    
    return (result);
}

inline v2
operator-(v2 a, f32 b)
{
    v2 result;
    
    result.x = a.x - b;
    result.y = a.y - b;
    
    return (result);
}

inline v2
operator-(v2 a)
{
    v2 result;
    
    result.x = -a.x;
    result.y = -a.y;
    
    return(result);
}

inline v3
operator*(f32 A, v3 B)
{
    v3 result;
    
    result.x = A*B.x;
    result.y = A*B.y;
    result.z = A*B.z;
    
    return(result);
}

inline v3
operator*(v3 B, f32 A)
{
    v3 result = A*B;
    
    return(result);
}

inline v3 &
operator*=(v3 &B, f32 A)
{
    B = A * B;
    
    return(B);
}

inline v3
operator-(v3 A)
{
    v3 result;
    
    result.x = -A.x;
    result.y = -A.y;
    result.z = -A.z;
    
    return(result);
}

inline v3
operator+(v3 A, v3 B)
{
    v3 result;
    
    result.x = A.x + B.x;
    result.y = A.y + B.y;
    result.z = A.z + B.z;
    
    return(result);
}

inline v3
operator+(v3 a, f32 B)
{
    v3 result;
    
    result.x = a.x + B;
    result.y = a.y + B;
    result.z = a.z + B;
    
    return (result);
}

inline v3 &
operator+=(v3 &A, v3 B)
{
    A = A + B;
    
    return(A);
}

inline v3
operator-(v3 A, v3 B)
{
    v3 result;
    
    result.x = A.x - B.x;
    result.y = A.y - B.y;
    result.z = A.z - B.z;
    
    return(result);
}

inline v3 &
operator-=(v3 &A, v3 B)
{
    A = A - B;
    
    return(A);
}

inline v4
operator*(f32 a, v4 B)
{
    v4 result;
    
    result.x = a*B.x;
    result.y = a*B.y;
    result.z = a*B.z;
    result.w = a*B.w;
    
    return(result);
}

inline v4
operator*(v4 B, f32 a)
{
    v4 result = a*B;
    
    return(result);
}

inline v4 &
operator*=(v4 &B, f32 a)
{
    B = a * B;
    
    return(B);
}

inline v4
operator+(v4 a, v4 B)
{
    v4 result;
    
    result.x = a.x + B.x;
    result.y = a.y + B.y;
    result.z = a.z + B.z;
    result.w = a.w + B.w;
    
    return(result);
}

inline v4 &
operator+=(v4 &a, v4 B)
{
    a = a + B;
    
    return(a);
}

inline v4
operator-(v4 a, v4 b)
{
    v4 result;
    
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
    result.w = a.w - b.w;
    
    return (result);
}

inline v4
operator-(v4 a)
{
    v4 result;
    
    result.x = -a.x;
    result.y = -a.y;
    result.z = -a.z;
    result.w = -a.w;
    
    return (result);
}

inline mat4x4 IdentityMatrix()
{
    mat4x4 R =
    {
        {{1, 0, 0, 0},
            {0, 1, 0, 0},
            {0, 0, 1, 0},
            {0, 0, 0, 1}},
    };
    
    return(R);
}

local_func v4
TransformVec(mat4x4 A, v4 P)
{
    v4 R;
    
    R.x = P.x*A.elem[0][0] + P.y*A.elem[0][1] + P.z*A.elem[0][2] + P.w*A.elem[0][3];
    R.y = P.x*A.elem[1][0] + P.y*A.elem[1][1] + P.z*A.elem[1][2] + P.w*A.elem[1][3];
    R.z = P.x*A.elem[2][0] + P.y*A.elem[2][1] + P.z*A.elem[2][2] + P.w*A.elem[2][3];
    R.w = P.x*A.elem[3][0] + P.y*A.elem[3][1] + P.z*A.elem[3][2] + P.w*A.elem[3][3];
    
    return(R);
};

inline mat4x4
Transpose(mat4x4 A)
{
    mat4x4 R;
    
    for(int j = 0; j <= 3; ++j)
    {
        for(int i = 0; i <= 3; ++i)
        {
            R.elem[j][i] = A.elem[i][j];
        }
    }
    
    return(R);
}

inline v4
operator*(mat4x4 A, v4 P)
{
    v4 R = TransformVec(A, P);
    return(R);
};

local_func mat4x4
operator*(mat4x4 A, mat4x4 B)
{
    // NOTE(casey): This is written to be instructive, not optimal!
    
    mat4x4 R = {};
    
    for(int r = 0; r <= 3; ++r) // NOTE(casey): Rows (of A)
    {
        for(int c = 0; c <= 3; ++c) // NOTE(casey): Column (of B)
        {
            for(int i = 0; i <= 3; ++i) // NOTE(casey): Columns of A, rows of B!
            {
                R.elem[r][c] += A.elem[r][i]*B.elem[i][c];
            }
        }
    }
    
    return(R);
}

inline mat4x4
PerspectiveProjection(f32 aspectRatio, f32 focalLength)
{
    f32 a = 1.0f;
    f32 b = aspectRatio;
    f32 c = focalLength;
    
    mat4x4 r =
    {
        {
            {a*c, 0, 0, 0},
            {0, b*c, 0, 0},
            {0,   0, 1, 0},
            {0,   0,-1, 0}
        }
    };
    
    return r;
};

local_func mat4x4
ColumnPicture3x3(v3 xAxis, v3 yAxis, v3 zAxis)
{
    mat4x4 result =
    {
        {
            {xAxis.x, yAxis.x, zAxis.x, 0.0f},
            {xAxis.y, yAxis.y, zAxis.y, 0.0f},
            {xAxis.z, yAxis.z, zAxis.z, 0.0f},
            {   0.0f,    0.0f,    0.0f, 1.0f}
        }
    };
    
    return result;
};

local_func mat4x4
RowPicture3x3(v3 xAxis, v3 yAxis, v3 zAxis)
{
    mat4x4 result =
    {
        {
            {xAxis.x, xAxis.y, xAxis.z, 0.0f},
            {yAxis.x, yAxis.y, yAxis.z, 0.0f},
            {zAxis.x, zAxis.y, zAxis.z, 0.0f},
            {   0.0f,    0.0f,    0.0f, 1.0f}
        }
    };
    
    return result;
};

local_func mat4x4
Translate(mat4x4 A, v4 T)
{
    mat4x4 result = A;
    
    result.elem[0][3] += T.x;
    result.elem[1][3] += T.y;
    result.elem[2][3] += T.z;
    
    return result;
};

local_func mat4x4
CamTransform(v3 xAxis, v3 yAxis, v3 zAxis, v3 vecToTransform)
{
    mat4x4 result = RowPicture3x3(xAxis, yAxis, zAxis);
    v4 vecToTransform_4d {vecToTransform, 1.0f};
    result = Translate(result, -(result*vecToTransform_4d));
    
    return result;
};

//Other v2 implementations 
v2i::v2i(i32 x, i32 y)
: x(x)
, y(y)
{}

inline v2i
operator*(i32 a, v2i B)
{
    v2i result;
    
    result.x = a * B.x;
    result.y = a * B.y;
    
    return (result);
}

inline v2i
operator*(v2i B, i32 a)
{
    v2i result = a * B;
    
    return (result);
}

inline v2i&
operator*=(v2i& B, i32 a)
{
    B = a * B;
    
    return (B);
}

inline v2i
operator+(v2i a, v2i b)
{
    v2i result;
    
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    
    return (result);
}

inline v2i&
operator+=(v2i& a, v2i b)
{
    a = a + b;
    
    return (a);
}

inline v2i&
operator+=(v2i& a, i32 b)
{
    a.x = a.x + b;
    a.y = a.y + b;
    
    return (a);
}

inline v2i&
operator-=(v2i& a, i32 b)
{
    a.x = a.x - b;
    a.y = a.y - b;
    
    return (a);
}

inline v2i&
operator-=(v2i& a, v2i b)
{
    a.x = a.x - b.x;
    a.y = a.y - b.y;
    
    return (a);
}

inline v2i
operator-(v2i a, v2i b)
{
    v2i result;
    
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    
    return (result);
}

v3i::v3i(i32 x, i32 y, i32 z)
: x(x)
, y(y)
, z(z)
{}

/* all v3i suff */

v4i::v4i(i32 x, i32 y, i32 z, i32 w)
: x(x)
, y(y)
, z(z)
, w(w)
{}

#endif // ATOMIC_TYPES_IMPL
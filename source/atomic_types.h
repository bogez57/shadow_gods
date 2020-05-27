#ifndef ATOMIC_TYPES_INCLUDE
#define ATOMIC_TYPES_INCLUDE

#include <stdint.h>
#include <utility>

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef s32 b32;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
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

struct v2f
{
    v2f() = default;
    v2f(f32 x, f32 y);
    
    union
    {
        f32 Elem[2];
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
    };
};

struct v3f
{
    v3f() = default;
    v3f(f32 x, f32 y, f32 z);
    
    union
    {
        f32 Elem[3];
        struct
        {
            f32 x, y, z;
        };
        
        struct
        {
            f32 r, g, b;
        };
    };
};

struct v4f
{
    v4f() = default;
    v4f(f32 x, f32 y, f32 z, f32 w);
    
    union
    {
        f32 Elem[4];
        struct
        {
            union
            {
                v3f rgb;
                struct
                {
                    f32 r, g, b;
                };
            };
            
            f32 a;
        };
        
        struct
        {
            union
            {
                v3f xyz;
                struct
                {
                    f32 x, y, z;
                };
            };
            
            f32 w;
        };
        
        struct
        {
            v2f xy;
            f32 _Ignored0;
            f32 _Ignored1;
        };
    };
};

struct v2i
{
    v2i() = default;
    v2i(s32 x, s32 y);
    
    union
    {
        s32 Elem[2];
        struct
        {
            s32 x, y;
        };
        struct
        {
            s32 width, height;
        };
        struct
        {
            s32 u, v;
        };
    };
};

struct v3i
{
    v3i() = default;
    v3i(s32 x, s32 y, s32 z);
    
    union
    {
        s32 Elem[3];
        struct
        {
            s32 x, y, z;
        };
        
        struct
        {
            s32 r, g, b;
        };
    };
};

struct v4i
{
    v4i() = default;
    v4i(s32 x, s32 y, s32 z, s32 w);
    
    union
    {
        s32 Elem[4];
        struct
        {
            s32 x, y, z, w;
        };
        
        struct
        {
            s32 r, g, b, a;
        };
    };
};

struct v4u32
{
    v4u32() = default;
    v4u32(u32 x, u32 y, u32 z, u32 w);
    
    union
    {
        s32 Elem[4];
        struct
        {
            u32 x, y, z, w;
        };
        
        struct
        {
            u32 r, g, b, a;
        };
    };
};

struct m2x2
{
    f32 Elem[2][2];
};

local_func
v2f CastV2IToV2F(v2i vecToCast);

#endif

#ifdef ATOMIC_TYPES_IMPL

v2f CastV2IToV2F(v2i vecToCast)
{
    v2f result{};
    
    result.x = (f32)vecToCast.x;
    result.y = (f32)vecToCast.y;
    
    return result;
}

v2i CastV2FToV2I(v2f vecToCast)
{
    v2i result{};
    
    result.x = (s32)vecToCast.x;
    result.y = (s32)vecToCast.y;
    
    return result;
};

v2f::v2f(f32 x, f32 y)
: x(x)
, y(y)
{}

inline bool
operator==(v2f a, v2f B)
{
    bool  result { false };
    
    if (a.x == B.x && a.y == B.y)
        result = true;
    
    return result;
};

inline bool
operator!=(v2f a, v2f B)
{
    bool  result { false };
    
    if (a.x != B.x || a.y != B.y)
        result = true;
    
    return result;
};

inline v2f
operator*(f32 a, v2f B)
{
    v2f result;
    
    result.x = a * B.x;
    result.y = a * B.y;
    
    return (result);
}

inline v2f
operator*(v2f B, f32 a)
{
    v2f result = a * B;
    
    return (result);
}

inline v2f&
operator*=(v2f& B, f32 a)
{
    B = a * B;
    
    return (B);
}

inline v2f
operator/(v2f b, f32 a)
{
    v2f result;
    result.x = b.x / a;
    result.y = b.y / a;
    
    return result;
}

inline v2f
operator+(v2f a, v2f B)
{
    v2f result;
    
    result.x = a.x + B.x;
    result.y = a.y + B.y;
    
    return (result);
}

inline v2f
operator+(v2f a, f32 B)
{
    v2f result;
    
    result.x = a.x + B;
    result.y = a.y + B;
    
    return (result);
}

inline v2f&
operator+=(v2f& a, v2f B)
{
    a = a + B;
    
    return (a);
}

inline v2f&
operator+=(v2f& a, f32 B)
{
    a.x = a.x + B;
    a.y = a.y + B;
    
    return (a);
}

inline v2f&
operator-=(v2f& a, f32 B)
{
    a.x = a.x - B;
    a.y = a.y - B;
    
    return (a);
}

inline v2f&
operator-=(v2f& a, v2f B)
{
    a.x = a.x - B.x;
    a.y = a.y - B.y;
    
    return (a);
}

inline v2f
operator-(v2f a, v2f B)
{
    v2f result;
    
    result.x = a.x - B.x;
    result.y = a.y - B.y;
    
    return (result);
}

inline v2f
operator-(v2f a, f32 b)
{
    v2f result;
    
    result.x = a.x - b;
    result.y = a.y - b;
    
    return (result);
}

inline v2f
operator-(v2f a)
{
    v2f result;
    
    result.x = -a.x;
    result.y = -a.y;
    
    return(result);
}

v3f::v3f(f32 x, f32 y, f32 z)
: x(x)
, y(y)
, z(z)
{}

inline v3f
operator*(f32 A, v3f B)
{
    v3f result;
    
    result.x = A*B.x;
    result.y = A*B.y;
    result.z = A*B.z;
    
    return(result);
}

inline v3f
operator*(v3f B, f32 A)
{
    v3f result = A*B;
    
    return(result);
}

inline v3f &
operator*=(v3f &B, f32 A)
{
    B = A * B;
    
    return(B);
}

inline v3f
operator-(v3f A)
{
    v3f result;
    
    result.x = -A.x;
    result.y = -A.y;
    result.z = -A.z;
    
    return(result);
}

inline v3f
operator+(v3f A, v3f B)
{
    v3f result;
    
    result.x = A.x + B.x;
    result.y = A.y + B.y;
    result.z = A.z + B.z;
    
    return(result);
}

inline v3f &
operator+=(v3f &A, v3f B)
{
    A = A + B;
    
    return(A);
}

inline v3f
operator-(v3f A, v3f B)
{
    v3f result;
    
    result.x = A.x - B.x;
    result.y = A.y - B.y;
    result.z = A.z - B.z;
    
    return(result);
}

inline v3f &
operator-=(v3f &A, v3f B)
{
    A = A - B;
    
    return(A);
}

v4f::v4f(f32 x, f32 y, f32 z, f32 w)
: x(x)
, y(y)
, z(z)
, w(w)
{}

inline v4f
operator*(f32 a, v4f B)
{
    v4f result;
    
    result.x = a*B.x;
    result.y = a*B.y;
    result.z = a*B.z;
    result.w = a*B.w;
    
    return(result);
}

inline v4f
operator*(v4f B, f32 a)
{
    v4f result = a*B;
    
    return(result);
}

inline v4f &
operator*=(v4f &B, f32 a)
{
    B = a * B;
    
    return(B);
}

inline v4f
operator+(v4f a, v4f B)
{
    v4f result;
    
    result.x = a.x + B.x;
    result.y = a.y + B.y;
    result.z = a.z + B.z;
    result.w = a.w + B.w;
    
    return(result);
}

inline v4f &
operator+=(v4f &a, v4f B)
{
    a = a + B;
    
    return(a);
}

v2i::v2i(s32 x, s32 y)
: x(x)
, y(y)
{}

inline v2i
operator*(s32 a, v2i B)
{
    v2i result;
    
    result.x = a * B.x;
    result.y = a * B.y;
    
    return (result);
}

inline v2i
operator*(v2i B, s32 a)
{
    v2i result = a * B;
    
    return (result);
}

inline v2i&
operator*=(v2i& B, s32 a)
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
operator+=(v2i& a, s32 b)
{
    a.x = a.x + b;
    a.y = a.y + b;
    
    return (a);
}

inline v2i&
operator-=(v2i& a, s32 b)
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

v3i::v3i(s32 x, s32 y, s32 z)
: x(x)
, y(y)
, z(z)
{}

/* all v3i suff */

v4i::v4i(s32 x, s32 y, s32 z, s32 w)
: x(x)
, y(y)
, z(z)
, w(w)
{}

#endif // ATOMIC_TYPES_IMPL
#pragma once

#include <stdint.h>
    
typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

typedef unsigned int uint;
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef size_t sizet;

typedef float f32;
typedef double f64;

#define local_persist static
#define local_func static
#define global_variable static
#define global_func

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
            f32 Width, Height;
        };
    };
};

v2f::v2f(f32 x, f32 y) :
    x(x),
    y(y)
{}

inline v2f
operator*(f32 A,  v2f B)
{
    v2f Result;

    Result.x = A*B.x;
    Result.y = A*B.y;

    return(Result);
}

inline v2f
operator*(v2f B, f32 A)
{
    v2f Result = A*B;

    return(Result);
}

inline  v2f&
operator*=(v2f& B, f32 A)
{
    B = A * B;

    return(B);
}

inline v2f
operator+(v2f A, v2f B)
{
    v2f Result;

    Result.x = A.x + B.x;
    Result.y = A.y + B.y;

    return(Result);
}

inline v2f&
operator+=(v2f& A, v2f B)
{
    A = A + B;

    return(A);
}

inline v2f&
operator+=(v2f& A, f32 B)
{
    A.x = A.x + B;
    A.y = A.y + B;

    return(A);
}

inline v2f&
operator-=(v2f& A, f32 B)
{
    A.x = A.x - B;
    A.y = A.y - B;

    return(A);
}

inline v2f&
operator-=(v2f& A, v2f B)
{
    A.x = A.x - B.x;
    A.y = A.y - B.y;

    return(A);
}

inline v2f
operator-(v2f A, v2f B)
{
    v2f Result;

    Result.x = A.x - B.x;
    Result.y = A.y - B.y;

    return(Result);
}

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

v3f::v3f(f32 x, f32 y, f32 z) :
    x(x),
    y(y),
    z(z)
{}

struct v2i
{
    v2i() = default;
    v2i(int x, int y);

    union
    {
        int Elem[2];
        struct
        {
            int x, y;
        };
        struct
        {
            int Width, Height;
        };
    };
};

v2i::v2i(int x, int y) :
    x(x),
    y(y)
{}

inline v2i
operator*(int A,  v2i B)
{
    v2i Result;

    Result.x = A*B.x;
    Result.y = A*B.y;

    return(Result);
}

inline v2i
operator*(v2i B, int A)
{
    v2i Result = A*B;

    return(Result);
}

inline  v2i&
operator*=(v2i& B, int A)
{
    B = A * B;

    return(B);
}

inline v2i
operator+(v2i A, v2i B)
{
    v2i Result;

    Result.x = A.x + B.x;
    Result.y = A.y + B.y;

    return(Result);
}

inline v2i&
operator+=(v2i& A, v2i B)
{
    A = A + B;

    return(A);
}

inline v2i&
operator+=(v2i& A, int B)
{
    A.x = A.x + B;
    A.y = A.y + B;

    return(A);
}

inline v2i&
operator-=(v2i& A, int B)
{
    A.x = A.x - B;
    A.y = A.y - B;

    return(A);
}

inline v2i&
operator-=(v2i& A, v2i B)
{
    A.x = A.x - B.x;
    A.y = A.y - B.y;

    return(A);
}

inline v2i
operator-(v2i A, v2i B)
{
    v2i Result;

    Result.x = A.x - B.x;
    Result.y = A.y - B.y;

    return(Result);
}

struct v3i
{
    v3i() = default;
    v3i(int x, int y, int z);

    union
    {
        int Elem[3];
        struct
        {
            int x, y, z;
        };

        struct
        {
            int r, g, b;
        };
    };
};

v3i::v3i(int x, int y, int z) :
    x(x),
    y(y),
    z(z)
{}
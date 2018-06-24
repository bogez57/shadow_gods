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

typedef float float32;
typedef double float64;

#define local_persist static
#define local_func static
#define global_variable static
#define global_func

struct vec2
{
    vec2() = default;
    vec2(float32 x, float32 y);

    union
    {
        float32 Elem[2];
        struct
        {
            float32 x, y;
        };
    };
};

vec2::vec2(float32 x, float32 y) :
    x(x),
    y(y)
{}

inline vec2
operator*(float32 A,  vec2 B)
{
    vec2 Result;

    Result.x = A*B.x;
    Result.y = A*B.y;

    return(Result);
}

inline vec2
operator*(vec2 B, float32 A)
{
    vec2 Result = A*B;

    return(Result);
}

inline  vec2&
operator*=(vec2& B, float32 A)
{
    B = A * B;

    return(B);
}

inline vec2
operator+(vec2 A, vec2 B)
{
    vec2 Result;

    Result.x = A.x + B.x;
    Result.y = A.y + B.y;

    return(Result);
}

inline vec2&
operator+=(vec2& A, vec2 B)
{
    A = A + B;

    return(A);
}

inline vec2&
operator+=(vec2& A, float32 B)
{
    A.x = A.x + B;
    A.y = A.y + B;

    return(A);
}

inline vec2&
operator-=(vec2& A, float32 B)
{
    A.x = A.x - B;
    A.y = A.y - B;

    return(A);
}

inline vec2&
operator-=(vec2& A, vec2 B)
{
    A.x = A.x - B.x;
    A.y = A.y - B.y;

    return(A);
}

inline vec2
operator-(vec2 A, vec2 B)
{
    vec2 Result;

    Result.x = A.x - B.x;
    Result.y = A.y - B.y;

    return(Result);
}

struct vec3
{
    vec3() = default;
    vec3(float32 x, float32 y, float32 z);

    union
    {
        float32 Elem[3];
        struct
        {
            float32 x, y, z;
        };

        struct
        {
            float32 r, g, b;
        };
    };
};

vec3::vec3(float32 x, float32 y, float32 z) :
    x(x),
    y(y),
    z(z)
{}
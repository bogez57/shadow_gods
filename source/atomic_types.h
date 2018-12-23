#pragma once

#include <stdint.h>

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
#define OUT
#define NOT !

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
            f32 x, y, z, w;
        };

        struct
        {
            f32 r, g, b, a;
        };
    };
};

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
            int width, height;
        };
    };
};

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

struct m2x2
{
    f32 Elem[2][2];
};
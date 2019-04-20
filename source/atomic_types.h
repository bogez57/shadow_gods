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
    };
};

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

struct v4ui
{
    v4ui() = default;
    v4ui(ui32 x, ui32 y, ui32 z, ui32 w);

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

struct m2x2
{
    f32 Elem[2][2];
};

#endif

#ifdef ATOMIC_TYPES_IMPL

v2f::v2f(f32 x, f32 y)
    : x(x)
    , y(y)
{}

inline b
operator==(v2f a, v2f B)
{
    b result { false };

    if (a.x == B.x && a.y == B.y)
        result = true;

    return result;
};

inline b
operator!=(v2f a, v2f B)
{
    b result { false };

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
#include "atomic_types.h"

v2f::v2f(f32 x, f32 y)
    : x(x)
    , y(y)
{}

inline b
operator==(v2f A, v2f B)
{
    b Result { false };

    if (A.x == B.x && A.y == B.y)
        Result = true;

    return Result;
};

inline b
operator!=(v2f A, v2f B)
{
    b Result { false };

    if (A.x != B.x || A.y != B.y)
        Result = true;

    return Result;
};

inline v2f
operator*(f32 A, v2f B)
{
    v2f Result;

    Result.x = A * B.x;
    Result.y = A * B.y;

    return (Result);
}

inline v2f
operator*(v2f B, f32 A)
{
    v2f Result = A * B;

    return (Result);
}

inline v2f&
operator*=(v2f& B, f32 A)
{
    B = A * B;

    return (B);
}

inline v2f
operator+(v2f A, v2f B)
{
    v2f Result;

    Result.x = A.x + B.x;
    Result.y = A.y + B.y;

    return (Result);
}

inline v2f&
operator+=(v2f& A, v2f B)
{
    A = A + B;

    return (A);
}

inline v2f&
operator+=(v2f& A, f32 B)
{
    A.x = A.x + B;
    A.y = A.y + B;

    return (A);
}

inline v2f&
operator-=(v2f& A, f32 B)
{
    A.x = A.x - B;
    A.y = A.y - B;

    return (A);
}

inline v2f&
operator-=(v2f& A, v2f B)
{
    A.x = A.x - B.x;
    A.y = A.y - B.y;

    return (A);
}

inline v2f
operator-(v2f A, v2f B)
{
    v2f Result;

    Result.x = A.x - B.x;
    Result.y = A.y - B.y;

    return (Result);
}

v3f::v3f(f32 x, f32 y, f32 z)
    : x(x)
    , y(y)
    , z(z)
{}

/* all v3f suff */

v4f::v4f(f32 x, f32 y, f32 z, f32 w)
    : x(x)
    , y(y)
    , z(z)
    , w(w)
{}

/* all v4f suff */

v2i::v2i(int x, int y)
    : x(x)
    , y(y)
{}

inline v2i
operator*(int A, v2i B)
{
    v2i Result;

    Result.x = A * B.x;
    Result.y = A * B.y;

    return (Result);
}

inline v2i
operator*(v2i B, int A)
{
    v2i Result = A * B;

    return (Result);
}

inline v2i&
operator*=(v2i& B, int A)
{
    B = A * B;

    return (B);
}

inline v2i
operator+(v2i A, v2i B)
{
    v2i Result;

    Result.x = A.x + B.x;
    Result.y = A.y + B.y;

    return (Result);
}

inline v2i&
operator+=(v2i& A, v2i B)
{
    A = A + B;

    return (A);
}

inline v2i&
operator+=(v2i& A, int B)
{
    A.x = A.x + B;
    A.y = A.y + B;

    return (A);
}

inline v2i&
operator-=(v2i& A, int B)
{
    A.x = A.x - B;
    A.y = A.y - B;

    return (A);
}

inline v2i&
operator-=(v2i& A, v2i B)
{
    A.x = A.x - B.x;
    A.y = A.y - B.y;

    return (A);
}

inline v2i
operator-(v2i A, v2i B)
{
    v2i Result;

    Result.x = A.x - B.x;
    Result.y = A.y - B.y;

    return (Result);
}

v3i::v3i(int x, int y, int z)
    : x(x)
    , y(y)
    , z(z)
{}

/* all v3i suff */
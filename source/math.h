#pragma once

#include "types.h"

struct vec2
{
    vec2() = default;
    vec2(float32 X, float32 Y);

    union
    {
        float32 Elem[2];
        struct
        {
            float32 X, Y;
        };
    };
};

vec2::vec2(float32 X, float32 Y) :
    X(X),
    Y(Y)
{}

inline vec2
V2(float32 X, float32 Y)
{
    vec2 Result;

    Result.X = X;
    Result.Y = Y;

    return(Result);
}

inline vec2
operator*(float32 A,  vec2 B)
{
    vec2 Result;

    Result.X = A*B.X;
    Result.Y = A*B.Y;

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
operator-(vec2 A)
{
    vec2 Result;

    Result.X = -A.X;
    Result.Y = -A.Y;

    return(Result);
}

inline vec2
operator+(vec2 A, vec2 B)
{
    vec2 Result;

    Result.X = A.X + B.X;
    Result.Y = A.Y + B.Y;

    return(Result);
}

inline  vec2&
operator+=(vec2& A, vec2 B)
{
    A = A + B;

    return(A);
}

inline vec2
operator-(vec2 A, vec2 B)
{
    vec2 Result;

    Result.X = A.X - B.X;
    Result.Y = A.Y - B.Y;

    return(Result);
}
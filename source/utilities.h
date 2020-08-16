#pragma once
#include <math.h>

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

#include <utility>
#include <string.h>

template <typename F>
struct Defer {
    Defer( F f ) : f( f ) {}
    ~Defer( ) { f( ); }
    F f;
};

template <typename F>
Defer<F> makeDefer( F f ) {
    return Defer<F>( f );
};

#define __defer( line ) defer_ ## line
#define _defer( line ) __defer( line )

struct defer_dummy { };
template<typename F>
Defer<F> operator+( defer_dummy, F&& f )
{
    return makeDefer<F>( std::forward<F>( f ) );
}

#define defer auto _defer( __LINE__ ) = defer_dummy( ) + [ & ]( )

struct Transform
{
    Transform() = default;
    Transform(v2 translation, f32 rotation, v2 scale) :
    translation{translation},
    rotation{rotation},
    scale{scale}
    {}
    
    v2 translation{};
    f32 rotation{};
    v2 scale{};
};

inline ui32
SafeTruncateUInt64(ui64 Value)
{
    BGZ_ASSERT(Value <= 0xFFFFFFFF, "Make sure there wouldn't be information lost with truncate if value was over 32-bits");
    ui32 result = (ui32)Value;
    return (result);
};

inline auto Swap(f32* a, f32* b) -> void
{
    f32 c { *a };
    *a = *b;
    *b = c;
};

inline auto Swap(v2* a, v2* b) -> void
{
    v2 c { *a };
    *a = *b;
    *b = c;
};

inline f32 Round(f32 floatToRound)
{
    return floorf(floatToRound + .5f);
};

inline i32
RoundFloat32ToInt32(f32 float32)
{
    i32 result{};
    
    if(float32 < 0.0f) i32 result = (i32)(float32 - 0.5f);
    else result = (i32)(float32 + 0.5f);
    
    return(result);
}

inline ui32
RoundFloat32ToUInt32(f32 float32)
{
    ui32 result;
    
    if(float32 < 0.0f) i32 result = (i32)(float32 - 0.5f);
    else result = (ui32)(float32 + 0.5f);
    
    return(result);
}

inline b32
StringCmp(const char* str1, const char* str2)
{
    b32 didStringsMatch = strcmp(str1, str2);
    
    if(!didStringsMatch)
        return 1;
    else
        return 0;
};
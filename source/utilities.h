#pragma once

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
    Transform(v2f translation, f32 rotation, v2f scale) :
    translation{translation},
    rotation{rotation},
    scale{scale}
    {}
    
    v2f translation{};
    f32 rotation{};
    v2f scale{};
};

inline u32
SafeTruncateUInt64(u64 Value)
{
    BGZ_ASSERT(Value <= 0xFFFFFFFF, "Make sure there wouldn't be information lost with truncate if value was over 32-bits");
    u32 result = (u32)Value;
    return (result);
};

inline auto Swap(f32* a, f32* b) -> void
{
    f32 c { *a };
    *a = *b;
    *b = c;
};

inline auto Swap(v2f* a, v2f* b) -> void
{
    v2f c { *a };
    *a = *b;
    *b = c;
};

inline s32
RoundFloat32ToInt32(f32 float32)
{
    s32 result{};
    
    if(float32 < 0.0f) s32 result = (s32)(float32 - 0.5f);
    else result = (s32)(float32 + 0.5f);
    
    return(result);
}

inline u32 
RoundFloat32ToUInt32(f32 float32)
{
    u32 result;
    
    if(float32 < 0.0f) s32 result = (s32)(float32 - 0.5f);
    else result = (u32)(float32 + 0.5f);
    
    return(result);
}

inline v2i
RoundFloat32ToInt32(v2f floats)
{
    v2i result{};
    
    if(floats.x < 0.0f) result.x = (s32)(floats.x - 0.5f);
    else result.x = (s32)(floats.x + .5f);
    
    if(floats.y < 0.0f) result.y = (s32)(floats.y - 0.5f);
    else result.y = (s32)(floats.y + .5f);
    
    return result;
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
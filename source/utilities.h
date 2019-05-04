#pragma once

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

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

inline auto Swap(v2f* a, v2f* b) -> void
{
    v2f c { *a };
    *a = *b;
    *b = c;
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

inline v2i
RoundFloat32ToInt32(v2f floats)
{
    v2i result{};

    if(floats.x < 0.0f) result.x = (i32)(floats.x - 0.5f);
    else result.x = (i32)(floats.x + .5f);

    if(floats.y < 0.0f) result.y = (i32)(floats.y - 0.5f);
    else result.y = (i32)(floats.y + .5f);

    return result;
}

#pragma once

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

inline ui32
SafeTruncateUInt64(ui64 Value)
{
    BGZ_ASSERT(Value <= 0xFFFFFFFF, "Make sure there wouldn't be information lost with truncate if value was over 32-bits");
    ui32 Result = (ui32)Value;
    return (Result);
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

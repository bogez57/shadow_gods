#pragma once

#include "shared.h"
#include "types.h"

struct Player
{
    vec2 Position;
    Texture CurrentTexture;
};

struct Level
{
    float32 Width{2048.0f};
    float32 Height{1152.0f};
    Texture BackgroundTexture;
};

struct Camera
{
    vec2 Position;
    float32 ZoomFactor;
};

struct Game_State
{
    Camera GameCamera;
    Level GameLevel;
    Player Fighter;
};

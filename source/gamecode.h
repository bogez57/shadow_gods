#pragma once

#include "shared.h"
#include "types.h"

struct Player
{
    vec2 Position;
};

struct Camera
{
    vec2 Position;
    float32 ZoomFactor;
};

struct Game_State
{
    Camera GameCamera;
    Player Fighter;
    Texture BackgroundTexture;
    Texture FighterTexture;
};

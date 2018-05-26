#pragma once

#include "shared.h"
#include "types.h"

struct Player
{
    vec2 Origin;
};

struct Camera
{
    vec2 Position;
    float32 ZoomFactor;
};

struct Game_State
{
    Player Fighter;
    Camera GameCamera;
    float32 MoveWidth;
    float32 MoveHeight;
    Texture BackgroundTexture;
};

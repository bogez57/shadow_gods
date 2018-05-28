#pragma once

#include "shared.h"
#include "types.h"

struct Player
{
    vec2 LevelPos;
    float32 Width;
    float32 Height;
    Texture CurrentTexture;
};

struct Level
{
    float32 Width;
    float32 Height;
    vec2 CenterPoint;
    Texture BackgroundTexture;
};

struct Camera
{
    vec2 LevelPos;
    float32 ViewWidth;
    float32 ViewHeight;
    float32 ZoomFactor;
};

struct Game_State
{
    Camera GameCamera;
    Level GameLevel;
    Player Fighter;
};

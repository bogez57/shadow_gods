#pragma once

#include "shared.h"
#include "types.h"

struct Rect
{
    float32 Width;
    float32 Height;
};

struct Player
{
    Rect Size;
    vec2 WorldPos;
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
    vec2 LookAt;
    vec2 ViewCenter;
    float32 ViewWidth;
    float32 ViewHeight;
    float32 ZoomFactor;
    vec2 DilatePoint;
};

struct Game_State
{
    Camera GameCamera;
    Level GameLevel;
    Player Fighter1;
    Player Fighter2;
};


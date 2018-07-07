#pragma once

#include "shared.h"
#include "types.h"

struct Memory_Chunk
{
    uint64* BaseAddress;
    uint64 Size;
    uint64 UsedMemory;
};

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
    float32 Scale;
    float32 DegreeOfRotation;
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
    vec2 DilatePoint;
    float32 ZoomFactor;
};

struct Game_State
{
    Memory_Chunk TestChunk;
    Camera GameCamera;
    Level GameLevel;
    Player Fighter1;
    Player Fighter2;
};


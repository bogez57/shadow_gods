#pragma once

#include "shared.h"
#include "types.h"

struct Memory_Chunk
{
    uint64* BaseAddress;
    uint64 Size;
    uint64 UsedMemory;
};

struct Limb
{
    Image DisplayImage;
    Texture CurrentTexture;
    Dimensions Size;
    vec2 WorldPos;
};

struct Player
{
    Dimensions Size;
    vec2 WorldPos;
    float32 Scale;
    float32 DegreeOfRotation;
    Limb* Limbs[4];
};

struct Level
{
    Dimensions Size;
    vec2 CenterPoint;
    Image DisplayImage;
    Texture CurrentTexture;
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
    Limb Head;
    Limb Torso;
    Limb LeftThigh;
    Limb RightThigh;
    Player Fighter1;
};


#pragma once

#include "shared.h"
#include "atomic_types.h"

struct Memory_Chunk
{
    ui64* BaseAddress;
    ui64 Size;
    ui64 UsedMemory;
};

struct Local_Transform
{
    f32 Rotation;
    f32 Scale;
};

struct Limb
{
    Local_Transform Transform;
    Image DisplayImage;
    Texture CurrentTexture;
    v2f Dimensions;
    v2f Offset;
    Limb* Parent;
    Limb* Child;
    v2f Position;
};

struct Physique 
{
    Local_Transform Transform;
    v2f Dimensions;

    union
    {
        Limb Limbs[4];
        struct
        {
            Limb Torso;
            Limb Head;
            Limb LeftThigh;
            Limb RightThigh;
        };
    };
};

struct Player
{
    f32 DegreeOfRotation;
    Physique Body;
    v2f WorldPos;
};

struct Level
{
    v2f Dimensions;
    v2f CenterPoint;
    Image DisplayImage;
    Texture CurrentTexture;
};

struct Camera
{
    v2f LookAt;
    v2f ViewCenter;
    f32 ViewWidth;
    f32 ViewHeight;
    v2f DilatePoint;
    f32 ZoomFactor;
};

struct Game_State
{
    Memory_Chunk TestChunk;
    Camera GameCamera;
    Level GameLevel;
    Player Fighter1;
};


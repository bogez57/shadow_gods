#pragma once

#include "atlas.h"
#include "shared.h"
#include "skeleton.h"
#include "animation.h"
#include "fighter.h"

struct Transform_v3
{
    v3 translation{};
    v3 rotation{};
    v3 scale{1.0f, 1.0f, 1.0f};
};

struct Fighter3D
{
    s32 id{};
    s32 textureID{};
    Geometry mesh{};
    Transform_v3 worldTransform{};
};

struct Game_Camera
{
    v2 lookAt{};
    v2 dilatePoint_inScreenCoords{};
    v2 dilatePointOffset_normalized{};
    f32 zoomFactor{};
};

struct Stage_Data
{
    Image backgroundImg;
    v2 size{};
    v2 centerPoint{};
    Fighter player;
    Fighter enemy;
    Fighter enemy2;
    Game_Camera camera;
};

struct Game_State
{
    Fighter3D fighter0{};
    Fighter3D fighter1{};
    Camera3D camera{};
    Image composite;
    Image normalMap;
    Atlas* atlas;
    f32 lightAngle{};
    f32 lightThreshold{};
    Stage_Data stage;
    bool isLevelOver{false};
};

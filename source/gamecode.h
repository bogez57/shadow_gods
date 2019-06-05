#pragma once

#include "atlas.h"
#include "shared.h"
#include "collisions.h"
#include "skeleton.h"

struct Transform
{
    f32 rotation;
    v2f pos;
    v2f scale;
};

struct Fighter
{
    Image image;
    Transform world;
    f32 height;
};

struct StageInfo
{
    f32 height;
    v2f centerPoint;
    Image backgroundImg;
};

struct Game_Camera
{
    v2f lookAt;
    v2f dilatePoint;
    f32 zoomFactor;
};

struct Stage_Data
{
    StageInfo info;
    Fighter player;
    Fighter enemy;
    Fighter enemy2;
    Game_Camera camera;
};

struct Game_State
{
    Image composite;
    Image normalMap;
    f32 lightAngle;
    f32 lightThreshold;
    Stage_Data stage;
};

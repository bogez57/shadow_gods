#pragma once

#include "atlas.h"
#include "shared.h"
#include "collisions.h"
#include "skeleton.h"

struct Image
{
    ui8* data;
    v2i size;
    ui32 pitch;
    f32 opacity {1.0f};
};

struct Fighter
{
    Image image;
    Transform world;
};

struct StageInfo
{
    v2f size;
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
    i32 imageWidth, imageHeight;
    Atlas* atlas;
    Image composite;
    Image normalMap;
    f32 lightAngle;
    f32 lightThreshold;
    Stage_Data stage;
};

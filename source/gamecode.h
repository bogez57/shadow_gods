#pragma once

#include "atlas.h"
#include "shared.h"
#include "collisions.h"
#include "skeleton.h"

struct Coordinate_Space
{
    v2f origin;
    v2f xBasis;
    v2f yBasis;
};

struct Fighter
{
    Image image;
    f32 rotationAngle;
    f32 scale;
    v2f worldPos;
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
    v2f viewCenter;
    f32 viewWidth;
    f32 viewHeight;
    v2f dilatePoint;
    f32 zoomFactor;
};

struct Stage_Data
{
    StageInfo info;
    Fighter player;
    Fighter enemy;
    Game_Camera camera;
};

struct Game_State
{
    i32 imageWidth{}, imageHeight{};
    Atlas* atlas{};
    Image image{};
    Stage_Data stage{};
};

#pragma once

#include "atlas.h"
#include "shared.h"
#include "collisions.h"
#include "skeleton.h"

struct Fighter
{
    Skeleton skel;
    v2f* worldPos;
};

struct StageInfo
{
    v2f size;
    v2f centerPoint;
    Image displayImage;
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
    Game_Camera camera;
};

struct Game_State
{
    i32 imageWidth{}, imageHeight{};
    v2i targetPos3 {};
    Image image{};
    Image backgroundImg{};
    Image torso{};
    Stage_Data stage{};
};

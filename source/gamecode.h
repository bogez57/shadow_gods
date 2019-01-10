#pragma once

#include "atomic_types.h"
#include "atlas.h"
#include "shared.h"
#include "memory_handling.h"
#include "collisions.h"
#include "skeleton.h"

struct Fighter
{
    Skeleton skel;
};

struct StageInfo
{
    v2f size;
    v2f centerPoint;
    Image displayImage;
    Texture currentTexture;
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
    Fighter* player;
    Game_Camera camera;
};

struct Game_State
{
    Stage_Data stage;
    Atlas* atlas;
};

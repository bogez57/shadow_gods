#pragma once

#include "atlas.h"
#include "shared.h"
#include "collisions.h"
#include "skeleton.h"
#include "animation.h"

struct Fighter
{
    v2f* worldPos;
    Transform world;
    f32 height;
    Skeleton skel;
    Animation anim;
    AnimationQueue animQueue;
    AnimationData animData;
};

struct Game_Camera
{
    v2f lookAt;
    v2f dilatePoint_inScreenDims;
    f32 zoomFactor;
};

struct Stage_Data
{
    Image backgroundImg;
    v2f size;
    v2f centerPoint;
    Fighter player;
    Fighter enemy;
    Fighter enemy2;
    Game_Camera camera;
};

struct Game_State
{
    Image composite;
    Image normalMap;
    Atlas* atlas;
    f32 lightAngle;
    f32 lightThreshold;
    Stage_Data stage;
};

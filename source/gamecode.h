#pragma once

#include "atlas.h"
#include "shared.h"
#include "skeleton.h"
#include "animation.h"
#include "fighter.h"
#include "renderer_stuff.h"

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
};

struct Game_State
{
    Image composite;
    Image normalMap;
    Atlas* atlas;
    f32 lightAngle{};
    f32 lightThreshold{};
    Stage_Data stage;
    b isLevelOver{false};
    Image openGLRenderTest{};
    Cube cube{};
};

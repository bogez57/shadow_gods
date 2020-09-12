#pragma once

#include "atlas.h"
#include "shared.h"
#include "skeleton.h"
#include "animation.h"
#include "fighter.h"

struct Game_Camera
{
    v2f lookAt{};
    v2f dilatePoint_inScreenCoords{};
    v2f dilatePointOffset_normalized{};
    f32 zoomFactor{};
    f32 zPosition{};
};

struct Stage_Data
{
    Image backgroundImg;
    v2f size{};
    v2f centerPoint{};
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
    f32 lightAngle{};
    f32 lightThreshold{};
    Stage_Data stage;
    bool isLevelOver{false};
    Image openGLRenderTest{};
};

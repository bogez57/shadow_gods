#pragma once

#include "atomic_types.h"
#include "shared.h"
#include "memory_handling.h"

struct AABB
{
    v2f minCorner;
    v2f maxCorner;
};

struct Collision_Box
{
    AABB bounds;
    v2f centerPoint;
    v2f size;
};

Collision_Box UpdateCollisionBoxBasedOnCenterPoint(Collision_Box oldCollisionBox, v2f newCenterPosition)
{
    Collision_Box newCollisionBox { oldCollisionBox };

    newCollisionBox.bounds.minCorner.x = newCenterPosition.x - oldCollisionBox.size.x;
    newCollisionBox.bounds.minCorner.y = newCenterPosition.y;
    newCollisionBox.bounds.maxCorner.x = newCenterPosition.x + oldCollisionBox.size.x;
    newCollisionBox.bounds.maxCorner.y = newCenterPosition.y + oldCollisionBox.size.y;

    newCollisionBox.centerPoint = newCenterPosition;

    return newCollisionBox;
}

void InitCollisionBox_1(Collision_Box* collisionBox, v2f centerPoint, v2f size)
{
    collisionBox->size = size;

    *collisionBox = UpdateCollisionBoxBasedOnCenterPoint(*collisionBox, centerPoint);
};

struct Fighter
{
    v2f worldPos;
    v2f prevFrameWorldPos;
    Collision_Box hitBox;
    Collision_Box hurtBox;
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
    Fighter player;
    Fighter ai;
    Game_Camera camera;
};

struct Game_State
{
    Stage_Data stage;
};

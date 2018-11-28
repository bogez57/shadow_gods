#pragma once

#include <spine/AnimationState.h>
#include <spine/AnimationStateData.h>
#include <spine/Atlas.h>
#include <spine/Skeleton.h>
#include <spine/SkeletonData.h>
#include <spine/SkeletonJson.h>

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
    spSkeleton* skeleton;
    spAnimationState* animationState;
    v2f worldPos;
    v2f prevFrameWorldPos;
};

void InitFighter_1(Fighter* fighter, v2f worldPos, spSkeleton* spineSkeleton, spAnimationState* spineAnimState, v2f scale)
{
    fighter->skeleton = spineSkeleton;
    fighter->animationState = spineAnimState;

    fighter->worldPos = worldPos;
    fighter->skeleton->x = fighter->worldPos.x;
    fighter->skeleton->y = fighter->worldPos.y;
    fighter->skeleton->scaleX = scale.x;
    fighter->skeleton->scaleY = scale.y;
};

void ApplyAnimationStateToSkeleton_2(spSkeleton* skeleton, spAnimationState* animState, v2f newWorldPos)
{
    // Needed for spine to correctly update bones
    skeleton->x = newWorldPos.x;
    skeleton->y = newWorldPos.y;
    spAnimationState_apply(animState, skeleton);
};

void TransformSkeletonToWorldSpace_1(spSkeleton* skeleton)
{
    spSkeleton_updateWorldTransform(skeleton);
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
    spSkeletonData* commonSkeletonData;
    spAnimationStateData* commonAnimationData;
    Fighter player;
    Fighter ai;
    Game_Camera camera;
};

struct Game_State
{
    Stage_Data stage;
    void (*SpineFuncPtrTest)(const spTimeline* timeline, spSkeleton* skeleton, float lastTime, float time, spEvent** firedEvents, int* eventsCount, float alpha, spMixBlend blend, spMixDirection direction);
    spAnimation* emptyAnim; //For dll reloading purposes only
    Memory_Handler memHandler;
};

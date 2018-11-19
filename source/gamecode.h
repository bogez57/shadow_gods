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
    Collision_Box newCollisionBox {};

    newCollisionBox.bounds.minCorner.x = newCenterPosition.x - oldCollisionBox.size.x;
    newCollisionBox.bounds.minCorner.y = newCenterPosition.y;
    newCollisionBox.bounds.maxCorner.x = newCenterPosition.x + oldCollisionBox.size.x;
    newCollisionBox.bounds.maxCorner.y = newCenterPosition.y + oldCollisionBox.size.y;

    { //Calculate new center
        newCollisionBox.centerPoint.x = ((newCollisionBox.bounds.minCorner.x + newCollisionBox.bounds.maxCorner.x) / 2);
        newCollisionBox.centerPoint.y = ((newCollisionBox.bounds.maxCorner.y + newCollisionBox.bounds.maxCorner.y) / 2);
    };

    return newCollisionBox;
}

struct Animation
{
    spAnimation* baseAnimation;
    v2f hitBoxCenter;
};

struct Fighter
{
    spSkeleton* skeleton;
    spAnimationState* animationState;
    v2f worldPos;
    v2f prevFrameWorldPos;
    Collision_Box hurtBox;
    Collision_Box hitBox;

    void Init(v2f worldPos, spSkeleton* spineSkeleton, spAnimationState* spineAnimState, v2f scale, v2f mainHurtBoxSize)
    {
        this->skeleton = spineSkeleton;
        this->animationState = spineAnimState;

        this->worldPos = worldPos;
        this->skeleton->x = this->worldPos.x;
        this->skeleton->y = this->worldPos.y;
        this->skeleton->scaleX = scale.x;
        this->skeleton->scaleY = scale.y;

        this->hurtBox.size = mainHurtBoxSize;
        this->hurtBox = UpdateCollisionBoxBasedOnCenterPoint(this->hurtBox, this->worldPos);
    };
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

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
    v2f  centerPoint;
    v2f  size;
};

struct Fighter
{
    spSkeleton*       skeleton;
    spAnimationState* animationState;
    v2f               worldPos;
    v2f               prevFrameWorldPos;
    Collision_Box     hurtBox;
};

struct StageInfo
{
    v2f     size;
    v2f     centerPoint;
    Image   displayImage;
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
    StageInfo             info {};
    spSkeletonData*       commonSkeletonData { nullptr };
    spAnimationStateData* commonAnimationData { nullptr };
    Fighter               player {};
    Fighter               ai {};
    Game_Camera           camera;
};

struct Game_State
{
    Stage_Data stage {};
    void (*SpineFuncPtrTest)(const spTimeline* timeline, spSkeleton* skeleton, float lastTime, float time, spEvent** firedEvents, int* eventsCount, float alpha, spMixBlend blend, spMixDirection direction);
    spAnimation*   emptyAnim;
    Memory_Handler memHandler;
};

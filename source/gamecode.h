#pragma once

#include "shared.h"
#include "atomic_types.h"
#include "memory_handling.h"
#include <spine/Skeleton.h>
#include <spine/Atlas.h>
#include <spine/SkeletonData.h>
#include <spine/SkeletonJson.h>
#include <spine/AnimationState.h>
#include <spine/AnimationStateData.h>

struct Fighter 
{
    spSkeleton* skeleton;
    spAnimationState* animationState;
    v2f worldPos;
};

struct StageInfo
{
    v2f size;
    v2f centerPoint;
    Image displayImage;
    Texture currentTexture;
};

struct Camera
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
    StageInfo info{};
    spSkeletonData* commonSkeletonData{nullptr};
    spAnimationStateData* commonAnimationData{nullptr};
    Fighter player{};
    Fighter ai{};
    Camera gameCamera;
};

struct Game_State
{
    Stage_Data stage{};
    void(*SpineFuncPtrTest)(const spTimeline* timeline, spSkeleton* skeleton, float lastTime, float time, spEvent** firedEvents, int* eventsCount, float alpha, spMixBlend blend, spMixDirection direction);
    spAnimation* emptyAnim;
    Memory_Handler memHandler;
};

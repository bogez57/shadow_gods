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

struct Level
{
    v2f Dimensions;
    v2f CenterPoint;
    Image DisplayImage;
    Texture CurrentTexture;
};

struct Camera
{
    v2f LookAt;
    v2f ViewCenter;
    f32 ViewWidth;
    f32 ViewHeight;
    v2f DilatePoint;
    f32 ZoomFactor;
};

struct Game_State
{
    Camera GameCamera;
    Level GameLevel;
    spSkeletonJson* SkelJson{nullptr};
    spSkeletonData* SkelData{nullptr};
    spAtlas* Atlas{nullptr};
    Fighter player{};
    Fighter ai{};
    spAnimationStateData* AnimationStateData;
    spAnimation* Animation;
    spTrackEntry* entry;
    void(*SpineFuncPtrTest)(const spTimeline* timeline, spSkeleton* skeleton, float lastTime, float time, spEvent** firedEvents, int* eventsCount, float alpha, spMixBlend blend, spMixDirection direction);
    spAnimation* EmptyAnim;
    Memory_Region MemRegions[REGION_COUNT];
    Dynamic_Mem_Allocator DynamAllocator;
};

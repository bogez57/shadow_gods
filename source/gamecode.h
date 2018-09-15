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
    spSkeleton* MySkeleton;
    spAnimationStateData* AnimationStateData;
    spAnimationState* AnimationState;
    spAnimation* Animation;
    spTrackEntry* entry;
    void(*SpineFuncPtrTest)(const spTimeline* timeline, spSkeleton* skeleton, float lastTime, float time, spEvent** firedEvents, int* eventsCount, float alpha, spMixBlend blend, spMixDirection direction);
    spAnimation* EmptyAnim;
    Dynamic_Mem_Allocator DynamAllocator;
};

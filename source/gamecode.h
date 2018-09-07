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

struct Local_Transform
{
    f32 Rotation;
    f32 Scale;
};

struct Limb
{
    Local_Transform Transform;
    Image DisplayImage;
    Texture CurrentTexture;
    v2f Dimensions;
    v2f OffsetFromParent;
    Limb* Parent;
    Limb* Child;
    v2f Position;
};

struct Physique 
{
    Local_Transform Transform;
    v2f Dimensions;

    union
    {
        Limb Limbs[10];
        struct
        {
            Limb Root;
            Limb Torso;
            Limb Neck;
            Limb Head;
            Limb LeftThigh;
            Limb RightThigh;
            Limb RightShoulder;
            Limb LeftShoulder;
            Limb RightArm;
            Limb LeftArm;
        };
    };
};

struct Player
{
    f32 DegreeOfRotation;
    Physique Body;
    v2f WorldPos;
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
    Player Fighter1;
    spSkeletonJson* SkelJson{nullptr};
    spSkeletonData* SkelData{nullptr};
    spAtlas* Atlas{nullptr};
    spSkeleton* MySkeleton;
    spAnimationStateData* AnimationStateData;
    spAnimationState* AnimationState;
    spAnimation* Animation;
    spTrackEntry* entry;
    Dynamic_Mem_Allocator DynamAllocator;
};

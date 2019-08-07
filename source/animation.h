#ifndef ANIMATION_INCLUDE_H
#define ANIMATION_INCLUDE_H

#include <string.h>
#include "skeleton.h"
#include "json.h"

/*
    API ideas:
    1. QueueAnimaiton() and PlayAnimationImmediately() funcitons
    2. Each Json object under rotate, translate, scale is a keyframe so call it that? 
    3. Should you call rotate, translate, scale timelines? 
    4. Organize data in more of a SOA fashion?
*/

struct KeyFrame
{
    f32 time;
    f32 angle;
    char* curve;
};

struct Timeline
{
    Dynam_Array<KeyFrame> keyFrames;
};

struct TimelineSet
{
    Timeline rotationTimeline;
    Timeline translationTimeline;
    Timeline scaleTimeline;
};

struct Animation
{
    const char* name;
    f32 time;
    i32 count;
    HashMap_Str<TimelineSet> timelineSets;
    b startAnimation{false};
};

struct AnimationQueue
{

};


void SetToSetupPose(Skeleton&& skel, Animation anim);
void CreateAnimationFromJsonFile(Animation&& anim, const char* jsonFilePath);
void UpdateSkeletonAnimation(Skeleton&& skel, Animation&& anim, f32 prevFrameDT);

#endif

#ifdef ANIMATION_IMPL

void SetToSetupPose(Skeleton&& skel, Animation anim)
{
    for(i32 boneIndex{}; boneIndex < skel.bones.size; ++boneIndex)
    {
        i32 hashIndex = GetHashIndex(anim.timelineSets, skel.bones.At(boneIndex).name);

        if(hashIndex != -1)
        {
            TimelineSet timelineSet =  GetVal(anim.timelineSets, hashIndex);
            Timeline rotationTimeline = timelineSet.rotationTimeline;

            *skel.bones.At(boneIndex).parentLocalRotation = skel.bones.At(boneIndex).originalParentLocalRotation + rotationTimeline.keyFrames.At(0).angle;
        };
    };    
};

void CreateAnimationFromJsonFile(Animation&& anim, const char* jsonFilePath)
{
    i32 length;

    const char* jsonFile = globalPlatformServices->ReadEntireFile($(length), jsonFilePath);

    Json* root {};
    root = Json_create(jsonFile);

    Init($(anim.timelineSets));

    Json* animations = Json_getItem(root, "animations"); /* clang-format off */BGZ_ASSERT(animations, "Unable to return valid json object!"); /* clang-format on */
    Json* currentAnimation = animations->child;

    anim.name = currentAnimation->name;

        Json* bonesOfAnimation = currentAnimation->child; i32 boneIndex{};
        for(Json* currentBone = bonesOfAnimation ? bonesOfAnimation->child : 0; currentBone; currentBone = currentBone->next, ++boneIndex)
        {
            Json* rotateTimeline_json = Json_getItem(currentBone, "rotate");

            if(rotateTimeline_json)
            {
                i32 keyFrameIndex{}; Timeline rotationTimeline{};
                rotationTimeline.keyFrames.Init(rotateTimeline_json->size, heap);
                for(Json* jsonKeyFrame = rotateTimeline_json ? rotateTimeline_json->child : 0; jsonKeyFrame; jsonKeyFrame = jsonKeyFrame->next, ++keyFrameIndex)
                {
                    KeyFrame keyFrame;

                    keyFrame.time = Json_getFloat(jsonKeyFrame, "time", 0.0f);
                    keyFrame.angle = Radians(Json_getFloat(jsonKeyFrame, "angle", 0.0f));

                    rotationTimeline.keyFrames.Insert(keyFrame, keyFrameIndex);
                };

                TimelineSet set{rotationTimeline};
                Insert<TimelineSet>($(anim.timelineSets), currentBone->name, set);
            };
        };
};

void StartAnimation(Animation&& anim)
{
    anim.startAnimation = true;
};

#if 0
f32 recursiveTest(f32 originalBoneRotation, Timeline rotationTimeline, Animation&& anim, i32 count, i32 maxCount)
{
    f32 lerpedRotation{0.0f};
    if(count == 0)
    {
        lerpedRotation = originalBoneRotation + rotationTimeline.keyFrames.At(count).angle;
    }
    else if(rotationTimeline.keyFrames.At(count).time <= anim.time)
    {
        f32 rotation0 = originalBoneRotation + rotationTimeline.keyFrames.At(count - 1).angle;
        f32 rotation1 = originalBoneRotation + rotationTimeline.keyFrames.At(count).angle;

        f32 diff = rotationTimeline.keyFrames.At(count).time - rotationTimeline.keyFrames.At(count - 1).time;
        f32 diff1 = anim.time - rotationTimeline.keyFrames.At(count - 1).time;

        f32 t = diff1 / diff;
        lerpedRotation = Lerp(rotation0, rotation1, t);

        if(count == maxCount)
        {
            anim.time = 0.0f;
            anim.startAnimation = false;
        }
    }
    else
    {
        lerpedRotation = recursiveTest(originalBoneRotation, rotationTimeline, $(anim), count - 1, maxCount);
    };

    return lerpedRotation;
};
#endif 

void UpdateSkeletonAnimation(Skeleton&& skel, Animation&& anim, f32 prevFrameDT)
{
    if(anim.startAnimation)
        anim.time += prevFrameDT;

    Bone* bone = FindBone(&skel, "torso");


    i32 hashIndex = GetHashIndex<TimelineSet>(anim.timelineSets, bone->name);
    TimelineSet timelineSet = GetVal<TimelineSet>(anim.timelineSets, hashIndex);
    Timeline rotationTimeline = timelineSet.rotationTimeline;

    f32 rotation0 = bone->originalParentLocalRotation + rotationTimeline.keyFrames.At(0).angle;
    f32 rotation1 = bone->originalParentLocalRotation + rotationTimeline.keyFrames.At(1).angle;
    f32 rotation2 = bone->originalParentLocalRotation + rotationTimeline.keyFrames.At(2).angle;
    f32 rotation3 = bone->originalParentLocalRotation + rotationTimeline.keyFrames.At(3).angle;

    ConvertNegativeToPositiveAngle_Radians($(rotation0));
    ConvertNegativeToPositiveAngle_Radians($(rotation1));
    ConvertNegativeToPositiveAngle_Radians($(rotation2));
    ConvertNegativeToPositiveAngle_Radians($(rotation3));

    local_persist b firstTime;

    if(anim.time > rotationTimeline.keyFrames.At(3).time)
    {
        anim.count = 0;
        anim.time = 0.0f;
        anim.startAnimation = false;
    }

    f32 t{};
    f32 lerpedRotation{bone->originalParentLocalRotation};
    if(rotationTimeline.keyFrames.At(0).time < anim.time)
    {
        f32 diff = rotationTimeline.keyFrames.At(1).time - rotationTimeline.keyFrames.At(0).time;
        f32 diff1 = anim.time - rotationTimeline.keyFrames.At(0).time;

        t = diff1 / diff;

        lerpedRotation = Lerp(rotation0, rotation1, t);
    };

    if(rotationTimeline.keyFrames.At(1).time < anim.time)
    {
        f32 diff = rotationTimeline.keyFrames.At(2).time - rotationTimeline.keyFrames.At(1).time;
        f32 diff1 = anim.time - rotationTimeline.keyFrames.At(1).time;

        t = diff1 / diff;

        lerpedRotation = Lerp(rotation1, rotation2, t);
    };

    if(rotationTimeline.keyFrames.At(2).time < anim.time)
    {
        f32 diff = rotationTimeline.keyFrames.At(3).time - rotationTimeline.keyFrames.At(2).time;
        f32 diff1 = anim.time - rotationTimeline.keyFrames.At(2).time;

        t = diff1 / diff;

        lerpedRotation = Lerp(rotation2, rotation3, t);
    };

    *bone->parentLocalRotation = lerpedRotation;
};

#endif //ANIMATION_IMPL
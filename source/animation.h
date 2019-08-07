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

f32 recursiveTest(f32 originalBoneRotation, Timeline rotationTimeline, Animation&& anim, i32 count)
{
    f32 lerpedRotation{0.0f};
    if(count == 0)
    {
        //do nothing
    }
    else if(rotationTimeline.keyFrames.At(count).time <= anim.time)
    {
        f32 rotation0 = originalBoneRotation + rotationTimeline.keyFrames.At(count - 1).angle;
        f32 rotation1 = originalBoneRotation + rotationTimeline.keyFrames.At(count).angle;

        f32 diff = rotationTimeline.keyFrames.At(count).time - rotationTimeline.keyFrames.At(count - 1).time;
        f32 diff1 = anim.time - rotationTimeline.keyFrames.At(count - 1).time;

        f32 t = diff1 / diff;
        lerpedRotation = Lerp(rotation0, rotation1, t);

        if(count == 3)
        {
            anim.time = 0.0f;
            anim.startAnimation = false;
        }
    }
    else
    {
        lerpedRotation = recursiveTest(originalBoneRotation, rotationTimeline, $(anim), count - 1);
    };

    return lerpedRotation;
};

void UpdateSkeletonAnimation(Skeleton&& skel, Animation&& anim, f32 prevFrameDT)
{
    if(anim.startAnimation)
    {
        anim.time += prevFrameDT;

        for(i32 boneIndex{}; boneIndex < skel.bones.size; ++boneIndex)
        {
            i32 hashIndex = GetHashIndex(anim.timelineSets, skel.bones.At(boneIndex).name);

            if(hashIndex != -1)
            {
                TimelineSet boneTimelineSet = GetVal(anim.timelineSets, hashIndex);
                Timeline rotationTimeline = boneTimelineSet.rotationTimeline;

                f32 lerpedRotation = recursiveTest(skel.bones.At(boneIndex).originalParentLocalRotation, rotationTimeline, $(anim), (i32)rotationTimeline.keyFrames.size - 1);

                *skel.bones.At(boneIndex).parentLocalRotation = lerpedRotation;
            };
        };
    };
};

#endif //ANIMATION_IMPL
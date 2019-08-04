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



void CreateAnimationFromJsonFile(Animation&& anim, const char* jsonFilePath);
void UpdateSkeletonAnimation();

#endif

#ifdef ANIMATION_IMPL

void CreateAnimationFromJsonFile(Animation&& anim, const char* jsonFilePath)
{
    i32 length;

    const char* jsonFile = globalPlatformServices->ReadEntireFile($(length), jsonFilePath);

    Json* root {};
    root = Json_create(jsonFile);

    Init($(anim.timelineSets));

    Json* animations = Json_getItem(root, "animations"); /* clang-format off */BGZ_ASSERT(animations, "Unable to return valid json object!"); /* clang-format on */

    for(Json* currentAnimation = animations ? animations->child : 0; currentAnimation; currentAnimation = currentAnimation->next)
    {
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
};

void StartAnimation(Animation&& anim)
{
    anim.startAnimation = true;
};

void UpdateSkeletonAnimation(Skeleton&& skel, Animation&& anim, f32 prevFrameDT)
{
    if(anim.startAnimation)
        anim.time += prevFrameDT;

    Bone* bone = FindBone(&skel, "right-shoulder");

    TimelineSet timelineSet = Get<TimelineSet>(anim.timelineSets, bone->name);
    Timeline rotationTimeline = timelineSet.rotationTimeline;

    local_persist b firstTime;

    if(rotationTimeline.keyFrames.At(anim.count).time <= anim.time)
    {
        if(rotationTimeline.keyFrames.At(anim.count + 1).time <= anim.time)
        {
            ++anim.count;

            if(anim.count == (rotationTimeline.keyFrames.size - 1))
            {
                anim.count = 0;
                anim.time = 0.0f;
                anim.startAnimation = false;
            };
        };

        f32 rotation0 = bone->originalParentLocalRotation + rotationTimeline.keyFrames.At(anim.count).angle;
        f32 rotation1 = bone->originalParentLocalRotation + rotationTimeline.keyFrames.At(anim.count + 1).angle;

        f32 diff = rotationTimeline.keyFrames.At(anim.count + 1).time - rotationTimeline.keyFrames.At(anim.count).time;
        f32 diff1 = anim.time - rotationTimeline.keyFrames.At(anim.count).time;

        f32 t = diff1 / diff;
        f32 lerpedRotation = Lerp(rotation0, rotation1, t);

        *bone->parentLocalRotation = lerpedRotation;
    };
};

#endif //ANIMATION_IMPL
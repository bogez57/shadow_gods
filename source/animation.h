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
    Json* currentAnimation = Json_getItem(animations, "high_kick");

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

void UpdateSkeletonAnimation(Skeleton&& skel, Animation&& anim, f32 prevFrameDT)
{
    if(anim.startAnimation)
        anim.time += prevFrameDT;

    f32 maxTimeOfAnimation{};
    for(i32 boneIndex{}; boneIndex < skel.bones.size; ++boneIndex)
    {
        i32 hashIndex = GetHashIndex<TimelineSet>(anim.timelineSets, skel.bones.At(boneIndex).name);

        if(hashIndex != -1)
        {
            TimelineSet timelineSet = GetVal<TimelineSet>(anim.timelineSets, hashIndex);
            Timeline rotationTimelineOfBone = timelineSet.rotationTimeline;

            i32 count = (i32)rotationTimelineOfBone.keyFrames.size - 1;

            f32 lerpedRotation{*skel.bones.At(boneIndex).parentLocalRotation};
            while(count)
            {
                if(rotationTimelineOfBone.keyFrames.At(count - 1).time < anim.time && rotationTimelineOfBone.keyFrames.size != 1)
                {
                    if(!strcmp(skel.bones.At(boneIndex).name, "right-leg"))
                    {
                        int x = 3;
                    }

                    f32 rotation0 = skel.bones.At(boneIndex).originalParentLocalRotation + rotationTimelineOfBone.keyFrames.At(count - 1).angle;
                    f32 rotation1 = skel.bones.At(boneIndex).originalParentLocalRotation + rotationTimelineOfBone.keyFrames.At(count).angle;

                   if(rotation0 > (2*PI))
                    {
                        ConvertNegativeToPositiveAngle_Radians($(rotation0));
                    }
                    if(rotation1 > (2*PI))
                    {
                        ConvertNegativeToPositiveAngle_Radians($(rotation1));
                    }

                    f32 diff = rotationTimelineOfBone.keyFrames.At(count).time - rotationTimelineOfBone.keyFrames.At(count - 1).time;
                    f32 diff1 = anim.time - rotationTimelineOfBone.keyFrames.At(count - 1).time;

                    f32 t = diff1 / diff;

                    lerpedRotation = Lerp(rotation0, rotation1, t);

                    count = 0;
                }
                else
                {
                    --count;
                }
            };
            
            f32 currentMaxTime = rotationTimelineOfBone.keyFrames.At(rotationTimelineOfBone.keyFrames.size - 1).time;

            if(currentMaxTime > maxTimeOfAnimation)
                maxTimeOfAnimation = currentMaxTime;

            *skel.bones.At(boneIndex).parentLocalRotation = lerpedRotation;
        };
    };

    if(anim.time > maxTimeOfAnimation)
    {
        anim.time = 0.0f;
        anim.startAnimation = false;
    };
};

#endif //ANIMATION_IMPL
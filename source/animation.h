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
    HashMap_Str<TimelineSet> timelineSets;
    b startAnimation{ false };
};

struct AnimationQueue
{
};

void SetToSetupPose(Skeleton&& skel, Animation anim);
void CreateAnimationFromJsonFile(Animation&& anim, const char* jsonFilePath);
void UpdateAnimationState(Skeleton skel, Animation&& anim, f32 prevFrameDT);

#endif

#ifdef ANIMATION_IMPL

void SetToSetupPose(Skeleton&& skel, Animation anim)
{
    for (i32 boneIndex{}; boneIndex < skel.bones.size; ++boneIndex)
    {
        i32 hashIndex = GetHashIndex(anim.timelineSets, skel.bones.At(boneIndex).name);

        if (hashIndex != -1)
        {
            TimelineSet timelineSet = GetVal(anim.timelineSets, hashIndex, skel.bones.At(boneIndex).name);
            Timeline rotationTimeline = timelineSet.rotationTimeline;

            *skel.bones.At(boneIndex).parentLocalRotation = skel.bones.At(boneIndex).originalParentLocalRotation + rotationTimeline.keyFrames.At(0).angle;
        };
    };
};

void CreateAnimationFromJsonFile(Animation&& anim, const char* jsonFilePath)
{
    i32 length;

    const char* jsonFile = globalPlatformServices->ReadEntireFile($(length), jsonFilePath);

    Json* root{};
    root = Json_create(jsonFile);

    Init($(anim.timelineSets));

    Json* animations = Json_getItem(root, "animations"); /* clang-format off */BGZ_ASSERT(animations, "Unable to return valid json object!"); /* clang-format on */
    Json* currentAnimation = Json_getItem(animations, "high_kick");

    anim.name = currentAnimation->name;

    Json* bonesOfAnimation = currentAnimation->child;
    i32 boneIndex{};
    for (Json* currentBone = bonesOfAnimation ? bonesOfAnimation->child : 0; currentBone; currentBone = currentBone->next, ++boneIndex)
    {
        Json* rotateTimeline_json = Json_getItem(currentBone, "rotate");

        if (rotateTimeline_json)
        {
            i32 keyFrameIndex{};
            Timeline rotationTimeline{};
            rotationTimeline.keyFrames.Init(rotateTimeline_json->size, heap);
            for (Json* jsonKeyFrame = rotateTimeline_json ? rotateTimeline_json->child : 0; jsonKeyFrame; jsonKeyFrame = jsonKeyFrame->next, ++keyFrameIndex)
            {
                KeyFrame keyFrame;

                keyFrame.time = Json_getFloat(jsonKeyFrame, "time", 0.0f);
                keyFrame.angle = Radians(Json_getFloat(jsonKeyFrame, "angle", 0.0f));

                rotationTimeline.keyFrames.Insert(keyFrame, keyFrameIndex);
            };

            TimelineSet set{ rotationTimeline };
            Insert<TimelineSet>($(anim.timelineSets), currentBone->name, set);
        };
    };
};

void StartAnimation(Animation&& anim)
{
    anim.startAnimation = true;
};

//Do not handle 'animation clean up' option in spine which, when it is checked in spine, doesn't export
//duplicate keys. This function needs to know every key from animation in order to work properly
void UpdateAnimationState(Skeleton skel, Animation&& anim, f32 prevFrameDT)
{
    if (anim.startAnimation)
        anim.time += prevFrameDT;

    f32 maxTimeOfAnimation{};
    for (i32 boneIndex{}; boneIndex < skel.bones.size; ++boneIndex)
    {
        i32 hashIndex = GetHashIndex<TimelineSet>(anim.timelineSets, skel.bones.At(boneIndex).name);

        if (hashIndex != -1)
        {
            TimelineSet timelineSet = GetVal<TimelineSet>(anim.timelineSets, hashIndex, skel.bones.At(boneIndex).name);
            Timeline rotationTimelineOfBone = timelineSet.rotationTimeline;
            Bone* bone = &skel.bones.At(boneIndex);

            i32 keyFrameCount = (i32)rotationTimelineOfBone.keyFrames.size - 1;

            f32 lerpedRotation{ bone->originalParentLocalRotation + rotationTimelineOfBone.keyFrames.At(0).angle };
            while (keyFrameCount)
            {
                KeyFrame keyFrame0 = rotationTimelineOfBone.keyFrames.At(keyFrameCount - 1);
                KeyFrame keyFrame1 = rotationTimelineOfBone.keyFrames.At(keyFrameCount);

                if (keyFrame0.time < anim.time && keyFrame1.time > anim.time && rotationTimelineOfBone.keyFrames.size != 1)
                {
                    f32 rotationAngle0 = bone->originalParentLocalRotation + keyFrame0.angle;
                    f32 rotationAngle1 = bone->originalParentLocalRotation + keyFrame1.angle;

                    ConvertNegativeToPositiveAngle_Radians($(rotationAngle0));
                    ConvertNegativeToPositiveAngle_Radians($(rotationAngle1));

                    //Find percent to lerp
                    f32 diff = keyFrame1.time - keyFrame0.time;
                    f32 diff1 = anim.time - keyFrame0.time;
                    f32 percentToLerp = diff1 / diff;

                    v2f boneVector_frame0 = {bone->length * CosR(rotationAngle0), bone->length * SinR(rotationAngle0)};
                    v2f boneVector_frame1 = {bone->length * CosR(rotationAngle1), bone->length * SinR(rotationAngle1)};
                    f32 directionOfRotation = CrossProduct(boneVector_frame0, boneVector_frame1);

                    if (directionOfRotation > 0) //Rotate counter-clockwise
                    {
                        if (rotationAngle0 < rotationAngle1)
                        {
                            lerpedRotation = Lerp(rotationAngle0, rotationAngle1, percentToLerp);
                        }
                        else
                        {
                            ConvertPositiveToNegativeAngle_Radians($(rotationAngle0));
                            lerpedRotation = Lerp(rotationAngle0, rotationAngle1, percentToLerp);
                        }
                    }
                    else //Rotate clockwise
                    {
                        if (rotationAngle0 < rotationAngle1)
                        {
                            ConvertPositiveToNegativeAngle_Radians($(rotationAngle1));
                            lerpedRotation = Lerp(rotationAngle0, rotationAngle1, percentToLerp);
                        }
                        else
                        {
                            lerpedRotation = Lerp(rotationAngle0, rotationAngle1, percentToLerp);
                        }
                    }

                    keyFrameCount = 0;
                }
                else
                {
                    --keyFrameCount;
                }
            };

            f32 currentMaxTime = rotationTimelineOfBone.keyFrames.At(rotationTimelineOfBone.keyFrames.size - 1).time;

            if (currentMaxTime > maxTimeOfAnimation)
                maxTimeOfAnimation = currentMaxTime;

            *bone->parentLocalRotation = lerpedRotation;
        };
    };

    if (anim.time > maxTimeOfAnimation)
    {
        anim.time = 0.0f;
        anim.startAnimation = false;
    };
};

#endif //ANIMATION_IMPL
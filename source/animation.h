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
    v2f translation;
    char* curve;
};

struct Timeline
{
    b exists{false};
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
    HashMap_Str<TimelineSet> boneTimelineSets;
    HashMap_Str<f32> boneRotations;
    HashMap_Str<v2f> boneTranslations;
    b startAnimation{ false };
};

struct AnimationQueue
{
};

void SetToSetupPose(Skeleton&& skel, Animation anim);
void CreateAnimationFromJsonFile(Animation&& anim, const char* jsonFilePath);
void UpdateAnimationState(Animation&& anim, Dynam_Array<Bone>* bones, f32 prevFrameDT);

#endif

#ifdef ANIMATION_IMPL

void CreateAnimationFromJsonFile(Animation&& anim, const char* jsonFilePath)
{
    i32 length;

    Init($(anim.boneTimelineSets));
    Init($(anim.boneRotations));
    Init($(anim.boneTranslations));

    const char* jsonFile = globalPlatformServices->ReadEntireFile($(length), jsonFilePath);

    Json* root{};
    root = Json_create(jsonFile);
    Json* animations = Json_getItem(root, "animations"); /* clang-format off */BGZ_ASSERT(animations, "Unable to return valid json object!"); /* clang-format on */
    Json* currentAnimation = Json_getItem(animations, "right_cross");

    anim.name = currentAnimation->name;

    Json* bonesOfAnimation = currentAnimation->child;
    i32 boneIndex{};
    for (Json* currentBone = bonesOfAnimation ? bonesOfAnimation->child : 0; currentBone; currentBone = currentBone->next, ++boneIndex)
    {
        Json* rotateTimeline_json = Json_getItem(currentBone, "rotate");
        Json* translateTimeline_json = Json_getItem(currentBone, "translate");
        TimelineSet timeLineSet{};

        if (rotateTimeline_json)
        {
            i32 keyFrameIndex{};
            Timeline rotationTimeline{};
            rotationTimeline.keyFrames.Init(rotateTimeline_json->size, heap);
            rotationTimeline.exists = true;
            for (Json* jsonKeyFrame = rotateTimeline_json ? rotateTimeline_json->child : 0; jsonKeyFrame; jsonKeyFrame = jsonKeyFrame->next, ++keyFrameIndex)
            {
                KeyFrame keyFrame;

                keyFrame.time = Json_getFloat(jsonKeyFrame, "time", 0.0f);
                keyFrame.angle = Radians(Json_getFloat(jsonKeyFrame, "angle", 0.0f));

                rotationTimeline.keyFrames.Insert(keyFrame, keyFrameIndex);
            };

            timeLineSet.rotationTimeline = rotationTimeline;
        };

        if (translateTimeline_json)
        {
            i32 keyFrameIndex{};
            Timeline translateTimeline{};
            translateTimeline.keyFrames.Init(translateTimeline_json->size, heap);
            translateTimeline.exists = true;
            for (Json* jsonKeyFrame = translateTimeline_json ? translateTimeline_json->child : 0; jsonKeyFrame; jsonKeyFrame = jsonKeyFrame->next, ++keyFrameIndex)
            {
                KeyFrame keyFrame;

                f32 pixelsPerMeter{100.0f};
                keyFrame.time = Json_getFloat(jsonKeyFrame, "time", 0.0f);
                keyFrame.translation.x = Json_getFloat(jsonKeyFrame, "x", 0.0f) / pixelsPerMeter;
                keyFrame.translation.y = Json_getFloat(jsonKeyFrame, "y", 0.0f) / pixelsPerMeter;

                translateTimeline.keyFrames.Insert(keyFrame, keyFrameIndex);
            };

            timeLineSet.translationTimeline = translateTimeline;
        };

        Insert<TimelineSet>($(anim.boneTimelineSets), currentBone->name, timeLineSet);
    };
};

void SetToSetupPose(Skeleton&& skel, Animation anim)
{
    for (i32 boneIndex{}; boneIndex < skel.bones.size; ++boneIndex)
    {
        i32 hashIndex = GetHashIndex(anim.boneTimelineSets, skel.bones.At(boneIndex).name);

        if (hashIndex != HASH_DOES_NOT_EXIST)
        {
            TimelineSet timelineSet = GetVal(anim.boneTimelineSets, hashIndex, skel.bones.At(boneIndex).name);
            Timeline rotationTimeline = timelineSet.rotationTimeline;
            Timeline translateTimeline = timelineSet.translationTimeline;

            if(rotationTimeline.exists)
                *skel.bones.At(boneIndex).parentLocalRotation = skel.bones.At(boneIndex).originalParentLocalRotation + rotationTimeline.keyFrames.At(0).angle;
            if(translateTimeline.exists)
                *skel.bones.At(boneIndex).parentLocalPos += translateTimeline.keyFrames.At(0).translation;
        };
    };
};

void StartAnimation(Animation&& anim)
{
    anim.startAnimation = true;
};

void UpdateAnimationState(Animation&& anim, Dynam_Array<Bone>* bones, f32 prevFrameDT)
{
    if (anim.startAnimation)
        anim.time += prevFrameDT;

    f32 maxTimeOfAnimation{};
    for (i32 boneIndex{}; boneIndex < bones->size; ++boneIndex)
    {
        i32 hashIndex = GetHashIndex<TimelineSet>(anim.boneTimelineSets, bones->At(boneIndex).name);

        if (hashIndex != HASH_DOES_NOT_EXIST)
        {
            Bone* bone = &bones->At(boneIndex);

            TimelineSet timelineSet = GetVal<TimelineSet>(anim.boneTimelineSets, hashIndex, bones->At(boneIndex).name);

            Timeline rotationTimelineOfBone = timelineSet.rotationTimeline;
            if(rotationTimelineOfBone.exists)
            {
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

                        v2f boneVector_frame0 = { bone->length * CosR(rotationAngle0), bone->length * SinR(rotationAngle0) };
                        v2f boneVector_frame1 = { bone->length * CosR(rotationAngle1), bone->length * SinR(rotationAngle1) };
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

                Insert<f32>($(anim.boneRotations), bone->name, lerpedRotation);
            };

            Timeline translationTimeLineOfBone = timelineSet.translationTimeline;
            if(translationTimeLineOfBone.exists)
            {
                i32 keyFrameCount = (i32)translationTimeLineOfBone.keyFrames.size - 1;

                v2f newTranslation{ bone->originalParentLocalPos + translationTimeLineOfBone.keyFrames.At(0).translation };
                while(keyFrameCount)
                {
                    KeyFrame keyFrame0 = translationTimeLineOfBone.keyFrames.At(keyFrameCount - 1);
                    KeyFrame keyFrame1 = translationTimeLineOfBone.keyFrames.At(keyFrameCount);

                    if (keyFrame0.time < anim.time && keyFrame1.time > anim.time && translationTimeLineOfBone.keyFrames.size != 1)                        
                    {
                        v2f translation0 = bone->originalParentLocalPos + keyFrame0.translation;
                        v2f translation1 = bone->originalParentLocalPos + keyFrame1.translation;

                        //Find percent to lerp
                        f32 diff = keyFrame1.time - keyFrame0.time;
                        f32 diff1 = anim.time - keyFrame0.time;
                        f32 percentToLerp = diff1 / diff;

                        newTranslation = Lerp(translation0, translation1, percentToLerp);

                        keyFrameCount = 0;
                    }
                    else
                    {
                        --keyFrameCount;
                    }
                };

                f32 currentMaxTime = translationTimeLineOfBone.keyFrames.At(translationTimeLineOfBone.keyFrames.size - 1).time;

                if (currentMaxTime > maxTimeOfAnimation)
                    maxTimeOfAnimation = currentMaxTime;

                Insert<v2f>($(anim.boneTranslations), bone->name, newTranslation);
            };
        };
    };

    if (anim.time > maxTimeOfAnimation)
    {
        anim.time = 0.0f;
        anim.startAnimation = false;
    };
};

void ApplyAnimationToSkeleton(Skeleton&& skel, Animation anim)
{
    for (i32 boneIndex{}; boneIndex < skel.bones.size; ++boneIndex)
    {
        i32 hashIndex = GetHashIndex(anim.boneRotations, skel.bones.At(boneIndex).name);

        if (hashIndex != HASH_DOES_NOT_EXIST)
        {
            f32 newBoneRotation = GetVal(anim.boneRotations, hashIndex, skel.bones.At(boneIndex).name);
            *skel.bones.At(boneIndex).parentLocalRotation = newBoneRotation;
        };

        hashIndex = GetHashIndex(anim.boneTranslations, skel.bones.At(boneIndex).name);

        if (hashIndex != HASH_DOES_NOT_EXIST)
        {
            v2f newBoneTranslation = GetVal(anim.boneTranslations, hashIndex, skel.bones.At(boneIndex).name);
            *skel.bones.At(boneIndex).parentLocalPos = newBoneTranslation;
        };
    };
};

#endif //ANIMATION_IMPL
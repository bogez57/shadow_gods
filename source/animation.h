#ifndef ANIMATION_INCLUDE
#define ANIMATION_INCLUDE

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
    f32 time{};
    f32 angle{};
    v2f translation{};
    char* curve{nullptr};
};

struct Timeline
{
    Timeline() = default;
    Timeline(Init) :
        keyFrames{0, heap}
    {}

    b exists{false};
    Dynam_Array<KeyFrame> keyFrames;
};

struct TimelineSet
{
    TimelineSet() = default;
    TimelineSet(Init init) :
        rotationTimeline{init},
        translationTimeline{init},
        scaleTimeline{init}
    {};

    Timeline rotationTimeline;
    Timeline translationTimeline;
    Timeline scaleTimeline;
};

enum class PlayBackStatus
{
    DEFAULT,
    IDLE,
    IMMEDIATE,
    NEXT
};

struct Animation
{
    Animation() = default;
    Animation(Init) :
        boneTimelineSets{heap},
        boneRotations{heap},
        boneTranslations{heap},
        startAnimation{false}
    {};

    const char* name{nullptr};
    f32 totalTime{};
    f32 currentTime{};
    HashMap_Str<TimelineSet> boneTimelineSets; 
    HashMap_Str<f32> boneRotations;
    HashMap_Str<v2f> boneTranslations;
    b startAnimation{false};
    PlayBackStatus status{PlayBackStatus::DEFAULT};
};

struct AnimationData
{
    AnimationData() = default;
    AnimationData(const char* animDataJsonFilePath);

    HashMap_Str<Animation> animations;
};

struct AnimationQueue
{
    AnimationQueue() = default;
    AnimationQueue(Init) :
        queuedAnimations{20, heap},
        idleAnim{Init::_}
    {}

    Ring_Buffer<Animation> queuedAnimations;
    b hasIdleAnim{false};
    Animation idleAnim;
};

void SetToSetupPose(Skeleton&& skel, Animation anim);
void SetIdleAnimation(AnimationQueue&& animQueue, const AnimationData animData, const char* animName);
void CreateAnimationsFromJsonFile(AnimationData&& animData, const char* jsonFilePath);
void UpdateAnimationState(AnimationQueue&& animQueue, Dynam_Array<Bone>* bones, f32 prevFrameDT);
void QueueAnimation(AnimationQueue&& animQueue, const AnimationData animData, const char* animName, PlayBackStatus status);

#endif

#ifdef ANIMATION_IMPL

AnimationData::AnimationData(const char* animJsonFilePath) : animations{heap}
{
    i32 length;

    const char* jsonFile = globalPlatformServices->ReadEntireFile($(length), animJsonFilePath);

    Json* root{};
    root = Json_create(jsonFile);
    Json* animations = Json_getItem(root, "animations"); /* clang-format off */BGZ_ASSERT(animations, "Unable to return valid json object!"); /* clang-format on */

    i32 animIndex{};
    for (Json* currentAnimation_json = animations ? animations->child : 0; currentAnimation_json; currentAnimation_json = currentAnimation_json->next, ++animIndex)
    {
        Animation animation{Init::_};

        animation.name = currentAnimation_json->name;

        Json* bonesOfAnimation = currentAnimation_json->child;
        i32 boneIndex{}; f32 maxTimeOfAnimation{};
        for (Json* currentBone = bonesOfAnimation ? bonesOfAnimation->child : 0; currentBone; currentBone = currentBone->next, ++boneIndex)
        {
            Json* rotateTimeline_json = Json_getItem(currentBone, "rotate");
            Json* translateTimeline_json = Json_getItem(currentBone, "translate");
            TimelineSet timelineSet{Init::_};

            if (rotateTimeline_json)
            {
                i32 keyFrameIndex{};
                Timeline rotationTimeline{};
                rotationTimeline.keyFrames = { rotateTimeline_json->size, heap };
                rotationTimeline.exists = true;
                for (Json* jsonKeyFrame = rotateTimeline_json ? rotateTimeline_json->child : 0; jsonKeyFrame; jsonKeyFrame = jsonKeyFrame->next, ++keyFrameIndex)
                {
                    KeyFrame keyFrame;

                    keyFrame.time = Json_getFloat(jsonKeyFrame, "time", 0.0f);
                    keyFrame.angle = Radians(Json_getFloat(jsonKeyFrame, "angle", 0.0f));

                    Insert($(rotationTimeline.keyFrames), keyFrame, keyFrameIndex);
                };

                timelineSet.rotationTimeline = rotationTimeline;

                f32 maxTimeOfRotationTimeline = rotationTimeline.keyFrames.At(rotationTimeline.keyFrames.size - 1).time;
            
                if (maxTimeOfRotationTimeline > maxTimeOfAnimation)
                    maxTimeOfAnimation = maxTimeOfRotationTimeline;
            };

            if (translateTimeline_json)
            {
                i32 keyFrameIndex{};
                Timeline translateTimeline{Init::_};
                translateTimeline.keyFrames = { translateTimeline_json->size, heap };
                translateTimeline.exists = true;
                for (Json* jsonKeyFrame = translateTimeline_json ? translateTimeline_json->child : 0; jsonKeyFrame; jsonKeyFrame = jsonKeyFrame->next, ++keyFrameIndex)
                {
                    KeyFrame keyFrame;

                    f32 pixelsPerMeter{100.0f};
                    keyFrame.time = Json_getFloat(jsonKeyFrame, "time", 0.0f);
                    keyFrame.translation.x = Json_getFloat(jsonKeyFrame, "x", 0.0f) / pixelsPerMeter;
                    keyFrame.translation.y = Json_getFloat(jsonKeyFrame, "y", 0.0f) / pixelsPerMeter;

                    Insert($(translateTimeline.keyFrames), keyFrame, keyFrameIndex);
                };

                timelineSet.translationTimeline = translateTimeline;

                f32 maxTimeOfTranslationTimeline = translateTimeline.keyFrames.At(translateTimeline.keyFrames.size - 1).time;
            
                if (maxTimeOfTranslationTimeline > maxTimeOfAnimation)
                    maxTimeOfAnimation = maxTimeOfTranslationTimeline;
            };

            animation.totalTime = maxTimeOfAnimation;
            Insert<TimelineSet>($(animation.boneTimelineSets), currentBone->name, timelineSet);
        };

        Insert<Animation>($(this->animations), animation.name, animation);
    };
};

void SetIdleAnimation(AnimationQueue&& animQueue, const AnimationData animData, const char* animName)
{
    i32 index = GetHashIndex<Animation>(animData.animations, animName);
    BGZ_ASSERT(index != HASH_DOES_NOT_EXIST, "Wrong animations name!");

    Animation sourceAnim {Init::_};
    sourceAnim = *GetVal<Animation>(animData.animations, index, animName);

    Animation destAnim = sourceAnim;
    CopyArray(sourceAnim.boneTimelineSets.keyInfos, $(destAnim.boneTimelineSets.keyInfos));
    CopyArray(sourceAnim.boneRotations.keyInfos, $(destAnim.boneRotations.keyInfos));
    CopyArray(sourceAnim.boneTranslations.keyInfos, $(destAnim.boneTranslations.keyInfos));
    destAnim.status = PlayBackStatus::IDLE;
    animQueue.idleAnim = destAnim;

    animQueue.queuedAnimations.PushBack(destAnim);
};

void QueueAnimation(AnimationQueue&& animQueue, const AnimationData animData, const char* animName, PlayBackStatus playBackStatus)
{
    BGZ_ASSERT(playBackStatus != PlayBackStatus::IDLE, "Not suppose to set an IDLE status");

    i32 index = GetHashIndex<Animation>(animData.animations, animName);
    BGZ_ASSERT(index != HASH_DOES_NOT_EXIST, "Wrong animations name!");

    Animation sourceAnim {Init::_};
    sourceAnim = *GetVal<Animation>(animData.animations, index, animName);

    Animation destAnim = sourceAnim;
    CopyArray(sourceAnim.boneTimelineSets.keyInfos, $(destAnim.boneTimelineSets.keyInfos));
    CopyArray(sourceAnim.boneRotations.keyInfos, $(destAnim.boneRotations.keyInfos));
    CopyArray(sourceAnim.boneTranslations.keyInfos, $(destAnim.boneTranslations.keyInfos));
    destAnim.status = playBackStatus;

    switch(playBackStatus)
    {
        case PlayBackStatus::DEFAULT:
        {
            animQueue.queuedAnimations.PushBack(destAnim);
        }break;

        case PlayBackStatus::IMMEDIATE:
        {
            animQueue.queuedAnimations.Reset();
            animQueue.queuedAnimations.PushBack(destAnim);
        }break;

        case PlayBackStatus::NEXT:
        {
            {//Clear out animations not currently playing and insert new animation to play next
                if(NOT animQueue.queuedAnimations.Empty())
                {
                    animQueue.queuedAnimations.write = animQueue.queuedAnimations.read + 1;
                    animQueue.queuedAnimations.PushBack(destAnim);
                }
                else
                {
                    animQueue.queuedAnimations.PushBack(destAnim);
                }
            }
        }break;

        InvalidDefaultCase;
    };
};

//Returns higher keyFrame (e.g. if range is between 0 - 1 then keyFrame number 1 is returned)
i32 _ActiveKeyFrame(Timeline timelineOfBone, f32 currentAnimRuntime)
{
    i32 keyFrameCount = (i32)timelineOfBone.keyFrames.size - 1;

    while(keyFrameCount)
    {
        KeyFrame keyFrame0 = timelineOfBone.keyFrames.At(keyFrameCount - 1);
        KeyFrame keyFrame1 = timelineOfBone.keyFrames.At(keyFrameCount);

        if (keyFrame0.time < currentAnimRuntime && keyFrame1.time > currentAnimRuntime && timelineOfBone.keyFrames.size != 1)                        
        {
            return keyFrameCount;
        }
        else
        {
            --keyFrameCount;
        }
    };

    return keyFrameCount;
};

void UpdateAnimationState(AnimationQueue&& animQueue, Dynam_Array<Bone>* bones, f32 prevFrameDT)
{
    Animation* anim = animQueue.queuedAnimations.GetFirstElem();

    if(anim)
    {
        anim->currentTime += prevFrameDT;

        f32 maxTimeOfAnimation{};
        for (i32 boneIndex{}; boneIndex < bones->size; ++boneIndex)
        {
            i32 hashIndex = GetHashIndex<TimelineSet>(anim->boneTimelineSets, bones->At(boneIndex).name);

            if (hashIndex != HASH_DOES_NOT_EXIST)
            {
                Bone* bone = &bones->At(boneIndex);

                if(!strcmp(bone->name, "Pelvis"))
                    int x{3};

                TimelineSet* timelineSet = GetVal<TimelineSet>(anim->boneTimelineSets, hashIndex, bones->At(boneIndex).name);

                Timeline rotationTimelineOfBone = timelineSet->rotationTimeline;
                i32 keyFrameCount{};
                if(rotationTimelineOfBone.exists)
                {
                    f32 lerpedRotation{bone->originalParentLocalRotation + rotationTimelineOfBone.keyFrames.At(0).angle};
                    i32 keyFrameCount = _ActiveKeyFrame(rotationTimelineOfBone, anim->currentTime);
                    if (keyFrameCount) 
                    {
                        f32 rotationAngle0 = bone->originalParentLocalRotation + rotationTimelineOfBone.keyFrames.At(keyFrameCount - 1).angle;
                        f32 rotationAngle1 = bone->originalParentLocalRotation + rotationTimelineOfBone.keyFrames.At(keyFrameCount).angle;

                        ConvertNegativeToPositiveAngle_Radians($(rotationAngle0));
                        ConvertNegativeToPositiveAngle_Radians($(rotationAngle1));

                        //Find percent to lerp
                        f32 diff = rotationTimelineOfBone.keyFrames.At(keyFrameCount).time - rotationTimelineOfBone.keyFrames.At(keyFrameCount - 1).time;
                        f32 diff1 = anim->currentTime - rotationTimelineOfBone.keyFrames.At(keyFrameCount - 1).time;
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
                    };

                    Insert<f32>($(anim->boneRotations), bone->name, lerpedRotation);
                };

                Timeline translationTimeLineOfBone = timelineSet->translationTimeline;
                if(translationTimeLineOfBone.exists)
                {
                    v2f newTranslation{ bone->originalParentLocalPos + translationTimeLineOfBone.keyFrames.At(0).translation };
                    i32 keyFrameCount = _ActiveKeyFrame(translationTimeLineOfBone, anim->currentTime);
                    if (keyFrameCount) 
                    {
                        v2f translation0 = bone->originalParentLocalPos + translationTimeLineOfBone.keyFrames.At(keyFrameCount - 1).translation;
                        v2f translation1 = bone->originalParentLocalPos + translationTimeLineOfBone.keyFrames.At(keyFrameCount).translation;

                        //Find percent to lerp
                        f32 diff = translationTimeLineOfBone.keyFrames.At(keyFrameCount).time - translationTimeLineOfBone.keyFrames.At(keyFrameCount - 1).time;
                        f32 diff1 = anim->currentTime - translationTimeLineOfBone.keyFrames.At(keyFrameCount - 1).time;
                        f32 percentToLerp = diff1 / diff;

                        newTranslation = Lerp(translation0, translation1, percentToLerp);
                    };

                    Insert<v2f>($(anim->boneTranslations), bone->name, newTranslation);
                };
            };
        };
    };
};

void ApplyAnimationToSkeleton(Skeleton&& skel, AnimationQueue&& animQueue)
{
    Animation* anim = animQueue.queuedAnimations.GetFirstElem();

    if(anim)
    {
        if(anim->status == PlayBackStatus::IMMEDIATE)
        {
            //Reset bones to setup position 
            for (i32 boneIndex{}; boneIndex < skel.bones.size; ++boneIndex)
            {
                *skel.bones.At(boneIndex).parentLocalRotation = skel.bones.At(boneIndex).originalParentLocalRotation;
                *skel.bones.At(boneIndex).parentLocalPos = skel.bones.At(boneIndex).originalParentLocalPos;
            };

            anim->status = PlayBackStatus::DEFAULT;
        };

        for (i32 boneIndex{}; boneIndex < skel.bones.size; ++boneIndex)
        {
            i32 hashIndex = GetHashIndex(anim->boneRotations, skel.bones.At(boneIndex).name);

            if (hashIndex != HASH_DOES_NOT_EXIST)
            {
                f32 newBoneRotation = *GetVal(anim->boneRotations, hashIndex, skel.bones.At(boneIndex).name);
                *skel.bones.At(boneIndex).parentLocalRotation = newBoneRotation;
            };

            hashIndex = GetHashIndex(anim->boneTranslations, skel.bones.At(boneIndex).name);

            if (hashIndex != HASH_DOES_NOT_EXIST)
            {
                v2f newBoneTranslation = *GetVal(anim->boneTranslations, hashIndex, skel.bones.At(boneIndex).name);
                *skel.bones.At(boneIndex).parentLocalPos = newBoneTranslation;
            };
        };

        if (anim->currentTime > anim->totalTime)
        {
            anim->currentTime = 0.0f;

            animQueue.queuedAnimations.RemoveElem();

            if(animQueue.queuedAnimations.Empty())
                animQueue.queuedAnimations.PushBack(animQueue.idleAnim);
        };
    };
};

#endif //ANIMATION_IMPL
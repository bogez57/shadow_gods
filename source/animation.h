#ifndef ANIMATION_INCLUDE
#define ANIMATION_INCLUDE

#include <string.h>
#include "skeleton.h"
#include "json.h"

/*
    1. Organize data in more of a SOA fashion?
    2. Currently every bone in the skeleton is iterated over in an animaiton. It's just that animatons
    don't always have bonetimelines existing on every bone
*/

enum class CurveType
{
    LINEAR,
    STEPPED
};

struct RotationTimeline
{
    f32 (*GetTransformationVal)(RotationTimeline, i32);
    b exists{false};
    Dynam_Array<f32> times{heap};
    Dynam_Array<CurveType> curves{heap};
    Dynam_Array<f32> angles{heap};
};

struct TranslationTimeline
{
    v2f (*GetTransformationVal)(TranslationTimeline, i32);
    b exists{false};
    Dynam_Array<f32> times{heap};
    Dynam_Array<CurveType> curves{heap};
    Dynam_Array<v2f> translations{heap};
};

f32 GetTransformationVal_RotationTimeline(RotationTimeline rotationTimeline, i32 keyFrameIndex)
{
    return rotationTimeline.angles.At(keyFrameIndex);
};

v2f GetTransformationVal_TranslationTimeline(TranslationTimeline translationTimeline, i32 keyFrameIndex)
{
    return translationTimeline.translations.At(keyFrameIndex);
};

enum class PlayBackStatus
{
    DEFAULT,
    IDLE,
    IMMEDIATE,
    NEXT,
    HOLD
};

struct Animation
{
    Animation() = default;
    Animation(Init) :
        bones{heap}
    {
        Reserve($(bones), 10);
    };

    const char* name{nullptr};
    f32 totalTime{};
    f32 currentTime{};
    f32 mixTimeDuration{};
    f32 currentMixTime{};
    f32 initialTimeLeftInAnimAtMixingStart{};
    f32 hitBoxEndTime{};
    f32 hitBoxDuration{};
    v2f hitBoxCenterOffset{};
    PlayBackStatus status{PlayBackStatus::DEFAULT};
    b hitBoxTimerStarted{false};
    b repeat{false};
    b hasEnded{false};
    b MixingStarted{false};
    Dynam_Array<Bone*> bones;
    Array<RotationTimeline, 20> boneRotationTimelines;
    Array<TranslationTimeline, 20> boneTranslationTimelines;
    Array<f32, 20> boneRotations;
    Array<v2f, 20> boneTranslations;
    Dynam_Array<Animation> animsToTransitionTo{heap};
};

struct AnimationData
{
    AnimationData() = default;
    AnimationData(const char* animDataJsonFilePath, Skeleton&& skel);

    HashMap_Str<Animation> animations;
};

struct AnimationQueue
{
    AnimationQueue() = default;
    AnimationQueue(Init) :
        queuedAnimations{10, heap},
        idleAnim{Init::_}
    {}

    Ring_Buffer<Animation> queuedAnimations;
    b hasIdleAnim{false};
    Animation idleAnim;
};

void MixAnimations(AnimationData&& animData, const char* anim_from, const char* anim_to);
void CleanUpAnimation(Animation&& anim);
void CopyAnimation(Animation src, Animation&& dest);
void SetIdleAnimation(AnimationQueue&& animQueue, const AnimationData animData, const char* animName);
void CreateAnimationsFromJsonFile(AnimationData&& animData, const char* jsonFilePath);
Animation UpdateAnimationState(AnimationQueue&& animQueue, f32 prevFrameDT);
void QueueAnimation(AnimationQueue&& animQueue, const AnimationData animData, const char* animName, PlayBackStatus status);

#endif

#ifdef ANIMATION_IMPL

AnimationData::AnimationData(const char* animJsonFilePath, Skeleton&& skel) : animations{heap}
{
    i32 length;

    const char* jsonFile = globalPlatformServices->ReadEntireFile($(length), animJsonFilePath);

    Json* root{};
    root = Json_create(jsonFile);
    Json* animations = Json_getItem(root, "animations"); /* clang-format off */BGZ_ASSERT(animations, "Unable to return valid json object!"); /* clang-format on */

    i32 animIndex{};
    for (Json* currentAnimation_json = animations ? animations->child : 0; currentAnimation_json; currentAnimation_json = currentAnimation_json->next, ++animIndex)
    {
        Animation newAnimation{Init::_};
        Insert<Animation>($(this->animations), currentAnimation_json->name, newAnimation);
        i32 index = GetHashIndex(this->animations, currentAnimation_json->name);
        Animation* anim = (Animation*)&this->animations.keyInfos.At(index).value;

        for(i32 boneIndex{}; boneIndex < skel.bones.size; ++boneIndex)
        {
            PushBack($(anim->bones), &skel.bones.At(boneIndex));
        };

        anim->name = currentAnimation_json->name;

        Json* bonesOfAnimation = currentAnimation_json->child;
        i32 boneIndex_json{}; f32 maxTimeOfAnimation{};
        for (Json* currentBone = bonesOfAnimation ? bonesOfAnimation->child : 0; currentBone; currentBone = currentBone->next, ++boneIndex_json)
        {
            i32 boneIndex{};
            while(boneIndex < anim->bones.size)
            {
                if(StringCmp(anim->bones.At(boneIndex)->name, currentBone->name))
                    break;
                else
                    ++boneIndex;
            };

            Json* rotateTimeline_json = Json_getItem(currentBone, "rotate");
            Json* translateTimeline_json = Json_getItem(currentBone, "translate");

            if (rotateTimeline_json)
            {
                RotationTimeline* boneRotationTimeline = &anim->boneRotationTimelines.At(boneIndex);
                boneRotationTimeline->exists = true;
                boneRotationTimeline->GetTransformationVal = &GetTransformationVal_RotationTimeline;

                i32 keyFrameIndex{};
                for (Json* jsonKeyFrame = rotateTimeline_json ? rotateTimeline_json->child : 0; jsonKeyFrame; jsonKeyFrame = jsonKeyFrame->next, ++keyFrameIndex)
                {
                    PushBack($(boneRotationTimeline->times), Json_getFloat(jsonKeyFrame, "time", 0.0f));
                    PushBack($(boneRotationTimeline->angles), Json_getFloat(jsonKeyFrame, "angle", 0.0f));

                    const char* keyFrameCurve = Json_getString(jsonKeyFrame, "curve", ""); 
                    if(StringCmp(keyFrameCurve, "stepped"))
                        PushBack($(boneRotationTimeline->curves), CurveType::STEPPED);
                    else
                        PushBack($(boneRotationTimeline->curves), CurveType::LINEAR);
                };

                f32 maxTimeOfRotationTimeline = boneRotationTimeline->times.At(boneRotationTimeline->times.size - 1);
            
                if (maxTimeOfRotationTimeline > maxTimeOfAnimation)
                    maxTimeOfAnimation = maxTimeOfRotationTimeline;
            };

            if (translateTimeline_json)
            {
                TranslationTimeline* boneTranslationTimeline = &anim->boneTranslationTimelines.At(boneIndex);
                boneTranslationTimeline->exists = true;
                boneTranslationTimeline->GetTransformationVal = &GetTransformationVal_TranslationTimeline;

                i32 keyFrameIndex{};
                for (Json* jsonKeyFrame = translateTimeline_json ? translateTimeline_json->child : 0; jsonKeyFrame; jsonKeyFrame = jsonKeyFrame->next, ++keyFrameIndex)
                {
                    PushBack($(boneTranslationTimeline->times), Json_getFloat(jsonKeyFrame, "time", 0.0f));
                    PushBack($(boneTranslationTimeline->translations), v2f{0.0f, 0.0f});

                    boneTranslationTimeline->translations.At(keyFrameIndex).x = Json_getFloat(jsonKeyFrame, "x", 0.0f); 
                    boneTranslationTimeline->translations.At(keyFrameIndex).y = Json_getFloat(jsonKeyFrame, "y", 0.0f);

                    const char* keyFrameCurve = Json_getString(jsonKeyFrame, "curve", ""); 
                    if(StringCmp(keyFrameCurve, "stepped"))
                        PushBack($(boneTranslationTimeline->curves), CurveType::STEPPED);
                    else
                        PushBack($(boneTranslationTimeline->curves), CurveType::LINEAR);
                };

                f32 maxTimeOfTranslationTimeline = boneTranslationTimeline->times.At(boneTranslationTimeline->times.size - 1);
            
                if (maxTimeOfTranslationTimeline > maxTimeOfAnimation)
                    maxTimeOfAnimation = maxTimeOfTranslationTimeline;
            };

            anim->totalTime = maxTimeOfAnimation;
        };
    };
};

void MixAnimations(AnimationData&& animData, const char* animName_from, const char* animName_to, f32 mixDuration)
{
    i32 index_from = GetHashIndex<Animation>(animData.animations, animName_from);
    i32 index_to = GetHashIndex<Animation>(animData.animations, animName_to);
    BGZ_ASSERT(index_from != HASH_DOES_NOT_EXIST, "Wrong animations name!");
    BGZ_ASSERT(index_to != HASH_DOES_NOT_EXIST, "Wrong animations name!");

    Animation* anim_from = GetVal<Animation>(animData.animations, index_from, animName_from);
    Animation anim_to{Init::_};
    CopyAnimation(*GetVal<Animation>(animData.animations, index_to, animName_to), $(anim_to));
    BGZ_ASSERT(anim_from->totalTime > mixDuration, "passing a mix time that is too long!");

    anim_to.mixTimeDuration = mixDuration;

    if(anim_from->animsToTransitionTo.size > 0)
    {
        for(i32 i{}; i < anim_from->animsToTransitionTo.size; ++i)
        {
            BGZ_ASSERT(NOT StringCmp(anim_from->animsToTransitionTo.At(i).name, anim_to.name), "Duplicate mix animation tyring to be set");
        };

        PushBack($(anim_from->animsToTransitionTo), anim_to);
    }
    else
    {
        PushBack($(anim_from->animsToTransitionTo), anim_to);
    }
};

void CopyAnimation(Animation src, Animation&& dest)
{
    dest = src;

    for(i32 boneIndex{}; boneIndex < src.bones.size; ++boneIndex)
    {
        CopyArray(src.boneTranslationTimelines.At(boneIndex).times, $(dest.boneTranslationTimelines.At(boneIndex).times));
        CopyArray(src.boneTranslationTimelines.At(boneIndex).translations, $(dest.boneTranslationTimelines.At(boneIndex).translations));
        CopyArray(src.boneRotationTimelines.At(boneIndex).times, $(dest.boneRotationTimelines.At(boneIndex).times));
        CopyArray(src.boneRotationTimelines.At(boneIndex).angles, $(dest.boneRotationTimelines.At(boneIndex).angles));
    };

    CopyArray(src.animsToTransitionTo, $(dest.animsToTransitionTo));
};

void SetIdleAnimation(AnimationQueue&& animQueue, const AnimationData animData, const char* animName)
{
    i32 index = GetHashIndex<Animation>(animData.animations, animName);
    BGZ_ASSERT(index != HASH_DOES_NOT_EXIST, "Wrong animations name!");

    Animation* sourceAnim = GetVal<Animation>(animData.animations, index, animName);

    Animation destAnim;
    CopyAnimation(*sourceAnim, $(destAnim));

    destAnim.status = PlayBackStatus::IDLE;
    animQueue.idleAnim = destAnim;

    animQueue.queuedAnimations.PushBack(destAnim);
};

void QueueAnimation(AnimationQueue&& animQueue, const AnimationData animData, const char* animName, PlayBackStatus playBackStatus)
{
    BGZ_ASSERT(playBackStatus != PlayBackStatus::IDLE, "Not suppose to set an IDLE status");

    i32 index = GetHashIndex<Animation>(animData.animations, animName);
    BGZ_ASSERT(index != HASH_DOES_NOT_EXIST, "Wrong animations name!");

    Animation* sourceAnim = GetVal<Animation>(animData.animations, index, animName);

    Animation* nextAnim = animQueue.queuedAnimations.GetNextElem();
    const char* nextAnimName{""};
    if(nextAnim)
        nextAnimName = nextAnim->name;

    if(NOT animQueue.queuedAnimations.full && NOT StringCmp(sourceAnim->name, nextAnimName))
    {
        Animation destAnim; 
        CopyAnimation(*sourceAnim, $(destAnim));

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
                animQueue.queuedAnimations.ClearRemaining();
                animQueue.queuedAnimations.PushBack(destAnim);
            }break;

            case PlayBackStatus::HOLD:
            {
                if(animQueue.queuedAnimations.Empty())
                {
                    animQueue.queuedAnimations.PushBack(destAnim);
                    animQueue.queuedAnimations.GetFirstElem()->repeat = true;
                }
                else if(animQueue.queuedAnimations.GetFirstElem()->repeat == true)
                {
                    //Do nothing
                }
                else
                {
                    animQueue.queuedAnimations.Reset();
                    animQueue.queuedAnimations.PushBack(destAnim);
                    animQueue.queuedAnimations.GetFirstElem()->repeat = true;
                };
            }break;

            InvalidDefaultCase;
        };
    };
};

//Returns lower keyFrame of range(e.g. if range is between 0 - 1 then keyFrame number 0 is returned)
template<typename TransformationTimelineType>
i32 _CurrentActiveKeyFrame(TransformationTimelineType transformationTimelineOfBone, f32 currentAnimRuntime)
{
    BGZ_ASSERT(transformationTimelineOfBone.exists, "Trying to get keyframes from a timeline that does not exist");

    i32 result{};
    i32 keyFrameCount = (i32)transformationTimelineOfBone.times.size - 1;

    f32 keyFrameTime0{};
    f32 keyFrameTime1 = transformationTimelineOfBone.times.At(keyFrameCount);

    while(keyFrameCount)
    {
        keyFrameTime0 = transformationTimelineOfBone.times.At(keyFrameCount - 1);
        keyFrameTime1 = transformationTimelineOfBone.times.At(keyFrameCount);

        if (keyFrameTime0 <= currentAnimRuntime && keyFrameTime1 > currentAnimRuntime )                        
        {
            result = keyFrameCount - 1;
            keyFrameCount = 0;
        }
        else
        {
            --keyFrameCount;
        }
    };

    return result;
};

template<typename transformationType>
struct TransformationRangeResult
{
    transformationType transformation0{};
    transformationType transformation1{}; 
    f32 percentToLerp{};
};

template<typename TransformationType, typename TransformationTimelineType>
TransformationRangeResult<TransformationType> _GetTransformationRangeFromKeyFrames(TransformationTimelineType transformationTimelineOfBone, f32 currentAnimRunTime)
{
    BGZ_ASSERT(transformationTimelineOfBone.times.size != 0, "Can't get translations range from timeline w/ no keyframes!");

    TransformationRangeResult<TransformationType> result{};

    i32 firstKeyFrame{0}, lastKeyFrame{(i32)transformationTimelineOfBone.times.size - 1};
    if(transformationTimelineOfBone.times.size == 1)
    {
        if(currentAnimRunTime > transformationTimelineOfBone.times.At(firstKeyFrame))
        {
            result.transformation0 = transformationTimelineOfBone.GetTransformationVal(transformationTimelineOfBone, firstKeyFrame);
            result.transformation1 = result.transformation0;
            result.percentToLerp = 1.0f;
        };
    }
    else if(currentAnimRunTime > transformationTimelineOfBone.times.At(firstKeyFrame) && currentAnimRunTime < transformationTimelineOfBone.times.At(lastKeyFrame))
    {
        i32 activeKeyFrameIndex = _CurrentActiveKeyFrame(transformationTimelineOfBone, currentAnimRunTime);
        BGZ_ASSERT(activeKeyFrameIndex != lastKeyFrame, "Should never be returning the last keyframe of timeline here!");

        switch(transformationTimelineOfBone.curves.At(activeKeyFrameIndex))
        {
            case CurveType::STEPPED : 
            {
                result.transformation0 = transformationTimelineOfBone.GetTransformationVal(transformationTimelineOfBone, activeKeyFrameIndex);
                result.transformation1 = transformationTimelineOfBone.GetTransformationVal(transformationTimelineOfBone, activeKeyFrameIndex + 1);

                result.percentToLerp = 0.0f;
            }break;

            case CurveType::LINEAR :
            {
                result.transformation0 = transformationTimelineOfBone.GetTransformationVal(transformationTimelineOfBone, activeKeyFrameIndex);
                result.transformation1 = transformationTimelineOfBone.GetTransformationVal(transformationTimelineOfBone, activeKeyFrameIndex + 1);

                f32 time0 = transformationTimelineOfBone.times.At(activeKeyFrameIndex);
                f32 time1 = transformationTimelineOfBone.times.At(activeKeyFrameIndex + 1);

                f32 diff0 = time1 - time0;
                f32 diff1 = currentAnimRunTime - time0;
                result.percentToLerp = diff1 / diff0;
            }break;

            InvalidDefaultCase;
        }
    }
    else if(currentAnimRunTime > transformationTimelineOfBone.times.At(lastKeyFrame))
    {
        result.transformation0 = transformationTimelineOfBone.GetTransformationVal(transformationTimelineOfBone, lastKeyFrame);
        result.transformation1 = result.transformation0;
        result.percentToLerp = 1.0f;
    }

    return result;
};

template<typename transformationRangeType, typename TransformTimelineType>
TransformationRangeResult<transformationRangeType> _GetTransformationRangeFromKeyFrames(Animation* anim, TransformTimelineType boneRotationTimeline_originalAnim, TransformTimelineType boneRotationTimeline_nextAnim, f32 currentAnimRunTime, transformationRangeType initialTransformForMixing)
{
    TransformationRangeResult<transformationRangeType> result{};

    result.transformation0 = initialTransformForMixing;
    result.transformation1 = transformationRangeType{};

    if((boneRotationTimeline_originalAnim.exists && boneRotationTimeline_nextAnim.exists && boneRotationTimeline_nextAnim.times.At(0) > 0.0f) ||
       (boneRotationTimeline_originalAnim.exists && NOT boneRotationTimeline_nextAnim.exists))
    {
        //Leave transformation1 at default 0 value
    }

    else if((boneRotationTimeline_originalAnim.exists && boneRotationTimeline_nextAnim.exists) ||
            (boneRotationTimeline_originalAnim.exists && NOT boneRotationTimeline_nextAnim.exists))
    {
        i32 firstKeyFrame_index = 0;
        result.transformation1 = boneRotationTimeline_nextAnim.GetTransformationVal(boneRotationTimeline_nextAnim, firstKeyFrame_index);
    }

    result.percentToLerp = anim->currentMixTime / anim->initialTimeLeftInAnimAtMixingStart;

    return result;
};

Animation UpdateAnimationState(AnimationQueue&& animQueue, f32 prevFrameDT)
{
    auto InitializeMixingData = [](Animation&& anim, f32 prevFrameDT, f32 amountOfTimeLeftInAnim) -> void
    {
        anim.currentMixTime += prevFrameDT;

        if(NOT anim.MixingStarted)
        {
            anim.initialTimeLeftInAnimAtMixingStart = amountOfTimeLeftInAnim;
            anim.MixingStarted = true;

            for (i32 boneIndex{}; boneIndex < anim.bones.size; ++boneIndex)
            {
                anim.bones.At(boneIndex)->initialRotationForMixing = anim.boneRotations.At(boneIndex);
                anim.bones.At(boneIndex)->initialTranslationForMixing = anim.boneTranslations.At(boneIndex);
            }
        }

        if(anim.currentMixTime > anim.initialTimeLeftInAnimAtMixingStart)
        {
            anim.currentMixTime = anim.initialTimeLeftInAnimAtMixingStart;
            anim.hasEnded = true;
        }
    };

    auto DetermineRotationAmountAndDirection = [](TransformationRangeResult<f32> rotationRange, f32 boneLength) -> f32
    {
        f32 amountOfRotation{};
                    
        v2f boneVector_frame0 = { boneLength * CosR(rotationRange.transformation0), boneLength * SinR(rotationRange.transformation0) };
        v2f boneVector_frame1 = { boneLength * CosR(rotationRange.transformation1), boneLength * SinR(rotationRange.transformation1) };
        f32 directionOfRotation = CrossProduct(boneVector_frame0, boneVector_frame1);

        if (directionOfRotation > 0) //Rotate counter-clockwise
        {
            if (rotationRange.transformation0 < rotationRange.transformation1)
            {
                amountOfRotation = Lerp(rotationRange.transformation0, rotationRange.transformation1, rotationRange.percentToLerp);
            }
            else
            {
                ConvertPositiveToNegativeAngle_Radians($(rotationRange.transformation0));
                amountOfRotation = Lerp(rotationRange.transformation0, rotationRange.transformation1, rotationRange.percentToLerp);
            }
        }
        else //Rotate clockwise
        {
            if (rotationRange.transformation0 < rotationRange.transformation1)
            {
                ConvertPositiveToNegativeAngle_Radians($(rotationRange.transformation1));
                amountOfRotation = Lerp(rotationRange.transformation0, rotationRange.transformation1, rotationRange.percentToLerp);
            }
            else
            {
                amountOfRotation = Lerp(rotationRange.transformation0, rotationRange.transformation1, rotationRange.percentToLerp);
            }
        }

        return amountOfRotation;
    };

    if(animQueue.queuedAnimations.Empty())
        animQueue.queuedAnimations.PushBack(animQueue.idleAnim);

    Animation* anim = animQueue.queuedAnimations.GetFirstElem();
    BGZ_ASSERT(anim, "No animation returned!");

    //Update anim playback time
    f32 prevFrameAnimTime = anim->currentTime;
    anim->currentTime += prevFrameDT;
    if(anim->currentTime > anim->totalTime)
    {
        f32 diff = anim->currentTime - prevFrameAnimTime;
        anim->currentTime -= diff;
        anim->hasEnded = true;
    };

    {//Check if mixing needs to be activated
        f32 amountOfTimeLeftInAnim = anim->totalTime - anim->currentTime;
        Animation* nextAnimInQueue = animQueue.queuedAnimations.GetNextElem();
        if(nextAnimInQueue)
        {
            for(i32 animIndex{}; animIndex < anim->animsToTransitionTo.size; ++animIndex)
            {
                if(StringCmp(anim->animsToTransitionTo.At(animIndex).name, nextAnimInQueue->name))
                {
                    if(amountOfTimeLeftInAnim <= anim->animsToTransitionTo.At(animIndex).mixTimeDuration)
                    {
                        InitializeMixingData($(*anim), prevFrameDT, amountOfTimeLeftInAnim);
                    }
                }
            };
        };
    };

    f32 maxTimeOfAnimation{};
    for (i32 boneIndex{}; boneIndex < anim->bones.size; ++boneIndex)
    {
        const Bone* bone = anim->bones.At(boneIndex);

        //Gather transformation timelines
        v2f amountOfTranslation{0.0f, 0.0f};
        f32 amountOfRotation{0.0f};
        TranslationTimeline translationTimelineOfBone = anim->boneTranslationTimelines.At(boneIndex);
        RotationTimeline rotationTimelineOfBone = anim->boneRotationTimelines.At(boneIndex);

        Animation* nextAnimInQueue = animQueue.queuedAnimations.GetNextElem();

        {//Translation Timeline
            if(anim->MixingStarted)
            {
                BGZ_ASSERT(anim->animsToTransitionTo.size > 0, "No transition animation for mixing has been set!");

                TranslationTimeline nextAnimTranslationTimeline{};
                if(nextAnimInQueue)
                    nextAnimTranslationTimeline = nextAnimInQueue->boneTranslationTimelines.At(boneIndex);

                TransformationRangeResult<v2f> translationRange = _GetTransformationRangeFromKeyFrames<v2f, TranslationTimeline>(anim, translationTimelineOfBone, nextAnimTranslationTimeline, anim->currentTime, anim->bones.At(boneIndex)->initialTranslationForMixing);
                amountOfTranslation = Lerp(translationRange.transformation0, translationRange.transformation1, translationRange.percentToLerp);
            }
            else if (anim->currentTime > 0.0f)
            {
                if(StringCmp(bone->name, "right-shoulder"))
                    int x{};

                if(translationTimelineOfBone.exists)
                {
                    TransformationRangeResult<v2f> translationRange = _GetTransformationRangeFromKeyFrames<v2f, TranslationTimeline>(translationTimelineOfBone, anim->currentTime);
                    amountOfTranslation = Lerp(translationRange.transformation0, translationRange.transformation1, translationRange.percentToLerp);
                };
            };
        }

        {//Rotation Timeline
            if(anim->MixingStarted)
            {
                BGZ_ASSERT(anim->animsToTransitionTo.size > 0, "No transition animation for mixing has been set!");

                RotationTimeline nextAnimRotationTimeline{}; 
                if(nextAnimInQueue)
                    nextAnimRotationTimeline = nextAnimInQueue->boneRotationTimelines.At(boneIndex);

                TransformationRangeResult<f32> rotationRange = _GetTransformationRangeFromKeyFrames<f32, RotationTimeline>(anim, rotationTimelineOfBone, nextAnimRotationTimeline, anim->currentTime, anim->bones.At(boneIndex)->initialRotationForMixing);
                amountOfRotation = DetermineRotationAmountAndDirection(rotationRange, bone->length);
            }
            else if(anim->currentTime > 0.0f)
            {
                if(rotationTimelineOfBone.exists)
                {
                    TransformationRangeResult<f32> rotationRange = _GetTransformationRangeFromKeyFrames<f32, RotationTimeline>(rotationTimelineOfBone, anim->currentTime);
                    amountOfRotation = DetermineRotationAmountAndDirection(rotationRange, bone->length);
                };
            };
        }

        anim->boneRotations.At(boneIndex) = amountOfRotation;
        anim->boneTranslations.At(boneIndex) = amountOfTranslation;
    };

    Animation result;
    CopyAnimation(*anim, $(result));

    if (anim->hasEnded)
    {
        anim->currentTime = 0.0f;
        anim->currentMixTime = 0.0f;
        anim->MixingStarted = false;

        animQueue.queuedAnimations.RemoveElem();

        if(animQueue.queuedAnimations.Empty())
            animQueue.queuedAnimations.PushBack(animQueue.idleAnim);
    };

    return result;
};

void ApplyAnimationToSkeleton(Skeleton&& skel, Animation anim)
{
    ResetBonesToSetupPose($(skel));

    for (i32 boneIndex{}; boneIndex < skel.bones.size; ++boneIndex)
    {
        f32 boneRotationToAdd = anim.boneRotations.At(boneIndex);
        *skel.bones.At(boneIndex).parentLocalRotation += boneRotationToAdd;

        v2f boneTranslationToAdd = anim.boneTranslations.At(boneIndex);
        *skel.bones.At(boneIndex).parentLocalPos += boneTranslationToAdd;
    };
};

void CleanUpAnimation(Animation&& anim)
{
    anim.name = {nullptr};
    anim.totalTime = {};
    anim.currentTime = {};
    anim.status = {PlayBackStatus::DEFAULT};
    anim.repeat = {false};
    anim.hasEnded = {false};

    for(i32 i{}; i < anim.bones.size; ++i)
    {
        CleanUp($(anim.boneTranslationTimelines.At(i).times));
        CleanUp($(anim.boneTranslationTimelines.At(i).translations));
        CleanUp($(anim.boneRotationTimelines.At(i).times));
        CleanUp($(anim.boneRotationTimelines.At(i).angles));
    };
};

#endif //ANIMATION_IMPL
#ifndef ANIMATION_INCLUDE
#define ANIMATION_INCLUDE

#include <string.h>
#include "skeleton.h"
#include "collision_detection.h"
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
    RotationTimeline() = default;
    RotationTimeline(Init, i32 memPartitionID_dynamic);

    f32 (*GetTransformationVal)(RotationTimeline, i32);
    b exists { false };
    Dynam_Array<f32> times;
    Dynam_Array<CurveType> curves;
    Dynam_Array<f32> angles;
};

struct TranslationTimeline
{
    TranslationTimeline() = default;
    TranslationTimeline(Init, i32 memPartitionID_dynamic);

    v2f (*GetTransformationVal)(TranslationTimeline, i32);
    b exists { false };
    Dynam_Array<f32> times;
    Dynam_Array<CurveType> curves;
    Dynam_Array<v2f> translations;
};

struct ScaleTimeline
{
    ScaleTimeline() = default;
    ScaleTimeline(Init, i32 memPartitionID_dynamic);

    v2f (*GetTransformationVal)(ScaleTimeline, i32);
    b exists { false };
    Dynam_Array<f32> times;
    Dynam_Array<CurveType> curves;
    Dynam_Array<v2f> scales;
};

f32 GetTransformationVal_RotationTimeline(RotationTimeline rotationTimeline, i32 keyFrameIndex)
{
    return rotationTimeline.angles.At(keyFrameIndex);
};

v2f GetTransformationVal_TranslationTimeline(TranslationTimeline translationTimeline, i32 keyFrameIndex)
{
    return translationTimeline.translations.At(keyFrameIndex);
};

v2f GetTransformationVal_ScaleTimeline(ScaleTimeline scaleTimeline, i32 keyFrameIndex)
{
    return scaleTimeline.scales.At(keyFrameIndex);
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
    Animation(Init, i32 memPartitionID_dynamic);

    const char* name { nullptr };
    f32 totalTime {};
    f32 currentTime {};
    f32 mixTimeDuration {};
    f32 currentMixTime {};
    f32 initialTimeLeftInAnimAtMixingStart {};
    PlayBackStatus status { PlayBackStatus::DEFAULT };
    b repeat { false };
    b hasEnded { false };
    b MixingStarted { false };
    Dynam_Array<Bone*> bones;
    Dynam_Array<HitBox> hitBoxes;
    Array<RotationTimeline, 20> boneRotationTimelines;
    Array<TranslationTimeline, 20> boneTranslationTimelines;
    Array<ScaleTimeline, 20> boneScaleTimelines;
    Array<f32, 20> boneRotations;
    Array<v2f, 20> boneTranslations;
    Dynam_Array<Animation> animsToTransitionTo;
};

struct AnimationData
{
    AnimationData() = default;
    AnimationData(const char* animDataJsonFilePath, Skeleton skel);

    HashMap_Str<Animation> animations;
};

struct AnimationQueue
{
    AnimationQueue() = default;
    AnimationQueue(Init)
        : queuedAnimations { 10, heap }
        , idleAnim { Init::_ }
    {}

    Ring_Buffer<Animation> queuedAnimations;
    b hasIdleAnim { false };
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

RotationTimeline::RotationTimeline(Init, i32 memPartitionID_dynamic)
    : times {memPartitionID_dynamic}
    , curves {memPartitionID_dynamic}
    , angles {memPartitionID_dynamic}
{
};

TranslationTimeline::TranslationTimeline(Init, i32 memPartitionID_dynamic)
    : times {memPartitionID_dynamic}
    , curves {memPartitionID_dynamic}
    , translations {memPartitionID_dynamic}
{
};

ScaleTimeline::ScaleTimeline(Init, i32 memPartitionID_dynamic)
    : times {memPartitionID_dynamic}
    , curves {memPartitionID_dynamic}
    , scales {memPartitionID_dynamic}
{
};

Animation::Animation(Init, i32 memPartitionID_dynamic)
    : bones { memPartitionID_dynamic }
    , hitBoxes { memPartitionID_dynamic }
    , animsToTransitionTo { memPartitionID_dynamic }
{
    for(i32 i{}; i < this->boneRotationTimelines.Size(); ++i)
    {
        Initialize($(this->boneRotationTimelines.At(i).times), heap);
        Initialize($(this->boneRotationTimelines.At(i).curves), heap);
        Initialize($(this->boneRotationTimelines.At(i).angles), heap);
    };

    for(i32 i{}; i < this->boneTranslationTimelines.Size(); ++i)
    {
        Initialize($(this->boneTranslationTimelines.At(i).times), heap);
        Initialize($(this->boneTranslationTimelines.At(i).curves), heap);
        Initialize($(this->boneTranslationTimelines.At(i).translations), heap);
    };

    for(i32 i{}; i < this->boneScaleTimelines.Size(); ++i)
    {
        Initialize($(this->boneScaleTimelines.At(i).times), heap);
        Initialize($(this->boneScaleTimelines.At(i).curves), heap);
        Initialize($(this->boneScaleTimelines.At(i).scales), heap);
    };
};

AnimationData::AnimationData(const char* animJsonFilePath, Skeleton skel)
    : animations { heap }
{
    i32 length;

    const char* jsonFile = globalPlatformServices->ReadEntireFile($(length), animJsonFilePath);

    Json* root {};
    root = Json_create(jsonFile);
    Json* animations = Json_getItem(root, "animations"); /* clang-format off */BGZ_ASSERT(animations, "Unable to return valid json object!"); /* clang-format on */

    i32 animIndex {};
    for (Json* currentAnimation_json = animations ? animations->child : 0; currentAnimation_json; currentAnimation_json = currentAnimation_json->next, ++animIndex)
    {
        Animation newAnimation { Init::_, heap };
        Insert<Animation>($(this->animations), currentAnimation_json->name, newAnimation);
        i32 index = GetHashIndex(this->animations, currentAnimation_json->name);
        Animation* anim = (Animation*)&this->animations.keyInfos.At(index).value;

        for (i32 boneIndex {}; boneIndex < skel.bones.size; ++boneIndex)
            PushBack($(anim->bones), &skel.bones.At(boneIndex));

        anim->name = currentAnimation_json->name;

        Json* bonesOfAnimation = Json_getItem(currentAnimation_json, "bones");
        i32 boneIndex_json {};
        f32 maxTimeOfAnimation {};
        for (Json* currentBone = bonesOfAnimation ? bonesOfAnimation->child : 0; currentBone; currentBone = currentBone->next, ++boneIndex_json)
        {
            i32 boneIndex {};
            while (boneIndex < anim->bones.size)
            {
                if (StringCmp(anim->bones.At(boneIndex)->name, currentBone->name))
                    break;
                else
                    ++boneIndex;
            };

            Json* rotateTimeline_json = Json_getItem(currentBone, "rotate");
            Json* translateTimeline_json = Json_getItem(currentBone, "translate");
            Json* scaleTimeline_json = Json_getItem(currentBone, "scale");

            if (rotateTimeline_json)
            {
                RotationTimeline* boneRotationTimeline = &anim->boneRotationTimelines.At(boneIndex);
                boneRotationTimeline->exists = true;
                boneRotationTimeline->GetTransformationVal = &GetTransformationVal_RotationTimeline;

                i32 keyFrameIndex {};
                for (Json* jsonKeyFrame = rotateTimeline_json ? rotateTimeline_json->child : 0; jsonKeyFrame; jsonKeyFrame = jsonKeyFrame->next, ++keyFrameIndex)
                {
                    PushBack($(boneRotationTimeline->times), Json_getFloat(jsonKeyFrame, "time", 0.0f));
                    PushBack($(boneRotationTimeline->angles), Json_getFloat(jsonKeyFrame, "angle", 0.0f));

                    const char* keyFrameCurve = Json_getString(jsonKeyFrame, "curve", "");
                    if (StringCmp(keyFrameCurve, "stepped"))
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

                i32 keyFrameIndex {};
                for (Json* jsonKeyFrame = translateTimeline_json ? translateTimeline_json->child : 0; jsonKeyFrame; jsonKeyFrame = jsonKeyFrame->next, ++keyFrameIndex)
                {
                    PushBack($(boneTranslationTimeline->times), Json_getFloat(jsonKeyFrame, "time", 0.0f));
                    PushBack($(boneTranslationTimeline->translations), v2f { 0.0f, 0.0f });

                    boneTranslationTimeline->translations.At(keyFrameIndex).x = Json_getFloat(jsonKeyFrame, "x", 0.0f);
                    boneTranslationTimeline->translations.At(keyFrameIndex).y = Json_getFloat(jsonKeyFrame, "y", 0.0f);

                    const char* keyFrameCurve = Json_getString(jsonKeyFrame, "curve", "");
                    if (StringCmp(keyFrameCurve, "stepped"))
                        PushBack($(boneTranslationTimeline->curves), CurveType::STEPPED);
                    else
                        PushBack($(boneTranslationTimeline->curves), CurveType::LINEAR);
                };

                f32 maxTimeOfTranslationTimeline = boneTranslationTimeline->times.At(boneTranslationTimeline->times.size - 1);

                if (maxTimeOfTranslationTimeline > maxTimeOfAnimation)
                    maxTimeOfAnimation = maxTimeOfTranslationTimeline;
            };

            if (scaleTimeline_json)
            {
                //Implement
            };
        };

        { //Setup hit boxes for anim if any
            Json* collisionBoxesOfAnimation_json = Json_getItem(currentAnimation_json, "slots");

            if (collisionBoxesOfAnimation_json)
            {
                i32 hitBoxIndex {};
                for (Json* currentCollisionBox_json = collisionBoxesOfAnimation_json ? collisionBoxesOfAnimation_json->child : 0; currentCollisionBox_json; currentCollisionBox_json = currentCollisionBox_json->next, ++hitBoxIndex)
                {
                    HitBox hitBox {};
                    PushBack($(anim->hitBoxes), hitBox);
                    anim->hitBoxes.At(hitBoxIndex).boneName = CallocType(heap, char, 100);

                    { //Get bone name collision box is attached to by cutting out "box-" prefix
                        char boneName[100] = {};
                        i32 j { 0 };
                        for (i32 i = 4; i < strlen(currentCollisionBox_json->name); ++i, ++j)
                            boneName[j] = currentCollisionBox_json->name[i];

                        memcpy(anim->hitBoxes.At(hitBoxIndex).boneName, boneName, strlen(boneName));
                    };

                    Json* collisionBoxTimeline_json = Json_getItem(currentCollisionBox_json, "attachment");
                    Json* keyFrame1_json = collisionBoxTimeline_json->child;
                    Json* keyFrame2_json = collisionBoxTimeline_json->child->next;

                    f32 time1 = Json_getFloat(keyFrame1_json, "time", 0.0f);
                    f32 time2 = Json_getFloat(keyFrame2_json, "time", 0.0f);

                    anim->hitBoxes.At(hitBoxIndex).timeUntilHitBoxIsActivated = time1;
                    anim->hitBoxes.At(hitBoxIndex).duration = time2 - time1;

                    Dynam_Array<v2f> adjustedCollisionBoxVerts { heap }, finalCollsionBoxVertCoords { heap };
                    defer { CleanUp($(adjustedCollisionBoxVerts)); };
                    defer { CleanUp($(finalCollsionBoxVertCoords)); };

                    Json* collisionBoxDeformTimeline_json = Json_getItem(currentAnimation_json, "deform");
                    Json* deformKeyFrame_json = collisionBoxDeformTimeline_json->child->child->child->child;
                    Json* deformedVerts_json = Json_getItem(deformKeyFrame_json, "vertices")->child;

                    Bone* bone = GetBoneFromSkeleton(skel, anim->hitBoxes.At(hitBoxIndex).boneName);
                    i32 numVerts = (i32)bone->originalCollisionBoxVerts.size;
                    for (i32 i {}; i < numVerts; ++i)
                    {
                        //Read in adjusted/deformed vert data from individual animation json info
                        PushBack($(adjustedCollisionBoxVerts), v2f { deformedVerts_json->valueFloat, deformedVerts_json->next->valueFloat });
                        deformedVerts_json = deformedVerts_json->next->next;

                        //Transform original verts into new transformed vert positions based on anim deformed verts
                        v2f finalVertCoord = bone->originalCollisionBoxVerts.At(i) + adjustedCollisionBoxVerts.At(i);
                        PushBack($(finalCollsionBoxVertCoords), finalVertCoord);
                    };

                    v2f vector0_1 = finalCollsionBoxVertCoords.At(0) - finalCollsionBoxVertCoords.At(1);
                    v2f vector1_2 = finalCollsionBoxVertCoords.At(1) - finalCollsionBoxVertCoords.At(2);

                    anim->hitBoxes.At(hitBoxIndex).size.width = Magnitude(vector0_1);
                    anim->hitBoxes.At(hitBoxIndex).size.height = Magnitude(vector1_2);
                    anim->hitBoxes.At(hitBoxIndex).worldPosOffset = { (finalCollsionBoxVertCoords.At(0).x + finalCollsionBoxVertCoords.At(2).x) / 2.0f,
                        (finalCollsionBoxVertCoords.At(0).y + finalCollsionBoxVertCoords.At(2).y) / 2.0f };
                }
            };
        }

        anim->totalTime = maxTimeOfAnimation;
    };
};

AnimationData CopyAnimData(AnimationData src)
{
    AnimationData dest = src;
    dest.animations = CopyHashMap(src.animations);

    return dest;
};

void MixAnimations(AnimationData&& animData, const char* animName_from, const char* animName_to, f32 mixDuration)
{
    i32 index_from = GetHashIndex<Animation>(animData.animations, animName_from);
    i32 index_to = GetHashIndex<Animation>(animData.animations, animName_to);
    BGZ_ASSERT(index_from != HASH_DOES_NOT_EXIST, "Wrong animations name!");
    BGZ_ASSERT(index_to != HASH_DOES_NOT_EXIST, "Wrong animations name!");

    Animation* anim_from = GetVal<Animation>(animData.animations, index_from, animName_from);
    Animation anim_to { Init::_, heap };
    defer { CleanUpAnimation($(anim_to)); };

    CopyAnimation(*GetVal<Animation>(animData.animations, index_to, animName_to), $(anim_to));
    BGZ_ASSERT(anim_from->totalTime > mixDuration, "passing a mix time that is too long!");

    anim_to.mixTimeDuration = mixDuration;

    if (anim_from->animsToTransitionTo.size > 0)
    {
        for (i32 i {}; i < anim_from->animsToTransitionTo.size; ++i)
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

    for (i32 boneIndex {}; boneIndex < src.bones.size; ++boneIndex)
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
    const char* nextAnimName { "" };
    if (nextAnim)
        nextAnimName = nextAnim->name;

    if (NOT animQueue.queuedAnimations.full && NOT StringCmp(sourceAnim->name, nextAnimName))
    {
        Animation destAnim;
        CopyAnimation(*sourceAnim, $(destAnim));

        destAnim.status = playBackStatus;

        switch (playBackStatus)
        {
        case PlayBackStatus::DEFAULT:
        {
            animQueue.queuedAnimations.PushBack(destAnim);
        }
        break;

        case PlayBackStatus::IMMEDIATE:
        {
            animQueue.queuedAnimations.Reset();
            animQueue.queuedAnimations.PushBack(destAnim);
        }
        break;

        case PlayBackStatus::NEXT:
        {
            animQueue.queuedAnimations.ClearRemaining();
            animQueue.queuedAnimations.PushBack(destAnim);
        }
        break;

        case PlayBackStatus::HOLD:
        {
            if (animQueue.queuedAnimations.Empty())
            {
                animQueue.queuedAnimations.PushBack(destAnim);
                animQueue.queuedAnimations.GetFirstElem()->repeat = true;
            }
            else if (animQueue.queuedAnimations.GetFirstElem()->repeat == true)
            {
                //Do nothing
            }
            else
            {
                animQueue.queuedAnimations.Reset();
                animQueue.queuedAnimations.PushBack(destAnim);
                animQueue.queuedAnimations.GetFirstElem()->repeat = true;
            };
        }
        break;

            InvalidDefaultCase;
        };
    };
};

//Returns lower keyFrame of range(e.g. if range is between 0 - 1 then keyFrame number 0 is returned)
template <typename TransformationTimelineType>
i32 _CurrentActiveKeyFrame(TransformationTimelineType transformationTimelineOfBone, f32 currentAnimRuntime)
{
    BGZ_ASSERT(transformationTimelineOfBone.exists, "Trying to get keyframes from a timeline that does not exist");

    i32 result {};
    i32 keyFrameCount = (i32)transformationTimelineOfBone.times.size - 1;

    f32 keyFrameTime0 {};
    f32 keyFrameTime1 = transformationTimelineOfBone.times.At(keyFrameCount);

    while (keyFrameCount)
    {
        keyFrameTime0 = transformationTimelineOfBone.times.At(keyFrameCount - 1);
        keyFrameTime1 = transformationTimelineOfBone.times.At(keyFrameCount);

        if (keyFrameTime0 <= currentAnimRuntime && keyFrameTime1 > currentAnimRuntime)
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

template <typename transformationType>
struct TransformationRangeResult
{
    transformationType transformation0 {};
    transformationType transformation1 {};
    f32 percentToLerp {};
};

template <typename TransformationType, typename TransformationTimelineType>
TransformationRangeResult<TransformationType> _GetTransformationRangeFromKeyFrames(TransformationTimelineType transformationTimelineOfBone, f32 currentAnimRunTime)
{
    BGZ_ASSERT(transformationTimelineOfBone.times.size != 0, "Can't get translations range from timeline w/ no keyframes!");

    TransformationRangeResult<TransformationType> result {};

    i32 firstKeyFrame { 0 }, lastKeyFrame { (i32)transformationTimelineOfBone.times.size - 1 };
    if (transformationTimelineOfBone.times.size == 1)
    {
        if (currentAnimRunTime >= transformationTimelineOfBone.times.At(firstKeyFrame))
        {
            result.transformation0 = transformationTimelineOfBone.GetTransformationVal(transformationTimelineOfBone, firstKeyFrame);
            result.transformation1 = result.transformation0;
            result.percentToLerp = 1.0f;
        };
    }
    else if (currentAnimRunTime >= transformationTimelineOfBone.times.At(firstKeyFrame) && currentAnimRunTime < transformationTimelineOfBone.times.At(lastKeyFrame))
    {
        i32 activeKeyFrameIndex = _CurrentActiveKeyFrame(transformationTimelineOfBone, currentAnimRunTime);
        BGZ_ASSERT(activeKeyFrameIndex != lastKeyFrame, "Should never be returning the last keyframe of timeline here!");

        switch (transformationTimelineOfBone.curves.At(activeKeyFrameIndex))
        {
        case CurveType::STEPPED:
        {
            result.transformation0 = transformationTimelineOfBone.GetTransformationVal(transformationTimelineOfBone, activeKeyFrameIndex);
            result.transformation1 = transformationTimelineOfBone.GetTransformationVal(transformationTimelineOfBone, activeKeyFrameIndex + 1);

            result.percentToLerp = 0.0f;
        }
        break;

        case CurveType::LINEAR:
        {
            result.transformation0 = transformationTimelineOfBone.GetTransformationVal(transformationTimelineOfBone, activeKeyFrameIndex);
            result.transformation1 = transformationTimelineOfBone.GetTransformationVal(transformationTimelineOfBone, activeKeyFrameIndex + 1);

            f32 time0 = transformationTimelineOfBone.times.At(activeKeyFrameIndex);
            f32 time1 = transformationTimelineOfBone.times.At(activeKeyFrameIndex + 1);

            f32 diff0 = time1 - time0;
            f32 diff1 = currentAnimRunTime - time0;
            result.percentToLerp = diff1 / diff0;
        }
        break;

            InvalidDefaultCase;
        }
    }
    else if (currentAnimRunTime >= transformationTimelineOfBone.times.At(lastKeyFrame))
    {
        result.transformation0 = transformationTimelineOfBone.GetTransformationVal(transformationTimelineOfBone, lastKeyFrame);
        result.transformation1 = result.transformation0;
        result.percentToLerp = 1.0f;
    }

    return result;
};

template <typename transformationRangeType, typename TransformTimelineType>
TransformationRangeResult<transformationRangeType> _GetTransformationRangeFromKeyFrames(Animation* anim, TransformTimelineType boneRotationTimeline_originalAnim, TransformTimelineType boneRotationTimeline_nextAnim, f32 currentAnimRunTime, transformationRangeType initialTransformForMixing)
{
    TransformationRangeResult<transformationRangeType> result {};

    result.transformation0 = initialTransformForMixing;
    result.transformation1 = transformationRangeType {};

    if ((boneRotationTimeline_originalAnim.exists && boneRotationTimeline_nextAnim.exists && boneRotationTimeline_nextAnim.times.At(0) > 0.0f) || (boneRotationTimeline_originalAnim.exists && NOT boneRotationTimeline_nextAnim.exists))
    {
        //Leave transformation1 at default 0 value
    }

    else if ((boneRotationTimeline_originalAnim.exists && boneRotationTimeline_nextAnim.exists) || (boneRotationTimeline_originalAnim.exists && NOT boneRotationTimeline_nextAnim.exists))
    {
        i32 firstKeyFrame_index = 0;
        result.transformation1 = boneRotationTimeline_nextAnim.GetTransformationVal(boneRotationTimeline_nextAnim, firstKeyFrame_index);
    }

    result.percentToLerp = anim->currentMixTime / anim->initialTimeLeftInAnimAtMixingStart;

    return result;
};

Animation UpdateAnimationState(AnimationQueue&& animQueue, f32 prevFrameDT)
{
    auto InitializeMixingData = [](Animation&& anim, f32 prevFrameDT, f32 amountOfTimeLeftInAnim) -> void {
        anim.currentMixTime += prevFrameDT;

        if (NOT anim.MixingStarted)
        {
            anim.initialTimeLeftInAnimAtMixingStart = amountOfTimeLeftInAnim;
            anim.MixingStarted = true;

            for (i32 boneIndex {}; boneIndex < anim.bones.size; ++boneIndex)
            {
                anim.bones.At(boneIndex)->initialRotationForMixing = anim.boneRotations.At(boneIndex);
                anim.bones.At(boneIndex)->initialTranslationForMixing = anim.boneTranslations.At(boneIndex);
            }
        }

        if (anim.currentMixTime > anim.initialTimeLeftInAnimAtMixingStart)
        {
            anim.currentMixTime = anim.initialTimeLeftInAnimAtMixingStart;
            anim.hasEnded = true;
        }
    };

    auto DetermineRotationAmountAndDirection = [](TransformationRangeResult<f32> rotationRange, f32 boneLength) -> f32 {
        f32 amountOfRotation {};

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

    if (animQueue.queuedAnimations.Empty())
        animQueue.queuedAnimations.PushBack(animQueue.idleAnim);

    Animation* anim = animQueue.queuedAnimations.GetFirstElem();
    BGZ_ASSERT(anim, "No animation returned!");

    { //Check if mixing needs to be activated
        f32 amountOfTimeLeftInAnim = anim->totalTime - anim->currentTime;
        Animation* nextAnimInQueue = animQueue.queuedAnimations.GetNextElem();
        if (nextAnimInQueue)
        {
            for (i32 animIndex {}; animIndex < anim->animsToTransitionTo.size; ++animIndex)
            {
                if (StringCmp(anim->animsToTransitionTo.At(animIndex).name, nextAnimInQueue->name))
                {
                    if (amountOfTimeLeftInAnim <= anim->animsToTransitionTo.At(animIndex).mixTimeDuration)
                    {
                        InitializeMixingData($(*anim), prevFrameDT, amountOfTimeLeftInAnim);
                    }
                }
            };
        };
    };

    f32 maxTimeOfAnimation {};
    for (i32 boneIndex {}; boneIndex < anim->bones.size; ++boneIndex)
    {
        const Bone* bone = anim->bones.At(boneIndex);

        //Gather transformation timelines
        v2f amountOfTranslation { 0.0f, 0.0f };
        f32 amountOfRotation { 0.0f };
        TranslationTimeline translationTimelineOfBone = anim->boneTranslationTimelines.At(boneIndex);
        RotationTimeline rotationTimelineOfBone = anim->boneRotationTimelines.At(boneIndex);
        ScaleTimeline scaleTimelineOfBone = anim->boneScaleTimelines.At(boneIndex);

        Animation* nextAnimInQueue = animQueue.queuedAnimations.GetNextElem();

        { //Translation Timeline
            if (anim->MixingStarted)
            {
                BGZ_ASSERT(anim->animsToTransitionTo.size > 0, "No transition animation for mixing has been set!");

                TranslationTimeline nextAnimTranslationTimeline {};
                if (nextAnimInQueue)
                    nextAnimTranslationTimeline = nextAnimInQueue->boneTranslationTimelines.At(boneIndex);

                TransformationRangeResult<v2f> translationRange = _GetTransformationRangeFromKeyFrames<v2f, TranslationTimeline>(anim, translationTimelineOfBone, nextAnimTranslationTimeline, anim->currentTime, anim->bones.At(boneIndex)->initialTranslationForMixing);
                amountOfTranslation = Lerp(translationRange.transformation0, translationRange.transformation1, translationRange.percentToLerp);
            }
            else
            {
                if (translationTimelineOfBone.exists)
                {
                    TransformationRangeResult<v2f> translationRange = _GetTransformationRangeFromKeyFrames<v2f, TranslationTimeline>(translationTimelineOfBone, anim->currentTime);
                    amountOfTranslation = Lerp(translationRange.transformation0, translationRange.transformation1, translationRange.percentToLerp);
                };
            };
        }

        { //Rotation Timeline
            if (anim->MixingStarted)
            {
                BGZ_ASSERT(anim->animsToTransitionTo.size > 0, "No transition animation for mixing has been set!");

                RotationTimeline nextAnimRotationTimeline {};
                if (nextAnimInQueue)
                    nextAnimRotationTimeline = nextAnimInQueue->boneRotationTimelines.At(boneIndex);

                TransformationRangeResult<f32> rotationRange = _GetTransformationRangeFromKeyFrames<f32, RotationTimeline>(anim, rotationTimelineOfBone, nextAnimRotationTimeline, anim->currentTime, anim->bones.At(boneIndex)->initialRotationForMixing);
                amountOfRotation = DetermineRotationAmountAndDirection(rotationRange, bone->length);
            }
            else
            {
                if (StringCmp(bone->name, "right-bicep"))
                    int x {};

                if (rotationTimelineOfBone.exists)
                {
                    TransformationRangeResult<f32> rotationRange = _GetTransformationRangeFromKeyFrames<f32, RotationTimeline>(rotationTimelineOfBone, anim->currentTime);
                    amountOfRotation = DetermineRotationAmountAndDirection(rotationRange, bone->length);
                };
            };
        }

        { //Scale timeline
            //Implement
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

        if (animQueue.queuedAnimations.Empty())
            animQueue.queuedAnimations.PushBack(animQueue.idleAnim);
    };

    //Update anim playback time
    f32 prevFrameAnimTime = anim->currentTime;
    anim->currentTime += prevFrameDT;
    if (anim->currentTime > anim->totalTime)
    {
        f32 diff = anim->currentTime - anim->totalTime;
        anim->currentTime -= diff;
        anim->hasEnded = true;
    }
    else if (anim->currentTime == anim->totalTime)
    {
        anim->hasEnded = true;
    }

    return result;
};

void ApplyAnimationToSkeleton(Skeleton&& skel, Animation anim)
{
    ResetBonesToSetupPose($(skel));

    for (i32 boneIndex {}; boneIndex < skel.bones.size; ++boneIndex)
    {
        f32 boneRotationToAdd = anim.boneRotations.At(boneIndex);
        skel.bones.At(boneIndex).parentBoneSpace.rotation += boneRotationToAdd;

        v2f boneTranslationToAdd = anim.boneTranslations.At(boneIndex);
        skel.bones.At(boneIndex).parentBoneSpace.translation += boneTranslationToAdd;
    };
};

void CleanUpAnimation(Animation&& anim)
{
    anim.name = { nullptr };
    anim.totalTime = {};
    anim.currentTime = {};
    anim.status = { PlayBackStatus::DEFAULT };
    anim.repeat = { false };
    anim.hasEnded = { false };

    for (i32 i {}; i < anim.bones.size; ++i)
    {
        CleanUp($(anim.boneTranslationTimelines.At(i).times));
        CleanUp($(anim.boneTranslationTimelines.At(i).translations));
        CleanUp($(anim.boneRotationTimelines.At(i).times));
        CleanUp($(anim.boneRotationTimelines.At(i).angles));
    };
};

#endif //ANIMATION_IMPL
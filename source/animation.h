#ifndef ANIMATION_INCLUDE
#define ANIMATION_INCLUDE

#include <string.h>
#include "skeleton.h"
#include "collision_detection.h"
#include "json.h"
#include "memory_handling.h"

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
    
    f32 (*GetTransformationVal)(RotationTimeline, s32);
    bool  exists { false };
    Array<f32, 10> times;
    Array<CurveType, 10> curves;
    Array<f32, 10> angles;
    s32 timesCount {}, curvesCount {}, anglesCount {};
};

struct TranslationTimeline
{
    TranslationTimeline() = default;
    
    v2 (*GetTransformationVal)(TranslationTimeline, s32);
    bool  exists { false };
    Array<f32, 10> times;
    Array<CurveType, 10> curves;
    Array<v2, 10> translations;
    s32 timesCount {}, curvesCount {}, translationCount {};
};

struct ScaleTimeline
{
    ScaleTimeline() = default;
    
    v2 (*GetTransformationVal)(ScaleTimeline, s32);
    bool exists { false };
    Array<f32, 10> times;
    Array<CurveType, 10> curves;
    Array<v2, 10> scales;
    s32 timesCount {}, curvesCount {}, scaleCount {};
};

f32 GetTransformationVal_RotationTimeline(RotationTimeline rotationTimeline, s32 keyFrameIndex)
{
    return rotationTimeline.angles[keyFrameIndex];
};

v2 GetTransformationVal_TranslationTimeline(TranslationTimeline translationTimeline, s32 keyFrameIndex)
{
    return translationTimeline.translations[keyFrameIndex];
};

v2 GetTransformationVal_ScaleTimeline(ScaleTimeline scaleTimeline, s32 keyFrameIndex)
{
    return scaleTimeline.scales[keyFrameIndex];
};

enum class PlayBackStatus
{
    DEFAULT,
    IDLE,
    IMMEDIATE,
    IMMEDIATE_NOREPEAT,
    NEXT,
    HOLD
};

struct Animation
{
    Animation() = default;
    
    const char* name { nullptr };
    f32 totalTime {};
    f32 currentTime {};
    f32 mixTimeDuration {};
    f32 currentMixTime {};
    f32 initialTimeLeftInAnimAtMixingStart {};
    PlayBackStatus status { PlayBackStatus::DEFAULT };
    bool repeat { false };
    bool hasEnded { false };
    bool MixingStarted { false };
    DbgArray<HitBox, 10> hitBoxes;
    DbgArray<Animation*, 10> animsToTransitionTo;
    Array<Bone*, 20> bones;
    Array<RotationTimeline, 20> boneRotationTimelines;
    Array<TranslationTimeline, 20> boneTranslationTimelines;
    Array<ScaleTimeline, 20> boneScaleTimelines;
    Array<f32, 20> boneRotations;
    Array<v2, 20> boneTranslations;
};

struct AnimationMap
{
    RunTimeArr<Animation> animations {};
    RunTimeArr<s32> keys {};
};

void InitAnimMap(AnimationMap&& animMap, Memory_Partition&& memPart, s32 size)
{
    InitArr($(animMap.animations), &memPart, size);
    InitArr($(animMap.keys), &memPart, size);
};

void InsertAnimation(AnimationMap&& animMap, const char* animName, Animation anim)
{
    s32 uniqueID {};
    for (s32 i {}; animName[i] != 0; ++i)//TODO: Using this method to come up with keys isn't full proof. Could have a name with same letters in different order and it would produce conflicting keys. Prob need to change.
        uniqueID += animName[i];
    
    animMap.keys.Push() = uniqueID;
    animMap.animations.Push() = anim;
};

Animation* GetAnimation(AnimationMap animMap, const char* animName)
{
    s32 uniqueID {};
    for (s32 i {}; animName[i] != 0; ++i)
        uniqueID += animName[i];
    
    s32 keyIndex { -1 };
    for (s32 i {}; i < animMap.keys.length; ++i)
    {
        if (uniqueID == animMap.keys[i])
        {
            keyIndex = i;
            break;
        }
    };
    
    if (keyIndex == -1)
        BGZ_ASSERT(1 < 0, "Animation name is either incorrect or requested animation doesn't exist!");
    
    return &animMap.animations[keyIndex];
};

struct AnimationData
{
    AnimationData() = default;
    
    AnimationMap animMap {};
};

struct AnimationQueue
{
    AnimationQueue() = default;
    
    bool hasIdleAnim { false };
    Animation idleAnim;
    Ring_Buffer<Animation, 10> queuedAnimations;
};

void InitAnimData(AnimationData&& animData, Memory_Partition&& memPart, const char* animDataJsonFilePath, Skeleton skel);
void MixAnimations(AnimationData&& animData, const char* anim_from, const char* anim_to);
void CopyAnimation(Animation src, Animation&& dest);
void SetIdleAnimation(AnimationQueue&& animQueue, const AnimationData animData, const char* animName);
void CreateAnimationsFromJsonFile(AnimationData&& animData, const char* jsonFilePath);
Animation UpdateAnimationState(AnimationQueue&& animQueue, f32 prevFrameDT);
void QueueAnimation(AnimationQueue&& animQueue, const AnimationData animData, const char* animName, PlayBackStatus status);

#endif

#ifdef ANIMATION_IMPL

void InitAnimData(AnimationData&& animData, Memory_Partition&& memPart, const char* animDataJsonFilePath, Skeleton skel)
{
    s32 length;
    
    const char* jsonFile = globalPlatformServices->ReadEntireFile($(length), animDataJsonFilePath);
    
    Json* root {};
    root = Json_create(jsonFile);
    Json* animations = Json_getItem(root, "animations"); /* clang-format off */BGZ_ASSERT(animations, "Unable to return valid json object!"); /* clang-format on */
    
    InitAnimMap($(animData.animMap), $(memPart), 20);
    
    s32 animIndex {};
    for (Json* currentAnimation_json = animations ? animations->child : 0; currentAnimation_json; currentAnimation_json = currentAnimation_json->next, ++animIndex)
    {
        Animation newAnimation {};
        InsertAnimation($(animData.animMap), currentAnimation_json->name, newAnimation);
        Animation* anim = GetAnimation(animData.animMap, currentAnimation_json->name);
        
        anim->name = currentAnimation_json->name;
        
        for (s32 i {}; i < anim->bones.Size(); ++i)
            anim->bones[i] = &skel.bones[i];
        
        Json* bonesOfAnimation = Json_getItem(currentAnimation_json, "bones");
        s32 boneIndex_json {};
        f32 maxTimeOfAnimation {};
        for (Json* currentBone = bonesOfAnimation ? bonesOfAnimation->child : 0; currentBone; currentBone = currentBone->next, ++boneIndex_json)
        {
            s32 boneIndex {};
            while (boneIndex < anim->bones.Size())
            {
                if (StringCmp(anim->bones[boneIndex]->name, currentBone->name))
                    break;
                else
                    ++boneIndex;
            };
            
            Json* rotateTimeline_json = Json_getItem(currentBone, "rotate");
            Json* translateTimeline_json = Json_getItem(currentBone, "translate");
            Json* scaleTimeline_json = Json_getItem(currentBone, "scale");
            
            if (rotateTimeline_json)
            {
                RotationTimeline* boneRotationTimeline = &anim->boneRotationTimelines[boneIndex];
                boneRotationTimeline->exists = true;
                boneRotationTimeline->GetTransformationVal = &GetTransformationVal_RotationTimeline;
                
                s32 keyFrameIndex {};
                for (Json* jsonKeyFrame = rotateTimeline_json ? rotateTimeline_json->child : 0; jsonKeyFrame; jsonKeyFrame = jsonKeyFrame->next, ++keyFrameIndex)
                {
                    boneRotationTimeline->times[boneRotationTimeline->timesCount++] = Json_getFloat(jsonKeyFrame, "time", 0.0f);
                    boneRotationTimeline->angles[boneRotationTimeline->anglesCount++] = Json_getFloat(jsonKeyFrame, "angle", 0.0f);
                    
                    const char* keyFrameCurve = Json_getString(jsonKeyFrame, "curve", "");
                    if (StringCmp(keyFrameCurve, "stepped"))
                        boneRotationTimeline->curves[boneRotationTimeline->curvesCount++] = CurveType::STEPPED;
                    else
                        boneRotationTimeline->curves[boneRotationTimeline->curvesCount++] = CurveType::LINEAR;
                };
                
                f32 maxTimeOfRotationTimeline = boneRotationTimeline->times[boneRotationTimeline->timesCount - 1];
                
                if (maxTimeOfRotationTimeline > maxTimeOfAnimation)
                    maxTimeOfAnimation = maxTimeOfRotationTimeline;
            };
            
            if (translateTimeline_json)
            {
                TranslationTimeline* boneTranslationTimeline = &anim->boneTranslationTimelines[boneIndex];
                boneTranslationTimeline->exists = true;
                boneTranslationTimeline->GetTransformationVal = &GetTransformationVal_TranslationTimeline;
                
                s32 keyFrameIndex {};
                for (Json* jsonKeyFrame = translateTimeline_json ? translateTimeline_json->child : 0; jsonKeyFrame; jsonKeyFrame = jsonKeyFrame->next, ++keyFrameIndex)
                {
                    boneTranslationTimeline->times[boneTranslationTimeline->timesCount++] = Json_getFloat(jsonKeyFrame, "time", 0.0f);
                    boneTranslationTimeline->translations[boneTranslationTimeline->translationCount++] = { 0.0f, 0.0f };
                    
                    boneTranslationTimeline->translations[keyFrameIndex].x = Json_getFloat(jsonKeyFrame, "x", 0.0f);
                    boneTranslationTimeline->translations[keyFrameIndex].y = Json_getFloat(jsonKeyFrame, "y", 0.0f);
                    
                    const char* keyFrameCurve = Json_getString(jsonKeyFrame, "curve", "");
                    if (StringCmp(keyFrameCurve, "stepped"))
                        boneTranslationTimeline->curves[boneTranslationTimeline->curvesCount++] = CurveType::STEPPED;
                    else
                        boneTranslationTimeline->curves[boneTranslationTimeline->curvesCount++] = CurveType::LINEAR;
                };
                
                f32 maxTimeOfTranslationTimeline = boneTranslationTimeline->times[boneTranslationTimeline->timesCount - 1];
                
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
                s32 hitBoxIndex {};
                for (Json* currentCollisionBox_json = collisionBoxesOfAnimation_json ? collisionBoxesOfAnimation_json->child : 0; currentCollisionBox_json; currentCollisionBox_json = currentCollisionBox_json->next, ++hitBoxIndex)
                {
                    anim->hitBoxes.Push() = HitBox {};
                    anim->hitBoxes[hitBoxIndex].boneName = CallocType(heap, char, 100);
                    
                    { //Get bone name collision box is attached to by cutting out "box-" prefix
                        char boneName[100] = {};
                        s32 j { 0 };
                        for (s32 i = 4; i < strlen(currentCollisionBox_json->name); ++i, ++j)
                            boneName[j] = currentCollisionBox_json->name[i];
                        
                        memcpy(anim->hitBoxes[hitBoxIndex].boneName, boneName, strlen(boneName));
                    };
                    
                    Json* collisionBoxTimeline_json = Json_getItem(currentCollisionBox_json, "attachment");
                    Json* keyFrame1_json = collisionBoxTimeline_json->child;
                    Json* keyFrame2_json = collisionBoxTimeline_json->child->next;
                    
                    f32 time1 = Json_getFloat(keyFrame1_json, "time", 0.0f);
                    f32 time2 = Json_getFloat(keyFrame2_json, "time", 0.0f);
                    
                    anim->hitBoxes[hitBoxIndex].timeUntilHitBoxIsActivated = time1;
                    anim->hitBoxes[hitBoxIndex].duration = time2 - time1;
                    
                    Temporary_Memory collisionVertsTemp = BeginTemporaryMemory($(memPart));
                    {
                        RunTimeArr<v2> adjustedCollisionBoxVerts, finalCollsionBoxVertCoords;
                        InitArr($(adjustedCollisionBoxVerts), &memPart, 20);
                        InitArr($(finalCollsionBoxVertCoords), &memPart, 20);
                        
                        Json* collisionBoxDeformTimeline_json = Json_getItem(currentAnimation_json, "deform");
                        Json* deformKeyFrame_json = collisionBoxDeformTimeline_json->child->child->child->child;
                        Json* deformedVerts_json = Json_getItem(deformKeyFrame_json, "vertices")->child;
                        
                        Bone* bone = GetBoneFromSkeleton(&skel, anim->hitBoxes[hitBoxIndex].boneName);
                        s32 numVerts = (s32)bone->originalCollisionBoxVerts.length;
                        for (s32 i {}; i < numVerts; ++i)
                        {
                            //Read in adjusted/deformed vert data from individual animation json info
                            adjustedCollisionBoxVerts.Push() = v2 { deformedVerts_json->valueFloat, deformedVerts_json->next->valueFloat };
                            deformedVerts_json = deformedVerts_json->next->next;
                            
                            //Transform original verts into new transformed vert positions based on anim deformed verts
                            v2 finalVertCoord = bone->originalCollisionBoxVerts[i] + adjustedCollisionBoxVerts[i];
                            finalCollsionBoxVertCoords.Push() = finalVertCoord;
                        };
                        
                        v2 vector0_1 = finalCollsionBoxVertCoords[0] - finalCollsionBoxVertCoords[1];
                        v2 vector1_2 = finalCollsionBoxVertCoords[1] - finalCollsionBoxVertCoords[2];
                        
                        anim->hitBoxes[hitBoxIndex].size.width = Magnitude(vector0_1);
                        anim->hitBoxes[hitBoxIndex].size.height = Magnitude(vector1_2);
                        anim->hitBoxes[hitBoxIndex].worldPosOffset = { (finalCollsionBoxVertCoords[0].x + finalCollsionBoxVertCoords[2].x) / 2.0f,
                            (finalCollsionBoxVertCoords[0].y + finalCollsionBoxVertCoords[2].y) / 2.0f };
                        
                        EndTemporaryMemory(collisionVertsTemp);
                    };
                }
            };
        }
        
        anim->totalTime = maxTimeOfAnimation;
    };
};

void MixAnimations(AnimationData&& animData, const char* animName_from, const char* animName_to, f32 mixDuration)
{
    Animation* anim_from = GetAnimation(animData.animMap, animName_from);
    
    Animation anim_to {};
    
    CopyAnimation(*GetAnimation(animData.animMap, animName_to), $(anim_to));
    
    BGZ_ASSERT(anim_from->totalTime > mixDuration, "passing a mix time that is too long!");
    
    anim_to.mixTimeDuration = mixDuration;
    
    if (anim_from->animsToTransitionTo.length > 0)
    {
        for (s32 i {}; i < anim_from->animsToTransitionTo.length; ++i)
        {
            BGZ_ASSERT(NOT StringCmp(anim_from->animsToTransitionTo[i]->name, anim_to.name), "Duplicate mix animation tyring to be set");
        };
        
        anim_from->animsToTransitionTo.Push() = MallocType(heap, Animation, 1);
        CopyAnimation(anim_to, $(*anim_from->animsToTransitionTo[anim_from->animsToTransitionTo.length - 1]));
    }
    else
    {
        anim_from->animsToTransitionTo.Push() = MallocType(heap, Animation, 1);
        CopyAnimation(anim_to, $(*anim_from->animsToTransitionTo[anim_from->animsToTransitionTo.length - 1]));
    }
};

void CopyAnimation(Animation src, Animation&& dest)
{
    dest = src;
    
    for (s32 boneIndex {}; boneIndex < src.bones.Size(); ++boneIndex)
    {
        CopyArray(src.boneTranslationTimelines[boneIndex].times, $(dest.boneTranslationTimelines[boneIndex].times));
        CopyArray(src.boneTranslationTimelines[boneIndex].translations, $(dest.boneTranslationTimelines[boneIndex].translations));
        CopyArray(src.boneRotationTimelines[boneIndex].times, $(dest.boneRotationTimelines[boneIndex].times));
        CopyArray(src.boneRotationTimelines[boneIndex].angles, $(dest.boneRotationTimelines[boneIndex].angles));
    };
};

void SetIdleAnimation(AnimationQueue&& animQueue, const AnimationData animData, const char* animName)
{
    Animation* sourceAnim = GetAnimation(animData.animMap, animName);
    
    Animation destAnim;
    CopyAnimation(*sourceAnim, $(destAnim));
    
    destAnim.status = PlayBackStatus::IDLE;
    animQueue.idleAnim = destAnim;
    
    animQueue.queuedAnimations.PushBack(destAnim);
};

void QueueAnimation(AnimationQueue&& animQueue, const AnimationData animData, const char* animName, PlayBackStatus playBackStatus)
{
    BGZ_ASSERT(playBackStatus != PlayBackStatus::IDLE, "Not suppose to set an IDLE status");
    
    Animation* sourceAnim = GetAnimation(animData.animMap, animName);
    
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
            case PlayBackStatus::DEFAULT: {
                animQueue.queuedAnimations.PushBack(destAnim);
            }
            break;
            
            case PlayBackStatus::IMMEDIATE: {
                animQueue.queuedAnimations.Reset();
                animQueue.queuedAnimations.PushBack(destAnim);
            }
            break;
            
            case PlayBackStatus::IMMEDIATE_NOREPEAT: {
                if(NOT StringCmp(animQueue.queuedAnimations.GetFirstElem()->name, animName))
                {
                    animQueue.queuedAnimations.Reset();
                    animQueue.queuedAnimations.PushBack(destAnim);
                };
                
            }break;
            
            case PlayBackStatus::NEXT: {
                animQueue.queuedAnimations.ClearRemaining();
                animQueue.queuedAnimations.PushBack(destAnim);
            }
            break;
            
            case PlayBackStatus::HOLD: {
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
s32 _CurrentActiveKeyFrame(TransformationTimelineType transformationTimelineOfBone, f32 currentAnimRuntime)
{
    BGZ_ASSERT(transformationTimelineOfBone.exists, "Trying to get keyframes from a timeline that does not exist");
    
    s32 result {};
    s32 keyFrameCount = (s32)transformationTimelineOfBone.timesCount - 1;
    
    f32 keyFrameTime0 {};
    f32 keyFrameTime1 = transformationTimelineOfBone.times[keyFrameCount];
    
    while (keyFrameCount)
    {
        keyFrameTime0 = transformationTimelineOfBone.times[keyFrameCount - 1];
        keyFrameTime1 = transformationTimelineOfBone.times[keyFrameCount];
        
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
    BGZ_ASSERT(transformationTimelineOfBone.timesCount != 0, "Can't get translations range from timeline w/ no keyframes!");
    
    TransformationRangeResult<TransformationType> result {};
    
    s32 firstKeyFrame { 0 }, lastKeyFrame { (s32)transformationTimelineOfBone.timesCount - 1 };
    if (transformationTimelineOfBone.timesCount == 1)
    {
        if (currentAnimRunTime >= transformationTimelineOfBone.times[firstKeyFrame])
        {
            result.transformation0 = transformationTimelineOfBone.GetTransformationVal(transformationTimelineOfBone, firstKeyFrame);
            result.transformation1 = result.transformation0;
            result.percentToLerp = 1.0f;
        };
    }
    else if (currentAnimRunTime >= transformationTimelineOfBone.times[firstKeyFrame] && currentAnimRunTime < transformationTimelineOfBone.times[lastKeyFrame])
    {
        s32 activeKeyFrameIndex = _CurrentActiveKeyFrame(transformationTimelineOfBone, currentAnimRunTime);
        BGZ_ASSERT(activeKeyFrameIndex != lastKeyFrame, "Should never be returning the last keyframe of timeline here!");
        
        switch (transformationTimelineOfBone.curves[activeKeyFrameIndex])
        {
            case CurveType::STEPPED: {
                result.transformation0 = transformationTimelineOfBone.GetTransformationVal(transformationTimelineOfBone, activeKeyFrameIndex);
                result.transformation1 = transformationTimelineOfBone.GetTransformationVal(transformationTimelineOfBone, activeKeyFrameIndex + 1);
                
                result.percentToLerp = 0.0f;
            }
            break;
            
            case CurveType::LINEAR: {
                result.transformation0 = transformationTimelineOfBone.GetTransformationVal(transformationTimelineOfBone, activeKeyFrameIndex);
                result.transformation1 = transformationTimelineOfBone.GetTransformationVal(transformationTimelineOfBone, activeKeyFrameIndex + 1);
                
                f32 time0 = transformationTimelineOfBone.times[activeKeyFrameIndex];
                f32 time1 = transformationTimelineOfBone.times[activeKeyFrameIndex + 1];
                
                f32 diff0 = time1 - time0;
                f32 diff1 = currentAnimRunTime - time0;
                result.percentToLerp = diff1 / diff0;
            }
            break;
            
            InvalidDefaultCase;
        }
    }
    else if (currentAnimRunTime >= transformationTimelineOfBone.times[lastKeyFrame])
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
    
    if ((boneRotationTimeline_originalAnim.exists && boneRotationTimeline_nextAnim.exists && boneRotationTimeline_nextAnim.times[0] > 0.0f) || (boneRotationTimeline_originalAnim.exists && NOT boneRotationTimeline_nextAnim.exists))
    {
        //Leave transformation1 at default 0 value
    }
    
    else if ((boneRotationTimeline_originalAnim.exists && boneRotationTimeline_nextAnim.exists) || (boneRotationTimeline_originalAnim.exists && NOT boneRotationTimeline_nextAnim.exists))
    {
        s32 firstKeyFrame_index = 0;
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
            
            for (s32 boneIndex {}; boneIndex < anim.bones.Size(); ++boneIndex)
            {
                anim.bones[boneIndex]->initialRotationForMixing = anim.boneRotations[boneIndex];
                anim.bones[boneIndex]->initialTranslationForMixing = anim.boneTranslations[boneIndex];
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
        
        v2 boneVector_frame0 = { boneLength * CosR(rotationRange.transformation0), boneLength * SinR(rotationRange.transformation0) };
        v2 boneVector_frame1 = { boneLength * CosR(rotationRange.transformation1), boneLength * SinR(rotationRange.transformation1) };
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
            for (s32 animIndex {}; animIndex < anim->animsToTransitionTo.length; ++animIndex)
            {
                if (StringCmp(anim->animsToTransitionTo[animIndex]->name, nextAnimInQueue->name))
                {
                    if (amountOfTimeLeftInAnim <= anim->animsToTransitionTo[animIndex]->mixTimeDuration)
                    {
                        InitializeMixingData($(*anim), prevFrameDT, amountOfTimeLeftInAnim);
                    }
                }
            };
        };
    };
    
    f32 maxTimeOfAnimation {};
    for (s32 boneIndex {}; boneIndex < anim->bones.Size(); ++boneIndex)
    {
        const Bone* bone = anim->bones[boneIndex];
        
        //Gather transformation timelines
        v2 amountOfTranslation { 0.0f, 0.0f };
        f32 amountOfRotation { 0.0f };
        TranslationTimeline translationTimelineOfBone = anim->boneTranslationTimelines[boneIndex];
        RotationTimeline rotationTimelineOfBone = anim->boneRotationTimelines[boneIndex];
        ScaleTimeline scaleTimelineOfBone = anim->boneScaleTimelines[boneIndex];
        
        Animation* nextAnimInQueue = animQueue.queuedAnimations.GetNextElem();
        
        { //Translation Timeline
            if (anim->MixingStarted)
            {
                BGZ_ASSERT(anim->animsToTransitionTo.length > 0, "No transition animation for mixing has been set!");
                
                TranslationTimeline nextAnimTranslationTimeline {};
                if (nextAnimInQueue)
                    nextAnimTranslationTimeline = nextAnimInQueue->boneTranslationTimelines[boneIndex];
                
                TransformationRangeResult<v2> translationRange = _GetTransformationRangeFromKeyFrames<v2, TranslationTimeline>(anim, translationTimelineOfBone, nextAnimTranslationTimeline, anim->currentTime, anim->bones[boneIndex]->initialTranslationForMixing);
                amountOfTranslation = Lerp(translationRange.transformation0, translationRange.transformation1, translationRange.percentToLerp);
            }
            else
            {
                if (translationTimelineOfBone.exists)
                {
                    TransformationRangeResult<v2> translationRange = _GetTransformationRangeFromKeyFrames<v2, TranslationTimeline>(translationTimelineOfBone, anim->currentTime);
                    amountOfTranslation = Lerp(translationRange.transformation0, translationRange.transformation1, translationRange.percentToLerp);
                };
            };
        }
        
        { //Rotation Timeline
            if (anim->MixingStarted)
            {
                BGZ_ASSERT(anim->animsToTransitionTo.length > 0, "No transition animation for mixing has been set!");
                
                RotationTimeline nextAnimRotationTimeline {};
                if (nextAnimInQueue)
                    nextAnimRotationTimeline = nextAnimInQueue->boneRotationTimelines[boneIndex];
                
                TransformationRangeResult<f32> rotationRange = _GetTransformationRangeFromKeyFrames<f32, RotationTimeline>(anim, rotationTimelineOfBone, nextAnimRotationTimeline, anim->currentTime, anim->bones[boneIndex]->initialRotationForMixing);
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
        
        anim->boneRotations[boneIndex] = amountOfRotation;
        anim->boneTranslations[boneIndex] = amountOfTranslation;
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
    
    for (s32 boneIndex {}; boneIndex < skel.bones.length; ++boneIndex)
    {
        f32 boneRotationToAdd = anim.boneRotations[boneIndex];
        skel.bones[boneIndex].parentBoneSpace.rotation += boneRotationToAdd;
        
        v2 boneTranslationToAdd = anim.boneTranslations[boneIndex];
        skel.bones[boneIndex].parentBoneSpace.translation += boneTranslationToAdd;
    };
};

#endif
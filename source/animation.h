#ifndef ANIMATION_INCLUDE
#define ANIMATION_INCLUDE

#include <string.h>
#include "skeleton.h"
#include "json.h"

/*
    1. Organize data in more of a SOA fashion?
    2. Properly handle timelines with only 1 key frame (especially when mixing animations)
    3. Currently every bone in the skeleton is iterated over in an animaiton. It's just that animatons
    don't always have bonetimelines existing on every bone
*/

class Timeline1
{
public:
    f32 time;
};

class TranslationTimeline : public Timeline1
{
public:
    v2f translation;
};

enum class CurveType
{
    LINEAR,
    STEPPED
};
struct KeyFrame
{
    f32 time{};
    f32 angle{};
    v2f translation{};
    CurveType curve{CurveType::LINEAR};
};

struct Timeline
{
    Timeline() = default;
    Timeline(Init) :
        keyFrames{0, KeyFrame{}, heap}
    {}
    Timeline(i64 size) :
        keyFrames{size, KeyFrame{}, heap}
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
    NEXT,
    HOLD
};

struct Animation
{
    Animation() = default;
    Animation(Init) :
        boneTimelineSets{heap},
        boneRotations{heap},
        boneTranslations{heap},
        bones{heap}
    {
        Reserve($(bones), 10);
    };

    const char* name{nullptr};
    f32 totalTime{};
    f32 currentTime{};
    f32 mixTimeDuration{};
    f32 currentMixTime{};
    PlayBackStatus status{PlayBackStatus::DEFAULT};
    b repeat{false};
    b hasEnded{false};
    Dynam_Array<Bone*> bones;
    HashMap_Str<TimelineSet> boneTimelineSets; 
    HashMap_Str<f32> boneRotations;
    HashMap_Str<v2f> boneTranslations;
    Animation* animToTransitionTo{nullptr};
    b MixingStarted{false};
    f32 mixTimeSnapShot{};
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

    Timeline1 timeline{};
    TranslationTimeline* transTimeline = (TranslationTimeline*)&timeline;

    transTimeline->translation.x = 3.0f;
    transTimeline->time = 3.0f;

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
            TimelineSet transformationTimelines{Init::_};

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
                    const char* keyFrameCurve = Json_getString(jsonKeyFrame, "curve", "");

                    if(!strcmp(keyFrameCurve, "stepped"))
                        keyFrame.curve = CurveType::STEPPED;

                    Insert($(rotationTimeline.keyFrames), keyFrame, keyFrameIndex);
                };

                transformationTimelines.rotationTimeline = rotationTimeline;

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

                transformationTimelines.translationTimeline = translateTimeline;

                f32 maxTimeOfTranslationTimeline = translateTimeline.keyFrames.At(translateTimeline.keyFrames.size - 1).time;
            
                if (maxTimeOfTranslationTimeline > maxTimeOfAnimation)
                    maxTimeOfAnimation = maxTimeOfTranslationTimeline;
            };

            animation.totalTime = maxTimeOfAnimation;
            Insert<TimelineSet>($(animation.boneTimelineSets), currentBone->name, transformationTimelines);
        };

        Insert<Animation>($(this->animations), animation.name, animation);
    };

    //Set bones of Animation
    for(i32 animIndex{}; animIndex < this->animations.keyInfos.size; ++animIndex)//TODO: Remove iteration, this is very slow!
    {
        Animation* anim = (Animation*)&this->animations.keyInfos.At(animIndex).value;

        if(anim->name)
        {
            for(i32 boneIndex{}; boneIndex < skel.bones.size; ++boneIndex)
            {
                PushBack($(anim->bones), &skel.bones.At(boneIndex));

                i32 hashIndex = GetHashIndex<TimelineSet>(anim->boneTimelineSets, skel.bones.At(boneIndex).name);
                if (hashIndex == HASH_DOES_NOT_EXIST)
                {
                    TimelineSet timelineSet;
                    timelineSet.rotationTimeline.exists = false;
                    timelineSet.translationTimeline.exists = false;
                    Insert<TimelineSet>($(anim->boneTimelineSets), skel.bones.At(boneIndex).name, timelineSet);
                };
            };
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
    Animation* anim_to = GetVal<Animation>(animData.animations, index_to, animName_to);
    BGZ_ASSERT(anim_from->totalTime > mixDuration, "passing a mix time that is too long!");

    anim_from->mixTimeDuration = mixDuration;
    anim_from->animToTransitionTo = anim_to;
};

void CleanUpAnimation(Animation&& anim)
{
    anim.name = {nullptr};
    anim.totalTime = {};
    anim.currentTime = {};
    anim.status = {PlayBackStatus::DEFAULT};
    anim.repeat = {false};
    anim.hasEnded = {false};

    CleanUpHashMap_Str($(anim.boneTranslations));
    CleanUpHashMap_Str($(anim.boneRotations));
    CleanUpHashMap_Str($(anim.boneTimelineSets));
};

void CopyAnimation(Animation src, Animation&& dest)
{
    dest = src;
    CopyArray(src.boneTimelineSets.keyInfos, $(dest.boneTimelineSets.keyInfos));
    CopyArray(src.boneRotations.keyInfos, $(dest.boneRotations.keyInfos));
    CopyArray(src.boneTranslations.keyInfos, $(dest.boneTranslations.keyInfos));
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

    Animation destAnim; 
    CopyAnimation(*sourceAnim, $(destAnim));

    destAnim.status = playBackStatus;

    if(NOT animQueue.queuedAnimations.full)
    {
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
                //Clear out animations not currently playing and insert new animation to play next
                if(NOT animQueue.queuedAnimations.Empty())
                {
                    animQueue.queuedAnimations.write = animQueue.queuedAnimations.read + 1;
                    animQueue.queuedAnimations.PushBack(destAnim);
                }
                else
                {
                    animQueue.queuedAnimations.PushBack(destAnim);
                }
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
i32 _CurrentActiveKeyFrame(Timeline timelineOfBone, f32 currentAnimRuntime)
{
    BGZ_ASSERT(timelineOfBone.exists, "Trying to get keyframes from a timeline that does not exist");

    i32 result{};
    i32 keyFrameCount = (i32)timelineOfBone.keyFrames.size - 1;

    KeyFrame keyFrame0{};
    KeyFrame keyFrame1 = timelineOfBone.keyFrames.At(keyFrameCount);

    while(keyFrameCount)
    {
        keyFrame0 = timelineOfBone.keyFrames.At(keyFrameCount - 1);
        keyFrame1 = timelineOfBone.keyFrames.At(keyFrameCount);

        if (keyFrame0.time < currentAnimRuntime && keyFrame1.time > currentAnimRuntime )                        
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

struct TranslationRangeResult
{
    v2f translation0{};
    v2f translation1{}; 
    f32 percentToLerp{};
};
TranslationRangeResult _GetTranslationRangeFromKeyFrames(Timeline translationTimelineOfBone, f32 currentAnimRunTime)
{
    BGZ_ASSERT(translationTimelineOfBone.keyFrames.size != 0, "Can't get translations range from timeline w/ no keyframes!");

    TranslationRangeResult result{};

    if(translationTimelineOfBone.keyFrames.size == 1)
    {
        if(currentAnimRunTime > translationTimelineOfBone.keyFrames.At(0).time)
        {
            result.translation0 = translationTimelineOfBone.keyFrames.At(0).translation;
            result.translation1 = translationTimelineOfBone.keyFrames.At(0).translation;
            result.percentToLerp = 1.0f;
        };
    }
    else if(currentAnimRunTime > translationTimelineOfBone.keyFrames.At(0).time && currentAnimRunTime < translationTimelineOfBone.keyFrames.At(translationTimelineOfBone.keyFrames.size - 1).time)
    {
        i32 activeKeyFrameIndex = _CurrentActiveKeyFrame(translationTimelineOfBone, currentAnimRunTime);
        result.translation0 = translationTimelineOfBone.keyFrames.At(activeKeyFrameIndex).translation;
        result.translation1 = translationTimelineOfBone.keyFrames.At(activeKeyFrameIndex + 1).translation;

        f32 time0 = translationTimelineOfBone.keyFrames.At(activeKeyFrameIndex).time;
        f32 time1 = translationTimelineOfBone.keyFrames.At(activeKeyFrameIndex + 1).time;

        f32 diff0 = time1 - time0;
        f32 diff1 = currentAnimRunTime - time0;
        result.percentToLerp = diff1 / diff0;
    }

    return result;
};

TranslationRangeResult _GetTranslationRangeFromKeyFrames(Animation* anim, Timeline boneTranslationTimeline_originalAnim, Timeline boneTranslationTimeline_nextAnim, f32 currentAnimRunTime, const char* boneName, i32 boneIndex)
{
    TranslationRangeResult result{};

    result.translation0 = anim->bones.At(boneIndex)->initialTranslationForMixing;

    if(boneTranslationTimeline_nextAnim.exists)
    {
        //Catches cases where we don't want to mix keyframes that are not at the start of the next animations timeline
        if(boneTranslationTimeline_nextAnim.keyFrames.At(0).time > 0.0f)
            return result;
    };

    if(boneTranslationTimeline_originalAnim.exists && boneTranslationTimeline_nextAnim.exists)
    {
        result.translation1 = boneTranslationTimeline_nextAnim.keyFrames.At(0).translation;
    }
    else if(boneTranslationTimeline_originalAnim.exists && NOT boneTranslationTimeline_nextAnim.exists)
    {
        result.translation1 = v2f{0.0f, 0.0f};
    }
    else if (NOT boneTranslationTimeline_originalAnim.exists && boneTranslationTimeline_nextAnim.exists)
    {
        result.translation1 = boneTranslationTimeline_nextAnim.keyFrames.At(0).translation;
    };

    result.percentToLerp = anim->currentMixTime / anim->mixTimeSnapShot;

    return result;
};

struct RotationRangeResult
{
    f32 angle0{};
    f32 angle1{}; 
    f32 percentToLerp{};
};
RotationRangeResult _GetRotationRangeFromKeyFrames(Timeline rotationTimelineOfBone, f32 currentAnimRunTime)
{
    BGZ_ASSERT(rotationTimelineOfBone.keyFrames.size != 0, "Can't get rotation range from timeline w/ no keyframes!");

    RotationRangeResult result{};

    if(rotationTimelineOfBone.keyFrames.size == 1)
    {
        if(currentAnimRunTime > rotationTimelineOfBone.keyFrames.At(0).time)
        {
            result.angle0 = rotationTimelineOfBone.keyFrames.At(0).angle;
            result.angle1 = rotationTimelineOfBone.keyFrames.At(0).angle;
            result.percentToLerp = 1.0f;
        };
    }
    else if(currentAnimRunTime > rotationTimelineOfBone.keyFrames.At(0).time && currentAnimRunTime < rotationTimelineOfBone.keyFrames.At(rotationTimelineOfBone.keyFrames.size - 1).time)
    {
        i32 activeKeyFrameIndex = _CurrentActiveKeyFrame(rotationTimelineOfBone, currentAnimRunTime);
        result.angle0 = rotationTimelineOfBone.keyFrames.At(activeKeyFrameIndex).angle;
        result.angle1 = rotationTimelineOfBone.keyFrames.At(activeKeyFrameIndex + 1).angle;

        f32 time0 = rotationTimelineOfBone.keyFrames.At(activeKeyFrameIndex).time;
        f32 time1 = rotationTimelineOfBone.keyFrames.At(activeKeyFrameIndex + 1).time;

        f32 diff0 = time1 - time0;
        f32 diff1 = currentAnimRunTime - time0;
        result.percentToLerp = diff1 / diff0;
    }

    return result;
};

RotationRangeResult _GetRotationRangeFromKeyFrames(Animation* anim, Timeline boneRotationTimeline_originalAnim, Timeline boneRotationTimeline_nextAnim, f32 currentAnimRunTime, const char* boneName, i32 boneIndex)
{
    RotationRangeResult result{};

    result.angle0 = anim->bones.At(boneIndex)->initialRotationForMixing;

    if(boneRotationTimeline_nextAnim.exists)
    {
        //Catches cases where we don't want to mix keyframes that are not at the start of the next animations timeline
        if(boneRotationTimeline_nextAnim.keyFrames.At(0).time > 0.0f)
            return result;
    };

    if(boneRotationTimeline_originalAnim.exists && boneRotationTimeline_nextAnim.exists)
    {
        result.angle1 = boneRotationTimeline_nextAnim.keyFrames.At(0).angle;
    }
    else if(boneRotationTimeline_originalAnim.exists && NOT boneRotationTimeline_nextAnim.exists)
    {
        result.angle1 = 0.0f;
    }
    else if (NOT boneRotationTimeline_originalAnim.exists && boneRotationTimeline_nextAnim.exists)
    {
        result.angle1 = boneRotationTimeline_nextAnim.keyFrames.At(0).angle;
    };

    result.percentToLerp = anim->currentMixTime / anim->mixTimeSnapShot;

    return result;
};

Animation UpdateAnimationState(AnimationQueue&& animQueue, f32 prevFrameDT)
{
    Animation* anim = animQueue.queuedAnimations.GetFirstElem();
    BGZ_ASSERT(anim, "No animation returned!");

    f32 prevFrameAnimTime = anim->currentTime;
    anim->currentTime += prevFrameDT;

    if(anim->currentTime > anim->totalTime)
    {
        f32 diff = anim->currentTime - prevFrameAnimTime;
        anim->currentTime -= diff;
        anim->hasEnded = true;
    };

    f32 amountOfTimeLeftInAnim = anim->totalTime - anim->currentTime;
    const Animation* nextAnimInQueue = animQueue.queuedAnimations.GetNextElem();
    if(nextAnimInQueue && amountOfTimeLeftInAnim <= anim->mixTimeDuration) 
    {
        if(!strcmp(anim->animToTransitionTo->name, nextAnimInQueue->name))
        {
            if(!strcmp(anim->animToTransitionTo->name, nextAnimInQueue->name))
            {
                f32 prevFrameMixTime = anim->currentMixTime;
                anim->currentMixTime += prevFrameDT;

                if(NOT anim->MixingStarted)
                {
                    anim->mixTimeSnapShot = amountOfTimeLeftInAnim;
                    anim->MixingStarted= true;

                    for (i32 boneIndex{}; boneIndex < anim->bones.size; ++boneIndex)
                    {
                        i32 translationIndex = GetHashIndex(anim->boneTranslations, anim->bones.At(boneIndex)->name);
                        i32 rotationIndex = GetHashIndex(anim->boneRotations, anim->bones.At(boneIndex)->name);

                        anim->bones.At(boneIndex)->initialTranslationForMixing = *GetVal($(anim->boneTranslations), translationIndex, anim->bones.At(boneIndex)->name);
                        anim->bones.At(boneIndex)->initialRotationForMixing = *GetVal($(anim->boneRotations), rotationIndex, anim->bones.At(boneIndex)->name);
                    }
                }

                if(anim->currentMixTime > anim->mixTimeSnapShot)
                {
                    anim->currentMixTime = anim->mixTimeSnapShot;
                    anim->hasEnded = true;
                }
            }
        }
    };

    f32 maxTimeOfAnimation{};
    for (i32 boneIndex{}; boneIndex < anim->bones.size; ++boneIndex)
    {
        /*
            boneTranslationTimeline = bone->translationTimeline;

            boneRotationTimeline = boneTranslationTimeline->rotaiotnTimeline;
            boneTranslationTimeline = boneTranslationTimeline->translaitontimeline;

            if(anim->currentMixtime > 0.0f)
            {
                Timeline nextAnimTranslationTimeline = nextAnimTransformationTimelines->translationTimeline;
                boneTranslationTimeline 

                Timeline nextAnimTranslationTimeline = nextAnimTransformationTimelines->translationTimeline;
                boneRotationTimeline
            }
            else if(anim->ucrrenttime > 0.0f)
            {

            }


            Timeline* timeline

            switch(timeline->type)
            {
                case Rotation:
                {
                    f32 amountOfRotation{0.0f};
                    if(anim->currentMixTime > 0.0f)
                    {
                        TranslationRangeResult translationRange = _GetTranslationRangeFromKeyFrames(anim, translationTimelineOfBone, nextAnimTranslationTimeline, anim->currentTime, bone->name, boneIndex);
                        amountOfTranslation = Lerp(translationRange.translation0, translationRange.translation1, translationRange.percentToLerp);
                    }
                    else if(anim.currenttime > 0.0f)
                    {

                    }

                    Insert<f32>($(anim->boneRotations), bone->name, amountOfRotation);
                };

                case Translation:
                {
                    ........
                };
            }


        */

        const Bone* bone = anim->bones.At(boneIndex);

        //Gather transformation timelines
        i32 hashIndex = GetHashIndex<TimelineSet>(anim->boneTimelineSets, bone->name);
        BGZ_ASSERT(hashIndex != -1, "TimelineSet not found!");
        TimelineSet* transformationTimelines = GetVal<TimelineSet>(anim->boneTimelineSets, hashIndex, bone->name);
        TimelineSet* nextAnimTransformationTimelines{};
        if(anim->animToTransitionTo)
        {
            i32 hashIndex = GetHashIndex<TimelineSet>(anim->animToTransitionTo->boneTimelineSets, bone->name);
            BGZ_ASSERT(hashIndex != -1, "TimelineSet not found!");
            nextAnimTransformationTimelines = GetVal<TimelineSet>(anim->animToTransitionTo->boneTimelineSets, hashIndex, bone->name);
        };

        v2f amountOfTranslation{0.0f, 0.0f};
        f32 amountOfRotation{0.0f};
        if(anim->currentMixTime > 0.0f)
        {
            {//Translation mixing
                Timeline translationTimelineOfBone = transformationTimelines->translationTimeline;
                Timeline nextAnimTranslationTimeline = nextAnimTransformationTimelines->translationTimeline;

                TranslationRangeResult translationRange = _GetTranslationRangeFromKeyFrames(anim, translationTimelineOfBone, nextAnimTranslationTimeline, anim->currentTime, bone->name, boneIndex);
                amountOfTranslation = Lerp(translationRange.translation0, translationRange.translation1, translationRange.percentToLerp);
            };

            {//Rotation mixing
                Timeline rotationTimelineOfBone = transformationTimelines->rotationTimeline;
                Timeline nextAnimRotationTimeline = nextAnimTransformationTimelines->rotationTimeline;

                RotationRangeResult rotationRange = _GetRotationRangeFromKeyFrames(anim, rotationTimelineOfBone, nextAnimRotationTimeline, anim->currentTime, bone->name, boneIndex);

                v2f boneVector_frame0 = { bone->length * CosR(rotationRange.angle0), bone->length * SinR(rotationRange.angle0) };
                v2f boneVector_frame1 = { bone->length * CosR(rotationRange.angle1), bone->length * SinR(rotationRange.angle1) };
                f32 directionOfRotation = CrossProduct(boneVector_frame0, boneVector_frame1);

                if (directionOfRotation > 0) //Rotate counter-clockwise
                {
                    if (rotationRange.angle0 < rotationRange.angle1)
                    {
                        amountOfRotation = Lerp(rotationRange.angle0, rotationRange.angle1, rotationRange.percentToLerp);
                    }
                    else
                    {
                        ConvertPositiveToNegativeAngle_Radians($(rotationRange.angle0));
                        amountOfRotation = Lerp(rotationRange.angle0, rotationRange.angle1, rotationRange.percentToLerp);
                    }
                }
                else //Rotate clockwise
                {
                    if (rotationRange.angle0 < rotationRange.angle1)
                    {
                        ConvertPositiveToNegativeAngle_Radians($(rotationRange.angle1));
                        amountOfRotation = Lerp(rotationRange.angle0, rotationRange.angle1, rotationRange.percentToLerp);
                    }
                    else
                    {
                        amountOfRotation = Lerp(rotationRange.angle0, rotationRange.angle1, rotationRange.percentToLerp);
                    }
                }
            };
        }
        else if (anim->currentTime > 0.0f)
        {
            Timeline translationTimelineOfBone = transformationTimelines->translationTimeline;
            Timeline rotationTimelineOfBone = transformationTimelines->rotationTimeline;

            if(translationTimelineOfBone.exists)
            {
                TranslationRangeResult translationRange = _GetTranslationRangeFromKeyFrames(translationTimelineOfBone, anim->currentTime);
                amountOfTranslation = Lerp(translationRange.translation0, translationRange.translation1, translationRange.percentToLerp);
            };

            if(rotationTimelineOfBone.exists)
            {
                RotationRangeResult rotationRange = _GetRotationRangeFromKeyFrames(rotationTimelineOfBone, anim->currentTime);

                v2f boneVector_frame0 = { bone->length * CosR(rotationRange.angle0), bone->length * SinR(rotationRange.angle0) };
                v2f boneVector_frame1 = { bone->length * CosR(rotationRange.angle1), bone->length * SinR(rotationRange.angle1) };
                f32 directionOfRotation = CrossProduct(boneVector_frame0, boneVector_frame1);

                if (directionOfRotation > 0) //Rotate counter-clockwise
                {
                    if (rotationRange.angle0 < rotationRange.angle1)
                    {
                        amountOfRotation = Lerp(rotationRange.angle0, rotationRange.angle1, rotationRange.percentToLerp);
                    }
                    else
                    {
                        ConvertPositiveToNegativeAngle_Radians($(rotationRange.angle0));
                        amountOfRotation = Lerp(rotationRange.angle0, rotationRange.angle1, rotationRange.percentToLerp);
                    }
                }
                else //Rotate clockwise
                {
                    if (rotationRange.angle0 < rotationRange.angle1)
                    {
                        ConvertPositiveToNegativeAngle_Radians($(rotationRange.angle1));
                        amountOfRotation = Lerp(rotationRange.angle0, rotationRange.angle1, rotationRange.percentToLerp);
                    }
                    else
                    {
                        amountOfRotation = Lerp(rotationRange.angle0, rotationRange.angle1, rotationRange.percentToLerp);
                    }
                }
            };
        }

        i32 keyFrameCount{};
        KeyFrame keyFrame0{}, keyFrame1{};
        f32 diff{}, diff1{}, percentToLerp{};
#if 0
        if(anim->currentTime > 0.0f && rotationTimelineOfBone.keyFrames.size != 1)
        {
            i32 activeKeyFrame_index = _CurrentActiveKeyFrame(rotationTimelineOfBone, anim->currentTime);

            if(readyToMix)
            {
                i32 hashIndex = GetHashIndex<TimelineSet>(anim->animToTransitionTo->boneTimelineSets, bone->name);

                if(hashIndex != -1)
                {
                    TimelineSet* transformationTimelines = GetVal<TimelineSet>(anim->animToTransitionTo->boneTimelineSets, hashIndex, bone->name);
                    Timeline nextAnimRotationTimeline = transformationTimelines->rotationTimeline;

                    if(nextAnimRotationTimeline.exists)
                    {
                        keyFrame0 = rotationTimelineOfBone.keyFrames.At(activeKeyFrame_index);
                        keyFrame1 = nextAnimRotationTimeline.keyFrames.At(0);

                        f32 newZero = rotationTimelineOfBone.keyFrames.At(activeKeyFrame_index + 1).time - anim->mixTimeSnapShot;
                        diff1 = anim->currentTime - newZero;
                        percentToLerp = diff1 / rotationTimelineOfBone.keyFrames.At(activeKeyFrame_index + 1).time;

                        i32 index = GetHashIndex<TimelineSet>(anim->animToTransitionTo->boneTimelineSets, bone->name);
                        f32 oldAmountOfRotation = *GetVal($(anim->boneRotations), index, bone->name);

                        v2f boneVector_frame0 = { bone->length * CosR(keyFrame0.angle), bone->length * SinR(keyFrame0.angle) };
                        v2f boneVector_frame1 = { bone->length * CosR(keyFrame1.angle), bone->length * SinR(keyFrame1.angle) };
                        f32 directionOfRotation = CrossProduct(boneVector_frame0, boneVector_frame1);

                        if (directionOfRotation > 0) //Rotate counter-clockwise
                        {
                            if (oldAmountOfRotation < keyFrame1.angle)
                            {
                                amountOfRotation = Lerp(oldAmountOfRotation , keyFrame1.angle, percentToLerp);
                            }
                            else
                            {
                                ConvertPositiveToNegativeAngle_Radians($(oldAmountOfRotation));
                                amountOfRotation = Lerp(oldAmountOfRotation, keyFrame1.angle, percentToLerp);
                            }
                        }
                        else //Rotate clockwise
                        {
                            if (oldAmountOfRotation < keyFrame1.angle)
                            {
                                ConvertPositiveToNegativeAngle_Radians($(keyFrame1.angle));
                                amountOfRotation = Lerp(oldAmountOfRotation, keyFrame1.angle, percentToLerp);
                            }
                            else
                            {
                                amountOfRotation = Lerp(oldAmountOfRotation, keyFrame1.angle, percentToLerp);
                            }
                        }
                    };
                }
                else
                {
                    keyFrame0 = rotationTimelineOfBone.keyFrames.At(activeKeyFrame_index);
                    keyFrame1 = rotationTimelineOfBone.keyFrames.At(activeKeyFrame_index + 1);

                    ConvertNegativeToPositiveAngle_Radians($(keyFrame0.angle));
                    ConvertNegativeToPositiveAngle_Radians($(keyFrame1.angle));

                    f32 diff = keyFrame1.time - keyFrame0.time;
                    f32 diff1 = anim->currentTime - keyFrame0.time;
                    f32 percentToLerp = diff1 / diff;

                    v2f boneVector_frame0 = { bone->length * CosR(keyFrame0.angle), bone->length * SinR(keyFrame0.angle) };
                    v2f boneVector_frame1 = { bone->length * CosR(keyFrame1.angle), bone->length * SinR(keyFrame1.angle) };
                    f32 directionOfRotation = CrossProduct(boneVector_frame0, boneVector_frame1);

                    if (directionOfRotation > 0) //Rotate counter-clockwise
                    {
                        if (keyFrame0.angle < keyFrame1.angle)
                        {
                            amountOfRotation = Lerp(keyFrame0.angle, keyFrame1.angle, percentToLerp);
                        }
                        else
                        {
                            ConvertPositiveToNegativeAngle_Radians($(keyFrame0.angle));
                            amountOfRotation = Lerp(keyFrame0.angle, keyFrame1.angle, percentToLerp);
                        }
                    }
                    else //Rotate clockwise
                    {
                        if (keyFrame0.angle < keyFrame1.angle)
                        {
                            ConvertPositiveToNegativeAngle_Radians($(keyFrame1.angle));
                            amountOfRotation = Lerp(keyFrame0.angle, keyFrame1.angle, percentToLerp);
                        }
                        else
                        {
                            amountOfRotation = Lerp(keyFrame0.angle, keyFrame1.angle, percentToLerp);
                        }
                    }
                }
            }
            else
            {
                keyFrame0 = rotationTimelineOfBone.keyFrames.At(activeKeyFrame_index);
                keyFrame1 = rotationTimelineOfBone.keyFrames.At(activeKeyFrame_index + 1);

                ConvertNegativeToPositiveAngle_Radians($(keyFrame0.angle));
                ConvertNegativeToPositiveAngle_Radians($(keyFrame1.angle));

                //Find percent to lerp
                f32 diff = keyFrame1.time - keyFrame0.time;
                f32 diff1 = anim->currentTime - keyFrame0.time;
                f32 percentToLerp = diff1 / diff;

                v2f boneVector_frame0 = { bone->length * CosR(keyFrame0.angle), bone->length * SinR(keyFrame0.angle) };
                v2f boneVector_frame1 = { bone->length * CosR(keyFrame1.angle), bone->length * SinR(keyFrame1.angle) };
                f32 directionOfRotation = CrossProduct(boneVector_frame0, boneVector_frame1);

                if (directionOfRotation > 0) //Rotate counter-clockwise
                {
                    if (keyFrame0.angle < keyFrame1.angle)
                    {
                        amountOfRotation = Lerp(keyFrame0.angle, keyFrame1.angle, percentToLerp);
                    }
                    else
                    {
                        ConvertPositiveToNegativeAngle_Radians($(keyFrame0.angle));
                        amountOfRotation = Lerp(keyFrame0.angle, keyFrame1.angle, percentToLerp);
                    }
                }
                else //Rotate clockwise
                {
                    if (keyFrame0.angle < keyFrame1.angle)
                    {
                        ConvertPositiveToNegativeAngle_Radians($(keyFrame1.angle));
                        amountOfRotation = Lerp(keyFrame0.angle, keyFrame1.angle, percentToLerp);
                    }
                    else
                    {
                        amountOfRotation = Lerp(keyFrame0.angle, keyFrame1.angle, percentToLerp);
                    }
                }
            }
            
            if(keyFrame0.curve == CurveType::STEPPED)
            {
                if(anim->currentTime < keyFrame1.time)
                    amountOfRotation = keyFrame0.angle;
                else
                    amountOfRotation = keyFrame1.angle;
            }
            else if(keyFrame0.curve == CurveType::LINEAR)
            {
                //Find percent to lerp
            };
        };

        if(rotationTimelineOfBone.keyFrames.size == 1)
            amountOfRotation = rotationTimelineOfBone.keyFrames.At(0).angle;
#endif 

        Insert<f32>($(anim->boneRotations), bone->name, amountOfRotation);
        Insert<v2f>($(anim->boneTranslations), bone->name, amountOfTranslation);
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
        i32 hashIndex = GetHashIndex(anim.boneRotations, skel.bones.At(boneIndex).name);

        if (hashIndex != HASH_DOES_NOT_EXIST)
        {
                f32 boneRotationToAdd = *GetVal(anim.boneRotations, hashIndex, skel.bones.At(boneIndex).name);
                *skel.bones.At(boneIndex).parentLocalRotation += boneRotationToAdd;
        };

        hashIndex = GetHashIndex(anim.boneTranslations, skel.bones.At(boneIndex).name);

        if (hashIndex != HASH_DOES_NOT_EXIST)
        {
            v2f boneTranslationToAdd = *GetVal(anim.boneTranslations, hashIndex, skel.bones.At(boneIndex).name);
            *skel.bones.At(boneIndex).parentLocalPos += boneTranslationToAdd;
        };
    };
};

#endif //ANIMATION_IMPL
#ifndef ANIMATION_INCLUDE
#define ANIMATION_INCLUDE

#include <string.h>
#include "skeleton.h"
#include "json.h"

/*
    API ideas:
    1. Should you call rotate, translate, scale timelines? 
    2. Organize data in more of a SOA fashion?
    3. Right now, code expects all animations start with with their keyframes set to the setup pose
    bone positions. Might want to eventually change this given this is a waste of keyframes. Fix might 
    involve mixing between idle anim and current playing anim or something. 
*/

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
        boneTranslations{heap}
    {};

    const char* name{nullptr};
    f32 totalTime{};
    f32 currentTime{};
    HashMap_Str<TimelineSet> boneTimelineSets; 
    HashMap_Str<f32> boneRotations;
    HashMap_Str<v2f> boneTranslations;
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
                    const char* keyFrameCurve = Json_getString(jsonKeyFrame, "curve", "");

                    if(!strcmp(keyFrameCurve, "stepped"))
                        keyFrame.curve = CurveType::STEPPED;

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

//Returns lower keyFrame of range(e.g. if range is between 0 - 1 then keyFrame number 0 is returned)
i32 _CurrentActiveKeyFrame(Timeline timelineOfBone, f32 currentAnimRuntime)
{
    i32 result{};
    i32 keyFrameCount = (i32)timelineOfBone.keyFrames.size - 1;

    KeyFrame keyFrame0{};
    KeyFrame keyFrame1 = timelineOfBone.keyFrames.At(keyFrameCount);

    //This catches the case where on prev frame current anim runtime was less than
    //the max time of current timeline but on this frame current anim runtime is no more.
    //By having calling function get back the final keyframe on the time line this can 
    //make sure the lerp finishes through on it's animation if there was still some 
    //work left to do (This helps prevent subtle bugs currently) 
    if(keyFrame1.time < currentAnimRuntime)
        return keyFrameCount - 1;

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

                TimelineSet* timelineSet = GetVal<TimelineSet>(anim->boneTimelineSets, hashIndex, bones->At(boneIndex).name);

                Timeline rotationTimelineOfBone = timelineSet->rotationTimeline;
                i32 keyFrameCount{};
                if(rotationTimelineOfBone.exists)
                {
                    f32 amountOfRotation{0.0f};
                    if(rotationTimelineOfBone.keyFrames.size == 1)
                    {
                        amountOfRotation = rotationTimelineOfBone.keyFrames.At(0).angle;
                    }
                    else if (anim->currentTime > 0.0f) 
                    {
                        i32 activeKeyFrame_index = _CurrentActiveKeyFrame(rotationTimelineOfBone, anim->currentTime);

                        KeyFrame keyFrame0 = rotationTimelineOfBone.keyFrames.At(activeKeyFrame_index);
                        KeyFrame keyFrame1 = rotationTimelineOfBone.keyFrames.At(activeKeyFrame_index + 1);

                        ConvertNegativeToPositiveAngle_Radians($(keyFrame0.angle));
                        ConvertNegativeToPositiveAngle_Radians($(keyFrame1.angle));

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
                        };
                    };

                    Insert<f32>($(anim->boneRotations), bone->name, amountOfRotation);
                };

                Timeline translationTimelineOfBone = timelineSet->translationTimeline;
                if(translationTimelineOfBone.exists)
                {
                    v2f amountOfTranslation{0.0f, 0.0f};
                    if(translationTimelineOfBone.keyFrames.size == 1) 
                    {
                        amountOfTranslation = translationTimelineOfBone.keyFrames.At(0).translation;
                    }
                    else if (anim->currentTime > 0.0f)
                    {
                        i32 activeKeyFrame_index = _CurrentActiveKeyFrame(translationTimelineOfBone, anim->currentTime);

                        KeyFrame keyFrame0 = translationTimelineOfBone.keyFrames.At(activeKeyFrame_index);
                        KeyFrame keyFrame1 = translationTimelineOfBone.keyFrames.At(activeKeyFrame_index + 1);

                        if(keyFrame0.curve == CurveType::STEPPED)
                        {
                            if(anim->currentTime < keyFrame1.time)
                                amountOfTranslation = keyFrame0.translation;
                            else
                                amountOfTranslation = keyFrame1.translation;
                        }
                        else if(keyFrame0.curve == CurveType::LINEAR)
                        {
                            //Find percent to lerp
                            f32 diff = keyFrame1.time - keyFrame0.time;
                            f32 diff1 = anim->currentTime - keyFrame0.time;
                            f32 percentToLerp = diff1 / diff;

                            amountOfTranslation = Lerp(keyFrame0.translation, keyFrame1.translation, percentToLerp);
                        };
                    };

                    Insert<v2f>($(anim->boneTranslations), bone->name, amountOfTranslation);
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
        ResetBonesToSetupPose($(skel));

        for (i32 boneIndex{}; boneIndex < skel.bones.size; ++boneIndex)
        {
            i32 hashIndex = GetHashIndex(anim->boneRotations, skel.bones.At(boneIndex).name);

            if (hashIndex != HASH_DOES_NOT_EXIST)
            {
                f32 boneRotationToAdd = *GetVal(anim->boneRotations, hashIndex, skel.bones.At(boneIndex).name);
                if(boneRotationToAdd == 0)
                    int x{3}; 
                *skel.bones.At(boneIndex).parentLocalRotation += boneRotationToAdd;
            };

            hashIndex = GetHashIndex(anim->boneTranslations, skel.bones.At(boneIndex).name);

            if (hashIndex != HASH_DOES_NOT_EXIST)
            {
                v2f boneTranslationToAdd = *GetVal(anim->boneTranslations, hashIndex, skel.bones.At(boneIndex).name);
                if(boneTranslationToAdd == v2f{0, 0})
                    int x{3}; 
                *skel.bones.At(boneIndex).parentLocalPos += boneTranslationToAdd;
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
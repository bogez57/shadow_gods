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
*/

struct Animation
{
    const char* name;
    const char* boneName;
    Dynam_Array<f32> times;
    Dynam_Array<f32> angles;
    f32 time;
    i32 count;
    b startAnimation{false};
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

    anim.times.Init(4, heap);
    anim.angles.Init(4, heap);

    Json* animations = Json_getItem(root, "animations"); /* clang-format off */BGZ_ASSERT(animations, "Unable to return valid json object!"); /* clang-format on */

    for(Json* currentAnimation = animations ? animations->child : 0; currentAnimation; currentAnimation = currentAnimation->next)
    {
        Json* bonesOfAnimation = currentAnimation->child;
        for(Json* currentBone = bonesOfAnimation ? bonesOfAnimation->child : 0; currentBone; currentBone = currentBone->next)
        {
            Json* rotate = Json_getItem(currentBone, "rotate");

            i32 animDataIndex{};
            for(Json* animData = rotate ? rotate->child : 0; animData; animData = animData->next, ++animDataIndex)
            {
                anim.times.Insert(Json_getFloat(animData, "time", 0.0f), animDataIndex);
                anim.angles.Insert(Json_getFloat(animData, "angle", 0.0f), animDataIndex);
            };
        };
    };

#if 0
    Json* testAnim = Json_getItem(animations, "test");
    Json* bones = Json_getItem(testAnim, "bones");
    Json* currentBone = Json_getItem(bones, "right-shoulder");
    anim.boneName = currentBone->name;
    Json* rotate = currentBone->child;

    i32 index{};
    for(Json* animDataSet = rotate->child; index < 4; animDataSet = animDataSet->next, ++index)
    {
        anim.times.At(index) = Json_getFloat(animDataSet, "time", 0.0f);
        anim.angles.At(index) = Json_getFloat(animDataSet, "angle", 0.0f);

        //Translate to game units
        anim.angles.At(index) = Radians(anim.angles.At(index));
    };
#endif
};

void StartAnimation(Animation&& anim)
{
    anim.startAnimation = true;
};

void UpdateSkeletonAnimation(Skeleton&& skel, Animation&& anim, f32 prevFrameDT)
{
    if(anim.startAnimation)
        anim.time += prevFrameDT;

    Bone* bone = FindBone(&skel, anim.boneName);

    f32 rotation0 = bone->originalParentLocalRotation + anim.angles.At(0);
    f32 rotation1 = bone->originalParentLocalRotation + anim.angles.At(1);
    f32 rotation2 = bone->originalParentLocalRotation + anim.angles.At(2);
    f32 rotation3 = bone->originalParentLocalRotation + anim.angles.At(3);

    local_persist b firstTime;

    if(anim.time > anim.times.At(3))
    {
        anim.count = 0;
        anim.time = 0.0f;
        anim.startAnimation = false;
    }

    f32 t{};
    f32 lerpedRotation{};
    if(anim.times.At(0) <= anim.time)
    {
        f32 diff = anim.times.At(1) - anim.times.At(0);
        f32 diff1 = anim.time - anim.times.At(0);

        t = diff1 / diff;

        lerpedRotation = Lerp(rotation0, rotation1, t);
    };

    if(anim.times.At(1) <= anim.time)
    {
        f32 diff = anim.times.At(2) - anim.times.At(1);
        f32 diff1 = anim.time - anim.times.At(1);

        t = diff1 / diff;

        lerpedRotation = Lerp(rotation1, rotation2, t);
    };

    if(anim.times.At(2) <= anim.time)
    {
        f32 diff = anim.times.At(3) - anim.times.At(2);
        f32 diff1 = anim.time - anim.times.At(2);

        t = diff1 / diff;

        lerpedRotation = Lerp(rotation2, rotation3, t);
    };

    *bone->parentLocalRotation = lerpedRotation;
};

#endif //ANIMATION_IMPL
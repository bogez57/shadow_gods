#ifndef ANIMATION_INCLUDE_H
#define ANIMATION_INCLUDE_H

#include <string.h>
#include "skeleton.h"
#include "json.h"

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

#if 0
    for(Json* currentAnimation = animations->child; currentAnimation != nullptr; currentAnimation = currentAnimation->next)
    {
        Json* bonesOfAnimation = currentAnimation->child;
        for(Json* currentBone = bonesOfAnimation->child; currentBone != nullptr; currentBone = currentBone->next)
        {
            Json* rotate = Json_getItem(currentBone, "rotate");

            i32 animDataIndex{};
            for(Json* animData = rotate->child; animData != nullptr; animData = animData->next, ++animDataIndex)
            {
                anim.times.PushBack(Json_getFloat(animData, "time", 0.0f));
                anim.angles.PushBack(Json_getFloat(animData, "angle", 0.0f));
            };
        };
    };
#endif 

    Json* testAnim = Json_getItem(animations, "test");
    Json* bones = Json_getItem(testAnim, "bones");
    Json* currentBone = Json_getItem(bones, "right-forearm");
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
        firstTime = true;
    }

    f32 t{};
    f32 lerpedRotation{};
    if(anim.times.At(0) <= anim.time)
    {
        t = anim.time / anim.times.At(anim.count + 1);
        lerpedRotation = Lerp(rotation0, rotation1, t);
    };

    if(anim.times.At(1) <= anim.time)
    {
        if(firstTime)
        {
            t = 1.0f;
            lerpedRotation = Lerp(rotation0, rotation1, t);
            firstTime = false;
        }
        else
        {
            t = anim.time / anim.times.At(anim.count + 2);
            lerpedRotation = Lerp(rotation1, rotation2, t);
        }
    };

    if(anim.times.At(2) <= anim.time)
    {
        t = anim.time / anim.times.At(anim.count + 3);
        lerpedRotation = Lerp(rotation2, rotation3, t);
    };

    *bone->parentLocalRotation = lerpedRotation;
};

#endif //ANIMATION_IMPL
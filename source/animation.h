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

    anim.times.Init(3, heap);
    anim.angles.Init(3, heap);

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
    for(Json* animDataSet = rotate->child; index < 3; animDataSet = animDataSet->next, ++index)
    {
        anim.times.At(index) = Json_getFloat(animDataSet, "time", 0.0f);
        anim.angles.At(index) = Json_getFloat(animDataSet, "angle", 0.0f);

        //Translate to game units
        anim.angles.At(index) = Radians(anim.angles.At(index));
    };
};

void UpdateSkeletonAnimation(Skeleton&& skel, Animation anim)
{
    Bone* bone = FindBone(&skel, anim.boneName);
    *bone->parentLocalRotation = bone->originalParentLocalRotation + anim.angles.At(1);
};

#endif //ANIMATION_IMPL
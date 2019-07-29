#ifndef ANIMATION_INCLUDE_H
#define ANIMATION_INCLUDE_H

#include <string.h>
#include "json.h"

struct Animation
{
    char* name;
    i32 time;
    f32 angle;
};

#endif

#ifdef ANIMATION_IMPL

void CreateAnimationFromJsonFile(Animation&& anim, const char* jsonFilePath)
{
    i32 length;

    const char* jsonFile = globalPlatformServices->ReadEntireFile($(length), jsonFilePath);


    Json* root {};
    root = Json_create(jsonFile);

    Json* animations = Json_getItem(root, "animations"); /* clang-format off */BGZ_ASSERT(animations, "Unable to return valid json object!"); /* clang-format on */
    Json* test = Json_getItem(animations, "test");
};

#endif //ANIMATION_IMPL
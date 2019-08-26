#ifndef FIGHTER_INCLUDE
#define FIGHTER_INCLUDE

#include "skeleton.h"
#include "animation.h"

struct Fighter
{
    Fighter() = default;
    Fighter(const char* atlasFilePath, const char* jsonFilePath, v2f worldPos);

    v2f* worldPos{nullptr};
    Transform world;
    f32 height{};
    Skeleton skel;
    AnimationQueue animQueue;
    AnimationData animData;
};

#endif

#ifdef FIGHTER_IMPL

Fighter::Fighter(const char* atlasFilePath, const char* jsonFilePath, v2f worldPos) :
    animData{jsonFilePath},
    skel{atlasFilePath, jsonFilePath, 20, 19, heap},
    animQueue{Init::_},
    world{0.0f, worldPos, {1.0f, 1.0f}}
{
    this->skel.worldPos = &this->world.translation;
    this->worldPos = &this->world.translation;
};

#endif //FIGHTER_IMPL
#ifndef FIGHTER_INCLUDE
#define FIGHTER_INCLUDE

#include "skeleton.h"
#include "animation.h"

struct Fighter
{
    Fighter() = default;
    Fighter(const char* atlasFilePath, const char* jsonFilePath, v2f worldPos, f32 fighterHeight);

    Transform world;
    f32 height{};
    Skeleton skel;
    AnimationQueue animQueue;
    AnimationData animData;
    Animation* currentAnim{nullptr};
};

#endif

#ifdef FIGHTER_IMPL

Fighter::Fighter(const char* atlasFilePath, const char* jsonFilePath, v2f worldPos, f32 fighterHeight) :
    animData{jsonFilePath},
    skel{atlasFilePath, jsonFilePath, heap},
    animQueue{Init::_},
    world{0.0f, worldPos, {1.0f, 1.0f}},
    height{fighterHeight}
{
    f32 scaleFactor{};
    { //Change fighter height
        f32 aspectRatio = this->skel.height / this->skel.width;
        scaleFactor = this->height / this->skel.height;

        //New fighter height
        this->skel.height = this->height;
        this->skel.width = aspectRatio * this->height;

        for (i32 boneIndex{}; boneIndex < this->skel.bones.size; ++boneIndex)
        {
            if(!strcmp(this->skel.bones.At(boneIndex).name, "left-shoulder"))
                int x{3};

            this->skel.bones.At(boneIndex).transform.translation.x *= scaleFactor;
            this->skel.bones.At(boneIndex).transform.translation.y *= scaleFactor;
            this->skel.bones.At(boneIndex).originalParentLocalPos *= scaleFactor;
            this->skel.bones.At(boneIndex).length *= scaleFactor;
        };

        for (i32 slotI{}; slotI < this->skel.slots.size; ++slotI)
        {
            this->skel.slots.At(slotI).regionAttachment.height *= scaleFactor;
            this->skel.slots.At(slotI).regionAttachment.width *= scaleFactor;
            this->skel.slots.At(slotI).regionAttachment.parentBoneLocalPos.x *= scaleFactor;
            this->skel.slots.At(slotI).regionAttachment.parentBoneLocalPos.y *= scaleFactor;
        };
    };

    {//Adjust animations to new height standards
        //TODO: Very stupid, move out or change as I'm currently iterating over ALL keyInfos for which there are a lot in my current hashMap_Str class
        for(i32 animIndex{}; animIndex < this->animData.animations.keyInfos.size; ++animIndex)
        {
            Animation anim = (Animation)this->animData.animations.keyInfos.At(animIndex).value;

            if(anim.name)
            {
                for (i32 boneIndex{}; boneIndex < this->skel.bones.size; ++boneIndex)
                {
                    i32 hashIndex = GetHashIndex(anim.boneTimelineSets, this->skel.bones.At(boneIndex).name);

                    if (hashIndex != HASH_DOES_NOT_EXIST)
                    {
                        TimelineSet* timelineSet = GetVal(anim.boneTimelineSets, hashIndex, this->skel.bones.At(boneIndex).name);

                        if(timelineSet->translationTimeline.exists)
                        {
                            for(i32 i{}; i < timelineSet->translationTimeline.keyFrames.size; ++i)
                            {
                                timelineSet->translationTimeline.keyFrames.At(i).translation *= scaleFactor;
                            };
                        };
                    };
                };
            };
        };
    };
};

#endif //FIGHTER_IMPL
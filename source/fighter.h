#ifndef FIGHTER_INCLUDE
#define FIGHTER_INCLUDE

#include "skeleton.h"
#include "animation.h"
#include "collision_detection.h"
#include "my_math.h"

struct Fighter
{
    Fighter() = default;
    Fighter(Skeleton skel, AnimationData animData, v2f worldPos, f32 fighterHeight, HurtBox defaultHurtBox);

    Transform world;
    f32 height{};
    Skeleton skel;
    AnimationQueue animQueue;
    AnimationData animData;
    Animation currentAnim;
    HitBox hitBox;
    HurtBox hurtBox;
};

#endif

#ifdef FIGHTER_IMPL

Fighter::Fighter(Skeleton skel, AnimationData animData, v2f worldPos, f32 fighterHeight, HurtBox defaultHurtBox) : 
    skel{skel},
    animData{animData},
    animQueue{Init::_},
    currentAnim{Init::_},
    world{0.0f, worldPos, {1.0f, 1.0f}},
    height{fighterHeight},
    hurtBox{defaultHurtBox}
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

        for(i32 animIndex{}; animIndex < this->animData.animations.keyInfos.size; ++animIndex)
        {
            Animation* anim = (Animation*)&this->animData.animations.keyInfos.At(animIndex).value;

            if(anim->name)
            {
                if(StringCmp(anim->name, "right-cross"))
                    int x{};

                anim->hitBox.size.width *= scaleFactor;
                anim->hitBox.size.height *= scaleFactor;
                anim->hitBox.worldPosOffset.x *= scaleFactor;
                anim->hitBox.worldPosOffset.y *= scaleFactor;
            }
        }
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
                    TranslationTimeline* translationTimeline = &anim.boneTranslationTimelines.At(boneIndex);

                    if(translationTimeline->exists)
                    {
                        for(i32 keyFrameIndex{}; keyFrameIndex < translationTimeline->times.size; ++keyFrameIndex)
                        {
                            translationTimeline->translations.At(keyFrameIndex) *= scaleFactor;
                        };
                    };
                };
            };
        };
    };
};

#endif //FIGHTER_IMPL
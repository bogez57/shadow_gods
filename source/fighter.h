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
    f32 height {};
    Skeleton skel;
    AnimationQueue animQueue;
    AnimationData animData;
    Animation currentAnim;
    HitBox hitBox;
    HurtBox hurtBox;
};

#endif

#ifdef FIGHTER_IMPL

Fighter::Fighter(Skeleton skel, AnimationData animData, v2f worldPos, f32 fighterHeight, HurtBox defaultHurtBox)
    : skel { skel }
    , animData { animData }
    , animQueue { Init::_ }
    , currentAnim { Init::_ }
    , world { 0.0f, worldPos, { 1.5f, 1.5f } }
    , height { fighterHeight }
    , hurtBox { defaultHurtBox }
{
    f32 scaleFactorForHeightAdjustment {};
    { //Change fighter height
        f32 aspectRatio = this->skel.height / this->skel.width;
        scaleFactorForHeightAdjustment = this->height / this->skel.height;

        //New fighter height
        this->skel.height = this->height;
        this->skel.width = aspectRatio * this->height;

        for (i32 boneIndex {}; boneIndex < this->skel.bones.size; ++boneIndex)
        {
            this->skel.bones.At(boneIndex).transform.translation.x *= (scaleFactorForHeightAdjustment * this->world.scale.x);
            this->skel.bones.At(boneIndex).transform.translation.y *= (scaleFactorForHeightAdjustment * this->world.scale.y);
            this->skel.bones.At(boneIndex).originalParentLocalPos.x *= (scaleFactorForHeightAdjustment * this->world.scale.x);
            this->skel.bones.At(boneIndex).originalParentLocalPos.y *= (scaleFactorForHeightAdjustment * this->world.scale.y);
            this->skel.bones.At(boneIndex).length *= (scaleFactorForHeightAdjustment * this->world.scale.x);
        };

        for (i32 slotI {}; slotI < this->skel.slots.size; ++slotI)
        {
            this->skel.slots.At(slotI).regionAttachment.height *= (scaleFactorForHeightAdjustment * this->world.scale.y);
            this->skel.slots.At(slotI).regionAttachment.width *= (scaleFactorForHeightAdjustment * this->world.scale.x);
            this->skel.slots.At(slotI).regionAttachment.parentBoneLocalPos.x *= (scaleFactorForHeightAdjustment * this->world.scale.x);
            this->skel.slots.At(slotI).regionAttachment.parentBoneLocalPos.y *= (scaleFactorForHeightAdjustment * this->world.scale.y);
        };

        for (i32 animIndex {}; animIndex < this->animData.animations.keyInfos.size; ++animIndex)
        {
            Animation* anim = (Animation*)&this->animData.animations.keyInfos.At(animIndex).value;

            if (anim->name)
            {
                anim = GetVal(this->animData.animations, animIndex, anim->name);
                for (i32 hitBoxIndex {}; hitBoxIndex < anim->hitBoxes.size; ++hitBoxIndex)
                {
                    anim->hitBoxes.At(hitBoxIndex).size.width *= (scaleFactorForHeightAdjustment * this->world.scale.x);
                    anim->hitBoxes.At(hitBoxIndex).size.height *= (scaleFactorForHeightAdjustment * this->world.scale.y);
                    anim->hitBoxes.At(hitBoxIndex).worldPosOffset.x *= (scaleFactorForHeightAdjustment * this->world.scale.x);
                    anim->hitBoxes.At(hitBoxIndex).worldPosOffset.y *= (scaleFactorForHeightAdjustment * this->world.scale.y);
                };
            }
        };
    };

    { //Adjust animations to new height standards
        //TODO: Very stupid, move out or change as I'm currently iterating over ALL keyInfos for which there are a lot in my current hashMap_Str class
        for (i32 animIndex {}; animIndex < this->animData.animations.keyInfos.size; ++animIndex)
        {
            Animation anim = (Animation)this->animData.animations.keyInfos.At(animIndex).value;

            if (anim.name)
            {
                for (i32 boneIndex {}; boneIndex < this->skel.bones.size; ++boneIndex)
                {
                    TranslationTimeline* translationTimeline = &anim.boneTranslationTimelines.At(boneIndex);

                    if (translationTimeline->exists)
                    {
                        for (i32 keyFrameIndex {}; keyFrameIndex < translationTimeline->times.size; ++keyFrameIndex)
                        {
                            translationTimeline->translations.At(keyFrameIndex).x *= (scaleFactorForHeightAdjustment * this->world.scale.x);
                            translationTimeline->translations.At(keyFrameIndex).y *= (scaleFactorForHeightAdjustment * this->world.scale.y);
                        };
                    };
                };
            };
        };
    };
};

#endif //FIGHTER_IMPL
#ifndef FIGHTER_INCLUDE
#define FIGHTER_INCLUDE

#include "skeleton.h"
#include "animation.h"
#include "collision_detection.h"
#include "my_math.h"

struct Fighter
{
    Fighter() = default;
    Fighter(i32 memParitionID_dynamic, Skeleton skel, AnimationData animData, v2f worldPos, f32 fighterHeight, HurtBox defaultHurtBox, b flipX);
    
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

Fighter::Fighter(i32 memParitionID_dynamic, Skeleton skel, AnimationData animData, v2f worldPos, f32 fighterHeight, HurtBox defaultHurtBox, b flipX = false)
: skel { skel }
, animData { animData }
, animQueue { Init::_, memParitionID_dynamic}
, currentAnim { Init::_, memParitionID_dynamic}
, world { worldPos, 0.0f, { 1.0f, 1.0f } }
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
        
        for (i32 boneIndex {}; boneIndex < this->skel.bones.length; ++boneIndex)
        {
            this->skel.bones[boneIndex].worldSpace.scale = this->world.scale;
            this->skel.bones[boneIndex].parentBoneSpace.translation.x *= (scaleFactorForHeightAdjustment * this->world.scale.x);
            this->skel.bones[boneIndex].parentBoneSpace.translation.y *= (scaleFactorForHeightAdjustment * this->world.scale.y);
            this->skel.bones[boneIndex].initialPos_parentBoneSpace.x *= (scaleFactorForHeightAdjustment * this->world.scale.x);
            this->skel.bones[boneIndex].initialPos_parentBoneSpace.y *= (scaleFactorForHeightAdjustment * this->world.scale.y);
            this->skel.bones[boneIndex].length *= (scaleFactorForHeightAdjustment * this->world.scale.x);
        };
        
        for (i32 slotI {}; slotI < this->skel.slots.length; ++slotI)
        {
            this->skel.slots[slotI].regionAttachment.height *= (scaleFactorForHeightAdjustment * this->world.scale.y);
            this->skel.slots[slotI].regionAttachment.width *= (scaleFactorForHeightAdjustment * this->world.scale.x);
            this->skel.slots[slotI].regionAttachment.parentBoneSpace.translation.x *= (scaleFactorForHeightAdjustment * this->world.scale.x);
            this->skel.slots[slotI].regionAttachment.parentBoneSpace.translation.y *= (scaleFactorForHeightAdjustment * this->world.scale.y);
        };
        
        for (i32 animIndex {}; animIndex < this->animData.animMap.animations.length; ++animIndex)
        {
            Animation* anim = (Animation*)&this->animData.animMap.animations[animIndex];
            
            if (anim->name)
            {
                anim = GetAnimation(this->animData.animMap, anim->name);
                for (i32 hitBoxIndex {}; hitBoxIndex < anim->hitBoxCount; ++hitBoxIndex)
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
        for (i32 animIndex {}; animIndex < this->animData.animMap.animations.length; ++animIndex)
        {
            Animation anim = this->animData.animMap.animations[animIndex];
            
            if (anim.name)
            {
                for (i32 boneIndex {}; boneIndex < this->skel.bones.length; ++boneIndex)
                {
                    TranslationTimeline* translationTimeline = &anim.boneTranslationTimelines.At(boneIndex);
                    
                    if (translationTimeline->exists)
                    {
                        for (i32 keyFrameIndex {}; keyFrameIndex < translationTimeline->timesCount; ++keyFrameIndex)
                        {
                            translationTimeline->translations.At(keyFrameIndex).x *= (scaleFactorForHeightAdjustment * this->world.scale.x);
                            translationTimeline->translations.At(keyFrameIndex).y *= (scaleFactorForHeightAdjustment * this->world.scale.y);
                        };
                    };
                };
            };
        };
    };
    
    //Flip skeleton bones world positions/rotations
    if (flipX)
    {
        for (i32 i {}; i < this->skel.bones.length; ++i)
        {
            if (this->skel.bones[i].isRoot)
                this->skel.bones[i].parentBoneSpace.scale.x = -1.0f;
            else
                this->skel.bones[i].worldSpace.scale.y = -1.0f;
        };
        
        UpdateSkeletonBoneWorldTransforms($(this->skel), this->world.translation);
        
        for (i32 animIndex {}; animIndex < this->animData.animMap.animations.length; ++animIndex)
        {
            Animation* anim = &this->animData.animMap.animations[animIndex];
            
            if (anim->name)
            {
                anim = GetAnimation(this->animData.animMap, anim->name);
                for (i32 hitBoxIndex {}; hitBoxIndex < anim->hitBoxCount; ++hitBoxIndex)
                {
                    anim->hitBoxes.At(hitBoxIndex).worldPosOffset.x *= -1.0f;
                };
            }
        }
    }
};

#endif //FIGHTER_IMPL
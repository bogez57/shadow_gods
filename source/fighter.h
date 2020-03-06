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
        
        for (i32 boneIndex {}; boneIndex < this->skel.bones.size; ++boneIndex)
        {
            this->skel.bones.At(boneIndex).worldSpace.scale = this->world.scale;
            this->skel.bones.At(boneIndex).parentBoneSpace.translation.x *= (scaleFactorForHeightAdjustment * this->world.scale.x);
            this->skel.bones.At(boneIndex).parentBoneSpace.translation.y *= (scaleFactorForHeightAdjustment * this->world.scale.y);
            this->skel.bones.At(boneIndex).initialPos_parentBoneSpace.x *= (scaleFactorForHeightAdjustment * this->world.scale.x);
            this->skel.bones.At(boneIndex).initialPos_parentBoneSpace.y *= (scaleFactorForHeightAdjustment * this->world.scale.y);
            this->skel.bones.At(boneIndex).length *= (scaleFactorForHeightAdjustment * this->world.scale.x);
        };
        
        for (i32 slotI {}; slotI < this->skel.slots.size; ++slotI)
        {
            this->skel.slots.At(slotI).regionAttachment.height *= (scaleFactorForHeightAdjustment * this->world.scale.y);
            this->skel.slots.At(slotI).regionAttachment.width *= (scaleFactorForHeightAdjustment * this->world.scale.x);
            this->skel.slots.At(slotI).regionAttachment.parentBoneSpace.translation.x *= (scaleFactorForHeightAdjustment * this->world.scale.x);
            this->skel.slots.At(slotI).regionAttachment.parentBoneSpace.translation.y *= (scaleFactorForHeightAdjustment * this->world.scale.y);
        };
        
        for (i32 animIndex {}; animIndex < this->animData.animations.keyInfos.size; ++animIndex)
        {
            Animation* anim = (Animation*)&this->animData.animations.keyInfos.At(animIndex).value;
            
            if (anim->name)
            {
                anim = GetVal(this->animData.animations, animIndex, anim->name);
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
        for (i32 i {}; i < this->skel.bones.size; ++i)
        {
            if (this->skel.bones.At(i).isRoot)
                this->skel.bones.At(i).parentBoneSpace.scale.x = -1.0f;
            else
                this->skel.bones.At(i).worldSpace.scale.y = -1.0f;
        };
        
        UpdateSkeletonBoneWorldTransforms($(this->skel), this->world.translation);
        
        for (i32 animIndex {}; animIndex < this->animData.animations.keyInfos.size; ++animIndex)
        {
            Animation* anim = (Animation*)&this->animData.animations.keyInfos.At(animIndex).value;
            
            if (anim->name)
            {
                anim = GetVal(this->animData.animations, animIndex, anim->name);
                for (i32 hitBoxIndex {}; hitBoxIndex < anim->hitBoxCount; ++hitBoxIndex)
                {
                    anim->hitBoxes.At(hitBoxIndex).worldPosOffset.x *= -1.0f;
                };
            }
        }
    }
};

void CleanUpFighter(Fighter&& fighter)
{
    CleanUpAnimQueue($(fighter.animQueue));    
    CleanUpAnimation($(fighter.currentAnim));
    CleanUpSkeleton($(fighter.skel));
    //CleanUpAnimData(fighter.animData); //Need to create this for hashmap class
};

#endif //FIGHTER_IMPL
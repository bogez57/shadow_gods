#ifndef FIGHTER_INCLUDE
#define FIGHTER_INCLUDE

#include "skeleton.h"
#include "animation.h"
#include "collision_detection.h"
#include "my_math.h"

struct Fighter
{
    Fighter() = default;
    
    Transform world;
    f32 height {};
    Skeleton skel;
    AnimationQueue animQueue;
    AnimationData animData;
    Animation currentAnim;
    HitBox hitBox;
    HurtBox hurtBox;
};

void Init(Fighter&& fighter, AnimationData animData, Skeleton skel, f32 fighterHeight, HurtBox defaultHurtBox,v2f worldPos, b flipX);

#endif

#ifdef FIGHTER_IMPL

void Init(Fighter&& fighter, AnimationData animData, Skeleton skel, f32 fighterHeight, HurtBox defaultHurtBox, v2f worldPos, b flipX = false)
{
    fighter.skel = skel;//Deep copy needed??
    fighter.animData = animData;
    fighter.world = { worldPos, 0.0f, {1.0f, 1.0f} };
    fighter.height = fighterHeight;
    fighter.hurtBox = defaultHurtBox;

    f32 scaleFactorForHeightAdjustment {};
    { //Change fighter height
        f32 aspectRatio = fighter.skel.height / fighter.skel.width;
        scaleFactorForHeightAdjustment = fighter.height / fighter.skel.height;
        
        //New fighter height
        fighter.skel.height = fighter.height;
        fighter.skel.width = aspectRatio * fighter.height;
        
        for (i32 boneIndex {}; boneIndex < fighter.skel.bones.length; ++boneIndex)
        {
            fighter.skel.bones[boneIndex].worldSpace.scale = fighter.world.scale;
            fighter.skel.bones[boneIndex].parentBoneSpace.translation.x *= (scaleFactorForHeightAdjustment * fighter.world.scale.x);
            fighter.skel.bones[boneIndex].parentBoneSpace.translation.y *= (scaleFactorForHeightAdjustment * fighter.world.scale.y);
            fighter.skel.bones[boneIndex].initialPos_parentBoneSpace.x *= (scaleFactorForHeightAdjustment * fighter.world.scale.x);
            fighter.skel.bones[boneIndex].initialPos_parentBoneSpace.y *= (scaleFactorForHeightAdjustment * fighter.world.scale.y);
            fighter.skel.bones[boneIndex].length *= (scaleFactorForHeightAdjustment * fighter.world.scale.x);
        };
        
        for (i32 slotI {}; slotI < fighter.skel.slots.length; ++slotI)
        {
            fighter.skel.slots[slotI].regionAttachment.height *= (scaleFactorForHeightAdjustment * fighter.world.scale.y);
            fighter.skel.slots[slotI].regionAttachment.width *= (scaleFactorForHeightAdjustment * fighter.world.scale.x);
            fighter.skel.slots[slotI].regionAttachment.parentBoneSpace.translation.x *= (scaleFactorForHeightAdjustment * fighter.world.scale.x);
            fighter.skel.slots[slotI].regionAttachment.parentBoneSpace.translation.y *= (scaleFactorForHeightAdjustment * fighter.world.scale.y);
        };
        
        for (i32 animIndex {}; animIndex < fighter.animData.animMap.animations.length; ++animIndex)
        {
            Animation* anim = (Animation*)&fighter.animData.animMap.animations[animIndex];
            
            if (anim->name)
            {
                anim = GetAnimation(fighter.animData.animMap, anim->name);
                for (i32 hitBoxIndex {}; hitBoxIndex < anim->hitBoxCount; ++hitBoxIndex)
                {
                    anim->hitBoxes.At(hitBoxIndex).size.width *= (scaleFactorForHeightAdjustment * fighter.world.scale.x);
                    anim->hitBoxes.At(hitBoxIndex).size.height *= (scaleFactorForHeightAdjustment * fighter.world.scale.y);
                    anim->hitBoxes.At(hitBoxIndex).worldPosOffset.x *= (scaleFactorForHeightAdjustment * fighter.world.scale.x);
                    anim->hitBoxes.At(hitBoxIndex).worldPosOffset.y *= (scaleFactorForHeightAdjustment * fighter.world.scale.y);
                };
            }
        };
    };
    
    { //Adjust animations to new height standards
        //TODO: Very stupid, move out or change as I'm currently iterating over ALL keyInfos for which there are a lot in my current hashMap_Str class
        for (i32 animIndex {}; animIndex < fighter.animData.animMap.animations.length; ++animIndex)
        {
            Animation anim = fighter.animData.animMap.animations[animIndex];
            
            if (anim.name)
            {
                for (i32 boneIndex {}; boneIndex < fighter.skel.bones.length; ++boneIndex)
                {
                    TranslationTimeline* translationTimeline = &anim.boneTranslationTimelines.At(boneIndex);
                    
                    if (translationTimeline->exists)
                    {
                        for (i32 keyFrameIndex {}; keyFrameIndex < translationTimeline->timesCount; ++keyFrameIndex)
                        {
                            translationTimeline->translations.At(keyFrameIndex).x *= (scaleFactorForHeightAdjustment * fighter.world.scale.x);
                            translationTimeline->translations.At(keyFrameIndex).y *= (scaleFactorForHeightAdjustment * fighter.world.scale.y);
                        };
                    };
                };
            };
        };
    };
    
    //Flip skeleton bones world positions/rotations
    if (flipX)
    {
        for (i32 i {}; i < fighter.skel.bones.length; ++i)
        {
            if (fighter.skel.bones[i].isRoot)
                fighter.skel.bones[i].parentBoneSpace.scale.x = -1.0f;
            else
                fighter.skel.bones[i].worldSpace.scale.y = -1.0f;
        };
        
        UpdateSkeletonBoneWorldTransforms($(fighter.skel), fighter.world.translation);
        
        for (i32 animIndex {}; animIndex < fighter.animData.animMap.animations.length; ++animIndex)
        {
            Animation* anim = &fighter.animData.animMap.animations[animIndex];
            
            if (anim->name)
            {
                anim = GetAnimation(fighter.animData.animMap, anim->name);
                for (i32 hitBoxIndex {}; hitBoxIndex < anim->hitBoxCount; ++hitBoxIndex)
                {
                    anim->hitBoxes.At(hitBoxIndex).worldPosOffset.x *= -1.0f;
                };
            }
        }
    }
};

#endif //FIGHTER_IMPL
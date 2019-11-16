#ifndef FIGHTER_INCLUDE
#define FIGHTER_INCLUDE

#include "skeleton.h"
#include "animation.h"
#include "collision_detection.h"
#include "my_math.h"

struct Fighter
{
    Fighter() = default;
    Fighter(const char* atlasFilePath, const char* jsonFilePath, v2f worldPos, f32 fighterHeight, HurtBox defaultHurtBox);

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

Fighter::Fighter(const char* atlasFilePath, const char* jsonFilePath, v2f worldPos, f32 fighterHeight, HurtBox defaultHurtBox) :
    skel{atlasFilePath, jsonFilePath, heap},
    animData{jsonFilePath, this->skel},
    animQueue{Init::_},
    currentAnim{Init::_},
    world{0.0f, worldPos, {1.0f, 1.0f}},
    height{fighterHeight},
    hurtBox{defaultHurtBox}
{
    {//Translate pixels to meters and degrees to radians (since spine exports everything in pixel/degree units)
        f32 pixelsPerMeter{global_renderingInfo->_pixelsPerMeter};

        this->skel.width /= pixelsPerMeter;
        this->skel.height /= pixelsPerMeter;

        for (i32 boneIndex{}; boneIndex < this->skel.bones.size; ++boneIndex)
        {
            this->skel.bones.At(boneIndex).transform.translation.x /= pixelsPerMeter;
            this->skel.bones.At(boneIndex).transform.translation.y /= pixelsPerMeter;
            this->skel.bones.At(boneIndex).originalParentLocalPos.x /= pixelsPerMeter;
            this->skel.bones.At(boneIndex).originalParentLocalPos.y /= pixelsPerMeter;

            this->skel.bones.At(boneIndex).transform.rotation = Radians(this->skel.bones.At(boneIndex).transform.rotation);
            this->skel.bones.At(boneIndex).originalParentLocalRotation = Radians(this->skel.bones.At(boneIndex).originalParentLocalRotation);

            this->skel.bones.At(boneIndex).length /= pixelsPerMeter; 
        };

        for (i32 slotI{}; slotI < this->skel.slots.size; ++slotI)
        {
            this->skel.slots.At(slotI).regionAttachment.height /= pixelsPerMeter;
            this->skel.slots.At(slotI).regionAttachment.width /= pixelsPerMeter;
            this->skel.slots.At(slotI).regionAttachment.parentBoneLocalRotation = Radians(this->skel.slots.At(slotI).regionAttachment.parentBoneLocalRotation);
            this->skel.slots.At(slotI).regionAttachment.parentBoneLocalPos.x /= pixelsPerMeter;
            this->skel.slots.At(slotI).regionAttachment.parentBoneLocalPos.y /= pixelsPerMeter;
        };

        for(i32 animIndex{}; animIndex < this->animData.animations.keyInfos.size; ++animIndex)
        {
            Animation* anim = (Animation*)&this->animData.animations.keyInfos.At(animIndex).value;

            if(anim->name)
            {
                for(i32 boneIndex{}; boneIndex < anim->bones.size; ++boneIndex)
                {
                    TranslationTimeline* boneTranslationTimeline = &anim->boneTranslationTimelines.At(boneIndex);
                    for(i32 keyFrameIndex{}; keyFrameIndex < boneTranslationTimeline->translations.size; ++keyFrameIndex)
                    {
                        boneTranslationTimeline->translations.At(keyFrameIndex).x /= pixelsPerMeter;
                        boneTranslationTimeline->translations.At(keyFrameIndex).y /= pixelsPerMeter;
                    }

                    RotationTimeline* boneRotationTimeline = &anim->boneRotationTimelines.At(boneIndex);
                    for(i32 keyFrameIndex{}; keyFrameIndex < boneRotationTimeline->angles.size; ++keyFrameIndex)
                    {
                        boneRotationTimeline->angles.At(keyFrameIndex) = Radians(boneRotationTimeline->angles.At(keyFrameIndex));
                    }
                };

                anim->hitBox.size.width /= pixelsPerMeter;
                anim->hitBox.size.height /= pixelsPerMeter;
                anim->hitBox.worldPosOffset.x /= pixelsPerMeter;
                anim->hitBox.worldPosOffset.y /= pixelsPerMeter;
            }
        }
    }

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
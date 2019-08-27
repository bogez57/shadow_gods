/*
 Copyright (c) 2009, Dave Gamble
 Copyright (c) 2013, Esoteric Software

 Permission is hereby granted, dispose of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 */

/*
    Note: Currently slots and bones have to have the same names in order for parsing to work
*/

#ifndef SKELETON_INCLUDE
#define SKELETON_INCLUDE

#include <string.h>
#include "json.h"
#include "atlas.h"

struct Bone
{
    Bone() = default;
    Bone(Init)
        : childBones{heap}
    {
        Reserve($(childBones), 10);
    };

    v2f worldPos{};
    f32 originalParentLocalRotation{};
    v2f originalParentLocalPos{};
    v2f* parentLocalScale{nullptr};
    v2f* parentLocalPos{nullptr};
    f32* parentLocalRotation{nullptr};
    Transform transform;
    f32 length{};
    Bone* parentBone{nullptr};
    Dynam_Array<Bone*> childBones; 
    b isRoot{false};
    const char* name{nullptr};
};

struct Region_Attachment
{
    f32 width, height{};
    v2f parentBoneLocalPos{};
    v2f parentBoneLocalScale{};
    f32 parentBoneLocalRotation{};
    AtlasRegion region_image;
};

struct Slot
{
    char* name{nullptr};
    Bone* bone{nullptr};
    Region_Attachment regionAttachment{};
};

struct Skeleton
{
    Skeleton() = default;
    Skeleton(const char* atlasFilePath, const char* jsonFilepath, i32 numOfBones, i32 numOfSlots, i32 memParitionID);

    Dynam_Array<Bone> bones; 
    Dynam_Array<Slot> slots;
    f32 width{}, height{};
};

Bone* GetBoneFromSkeleton(Skeleton skeleton, char* boneName);

#endif

#ifdef SKELETON_IMPL

void _CreateSkeleton(Skeleton&& skel, Atlas atlas, const char* skeletonJson);
void _TranslateSkelPropertiesToGameUnits(Skeleton&& skeleton);
Skeleton::Skeleton(const char* atlasFilePath, const char* jsonFilePath, i32 numBones, i32 numSlots, i32 memParitionID) :
        bones{numBones, Bone{Init::_}, memParitionID},
        slots{numSlots, memParitionID}
{
    i32 length;

    const char* skeletonJson = globalPlatformServices->ReadEntireFile($(length), jsonFilePath);

    Atlas* atlas = CreateAtlasFromFile(atlasFilePath, 0);

    if (skeletonJson)
        _CreateSkeleton($(*this), *atlas, skeletonJson);
    else
        InvalidCodePath;

    globalPlatformServices->Free((void*)skeletonJson);

    _TranslateSkelPropertiesToGameUnits($(*this));
};

Bone* GetBoneFromSkeleton(Skeleton skeleton, char* boneName)
{
    Bone* bone {};

    for (i32 i = 0; i < skeleton.bones.size; ++i)
    {
        if (strcmp(skeleton.bones.At(i).name, boneName) == 0)
        {
            bone = &skeleton.bones.At(i);
            break;
        };
    };

    BGZ_ASSERT(bone->name != nullptr, "Bone was not found!");

    return bone;
};

void _CreateSkeleton(Skeleton&& skel, Atlas atlas, const char* skeletonJson)
{
    Json* root {};
    root = Json_create(skeletonJson);

    Json* jsonSkeleton = Json_getItem(root, "skeleton"); /* clang-format off */BGZ_ASSERT(jsonSkeleton, "Unable to return valid json object for skeleton!"); /* clang-format on */
    Json* jsonBones = Json_getItem(root, "bones"); /* clang-format off */BGZ_ASSERT(jsonBones, "Unable to return valid json object for bones!"); /* clang-format on */
    Json* jsonSlots = Json_getItem(root, "slots"); /* clang-format off */BGZ_ASSERT(jsonSlots, "Unable to return valid json object for slots!"); /* clang-format on */

    skel.width = Json_getFloat(jsonSkeleton, "width", 0.0f);
    skel.height = Json_getFloat(jsonSkeleton, "height", 0.0f);

    { //Create Bones
        i32 boneIndex {};
        for (Json* currentBone_json = jsonBones->child; boneIndex < jsonBones->size; currentBone_json = currentBone_json->next, ++boneIndex)
        {
            Bone* newBone = &skel.bones.At(boneIndex);
            newBone->name = Json_getString(currentBone_json, "name", 0);
            if(!strcmp(newBone->name, "root"))
                newBone->isRoot = true;
            newBone->transform.scale = v2f{1.0f, 1.0f};
            newBone->parentLocalScale = &newBone->transform.scale;

            newBone->transform.rotation = Json_getFloat(currentBone_json, "rotation", 0.0f);
            newBone->parentLocalRotation = &newBone->transform.rotation;
            newBone->originalParentLocalRotation = *newBone->parentLocalRotation;

            newBone->transform.translation.x = Json_getFloat(currentBone_json, "x", 0.0f);
            newBone->transform.translation.y = Json_getFloat(currentBone_json, "y", 0.0f);
            newBone->parentLocalPos = &newBone->transform.translation;
            newBone->originalParentLocalPos = *newBone->parentLocalPos;

            newBone->length = Json_getFloat(currentBone_json, "length", 0.0f);
            if (Json_getString(currentBone_json, "parent", 0)) //If no parent then skip
            {
                newBone->parentBone = GetBoneFromSkeleton(skel, (char*)Json_getString(currentBone_json, "parent", 0));
                PushBack($(newBone->parentBone->childBones), newBone);
            };
        };
    };

    { //Create Slots
        i32 slotIndex {};
        for (Json* currentSlot_json = jsonSlots->child; slotIndex < jsonSlots->size; currentSlot_json = currentSlot_json->next, ++slotIndex)
        {
            //Insert slot info in reverse order to get correct draw order (since json file has the draw order flipped from spine application)
            Slot* slot = &skel.slots.At(slotIndex);
            slot->name = (char*)Json_getString(currentSlot_json, "name", 0);
            slot->bone = GetBoneFromSkeleton(skel, (char*)Json_getString(currentSlot_json, "bone", 0));
            slot->regionAttachment = [currentSlot_json, root, atlas]() -> Region_Attachment 
            {
                Region_Attachment resultRegionAttch {};

                const char* attachmentName = Json_getString(currentSlot_json, "attachment", 0);
                Json* jsonSkin = Json_getItem(root, "skins");
                Json* jsonDefaultSkin = Json_getItem(jsonSkin, "default");

                i32 attachmentCounter {};
                for (Json* currentBodyPartOfSkin_json = jsonDefaultSkin->child; attachmentCounter < jsonDefaultSkin->size; currentBodyPartOfSkin_json = currentBodyPartOfSkin_json->next, ++attachmentCounter)
                {
                    Json* jsonAttachment = currentBodyPartOfSkin_json->child;

                    if (strcmp(jsonAttachment->name, attachmentName) == 0)
                    {
                        resultRegionAttch.width = (f32)Json_getInt(jsonAttachment, "width", 0);
                        resultRegionAttch.height = (f32)Json_getInt(jsonAttachment, "height", 0);
                        resultRegionAttch.parentBoneLocalPos.x = Json_getFloat(jsonAttachment, "x", 0.0f);
                        resultRegionAttch.parentBoneLocalPos.y = Json_getFloat(jsonAttachment, "y", 0.0f);
                        resultRegionAttch.parentBoneLocalRotation= Json_getFloat(jsonAttachment, "rotation", 0.0f);
                        resultRegionAttch.parentBoneLocalScale.x = Json_getFloat(jsonAttachment, "scaleX", 1.0f);
                        resultRegionAttch.parentBoneLocalScale.y = Json_getFloat(jsonAttachment, "scaleY", 1.0f);
                        resultRegionAttch.region_image = [atlas, attachmentName]() -> AtlasRegion 
                        {
                            AtlasRegion resultAtlasRegion {};
                            AtlasRegion* region = atlas.regions;

                            while (region)
                            {
                                if (strcmp(region->name, attachmentName) == 0)
                                {
                                    resultAtlasRegion = *region;
                                    break;
                                };

                                region = region->next;
                            };

                            return resultAtlasRegion;
                        }();

                        break;
                    };
                };

                return resultRegionAttch;
            }();
        };
    };
};

void _TranslateSkelPropertiesToGameUnits(Skeleton&& skeleton)
{
    f32 pixelsPerMeter{100.0f};

    skeleton.width /= pixelsPerMeter;
    skeleton.height /= pixelsPerMeter;

    for (i32 boneIndex{}; boneIndex < skeleton.bones.size; ++boneIndex)
    {
        skeleton.bones.At(boneIndex).transform.translation.x /= pixelsPerMeter;
        skeleton.bones.At(boneIndex).transform.translation.y /= pixelsPerMeter;
        skeleton.bones.At(boneIndex).originalParentLocalPos.x /= pixelsPerMeter;
        skeleton.bones.At(boneIndex).originalParentLocalPos.y /= pixelsPerMeter;

        skeleton.bones.At(boneIndex).transform.rotation = Radians(skeleton.bones.At(boneIndex).transform.rotation);
        skeleton.bones.At(boneIndex).originalParentLocalRotation = Radians(skeleton.bones.At(boneIndex).originalParentLocalRotation);

        skeleton.bones.At(boneIndex).length /= pixelsPerMeter; 
    };

    for (i32 slotI{}; slotI < skeleton.slots.size; ++slotI)
    {
        skeleton.slots.At(slotI).regionAttachment.height /= pixelsPerMeter;
        skeleton.slots.At(slotI).regionAttachment.width /= pixelsPerMeter;
        skeleton.slots.At(slotI).regionAttachment.parentBoneLocalRotation = Radians(skeleton.slots.At(slotI).regionAttachment.parentBoneLocalRotation);
        skeleton.slots.At(slotI).regionAttachment.parentBoneLocalPos.x /= pixelsPerMeter;
        skeleton.slots.At(slotI).regionAttachment.parentBoneLocalPos.y /= pixelsPerMeter;
    };
};

Bone* FindBone(Skeleton* skel, const char* boneName)
{
    for(i32 i = 0; i < skel->bones.size; ++i)
    {
        if(strcmp(skel->bones.At(i).name, boneName) == 0)
            return &skel->bones.At(i);
    };

    return nullptr;
};

#endif //SKELETON_IMPL
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

#ifndef SKELETON_INCLUDE_H
#define SKELETON_INCLUDE_H

#include "json.h"
#include "atlas.h"

struct Bone
{
    v2f parentLocalPos;
    v2f worldPos;
    f32 rotation, length;
    Bone* parentBone;
    const char* name;
};

struct Region_Attachment
{
    i32 width, height;
    v2f parentLocalPos;
    v2f parentLocalRotation;
    v2f scale;
    AtlasRegion imageInfo;
};

struct Slot
{
    char* name;
    Bone* bone;
    Region_Attachment regionAttachment;
};

struct Skeleton
{
    Dynam_Array<Bone> bones;
    Dynam_Array<Slot> slots;
    f32 width, height;
    v2f worldPos;
};

Skeleton CreateSkeletonUsingJsonFile(Atlas* atlas, const char* skeletonJsonFilePath);
Bone* GetBoneFromSkeleton(Skeleton skeleton, char* boneName);

#endif

#ifdef SKELETON_IMPL

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

Skeleton _CreateSkeleton(Atlas atlas, const char* skeletonJson)
{
    Json* root {};
    root = Json_create(skeletonJson);

    Json* jsonSkeleton = Json_getItem(root, "skeleton"); /* clang-format off */BGZ_ASSERT(jsonSkeleton, "Unable to return valid json object for skeleton!"); /* clang-format on */

    Skeleton newSkeleton {};
    newSkeleton.width = Json_getFloat(jsonSkeleton, "width", 0.0f);
    newSkeleton.height = Json_getFloat(jsonSkeleton, "height", 0.0f);

    { //Create Bones
        Json* jsonBones = Json_getItem(root, "bones"); /* clang-format off */BGZ_ASSERT(jsonBones, "Unable to return valid json object for bones!"); /* clang-format on */
        newSkeleton.bones.Init(jsonBones->size, heap);
        i32 boneIndex {};
        for (Json* currentJsonObject = jsonBones->child; boneIndex < newSkeleton.bones.size; currentJsonObject = currentJsonObject->next, ++boneIndex)
        {
            newSkeleton.bones.At(boneIndex).name = Json_getString(currentJsonObject, "name", 0);
            newSkeleton.bones.At(boneIndex).parentLocalPos.x = Json_getFloat(currentJsonObject, "x", 0.0f);
            newSkeleton.bones.At(boneIndex).parentLocalPos.y = Json_getFloat(currentJsonObject, "y", 0.0f);
            newSkeleton.bones.At(boneIndex).rotation = Json_getFloat(currentJsonObject, "rotation", 0.0f);
            newSkeleton.bones.At(boneIndex).length = Json_getFloat(currentJsonObject, "length", 0.0f);
            if (Json_getString(currentJsonObject, "parent", 0)) //If no parent then skip
                newSkeleton.bones.At(boneIndex).parentBone = GetBoneFromSkeleton(newSkeleton, (char*)Json_getString(currentJsonObject, "parent", 0));
        };
    };

    { //Create Slots
        Json* jsonSlots = Json_getItem(root, "slots"); /* clang-format off */BGZ_ASSERT(jsonSlots, "Unable to return valid json object for slots!"); /* clang-format on */
        newSkeleton.slots.Init(jsonSlots->size, heap);
        i32 slotIndex {};
        for (Json* currentJsonObject = jsonSlots->child; slotIndex < newSkeleton.slots.size; currentJsonObject = currentJsonObject->next, ++slotIndex)
        {
            //Insert slot info in reverse order to get correct draw order (since json file has the draw order flipped from spine application)
            Slot* slot = &newSkeleton.slots.At((newSkeleton.slots.size - 1) - slotIndex);
            slot->name = (char*)Json_getString(currentJsonObject, "name", 0);
            slot->bone = GetBoneFromSkeleton(newSkeleton, (char*)Json_getString(currentJsonObject, "bone", 0));
            slot->regionAttachment = [currentJsonObject, root, atlas]() -> Region_Attachment 
            {
                Region_Attachment resultRegionAttch {};

                const char* attachmentName = Json_getString(currentJsonObject, "attachment", 0);
                Json* jsonSkin = Json_getItem(root, "skins");
                Json* jsonDefaultSkin = Json_getItem(jsonSkin, "default");

                i32 attachmentCounter {};
                for (Json* currentJsonObject = jsonDefaultSkin->child; attachmentCounter < jsonDefaultSkin->size; currentJsonObject = currentJsonObject->next, ++attachmentCounter)
                {
                    Json* jsonAttachment = currentJsonObject->child;

                    if (strcmp(jsonAttachment->name, attachmentName) == 0)
                    {
                        resultRegionAttch.width = Json_getInt(jsonAttachment, "width", 0);
                        resultRegionAttch.height = Json_getInt(jsonAttachment, "height", 0);
                        resultRegionAttch.parentLocalPos.x = Json_getFloat(jsonAttachment, "x", 0.0f);
                        resultRegionAttch.parentLocalPos.y = Json_getFloat(jsonAttachment, "y", 0.0f);
                        resultRegionAttch.parentLocalRotation.y = Json_getFloat(jsonAttachment, "rotation", 0.0f);
                        resultRegionAttch.scale.x = Json_getFloat(jsonAttachment, "scaleX", 1.0f);
                        resultRegionAttch.scale.y = Json_getFloat(jsonAttachment, "scaleY", 1.0f);
                        resultRegionAttch.imageInfo = [atlas, attachmentName]() -> AtlasRegion 
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

    return newSkeleton;
};

Skeleton CreateSkeletonUsingJsonFile(Atlas atlas, const char* skeletonJsonFilePath)
{
    i32 length;
    Skeleton newSkeleton;

    const char* skeletonJson = globalPlatformServices->ReadEntireFile($(length), skeletonJsonFilePath);

    if (skeletonJson)
        newSkeleton = _CreateSkeleton(atlas, skeletonJson);
    else
        InvalidCodePath;

    globalPlatformServices->Free((void*)skeletonJson);

    return newSkeleton;
};

#endif //SKELETON_IMPL
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
    f32 x, y, rotation, scaleX, scaleY, length;
    Bone* parent;
};

typedef enum
{
    ATTACHMENT_REGION,
    ATTACHMENT_BOUNDING_BOX,
    ATTACHMENT_MESH,
    ATTACHMENT_LINKED_MESH,
    ATTACHMENT_PATH,
    ATTACHMENT_POINT,
    ATTACHMENT_CLIPPING
} AttachmentType;

class Attachment
{
    const char* const name;
    const AttachmentType type;

protected:
    Attachment() = default;
};

struct Slot
{
    Attachment* const attachment;
};

struct Skeleton
{
    i32 boneCount;
    Dynam_Array<Bone> bones;
    Bone* root;

    i32 slotCount;
    Dynam_Array<Slot*> slots;
    Dynam_Array<Slot*> drawOrder;

    f32 width, height;
    f32 localX, localY;
};

Skeleton CreateSkeletonUsingJsonFile(Atlas* atlas, const char* skeletonJsonFilePath);

#endif

#ifdef SKELETON_IMPL

Skeleton _CreateSkeleton(Atlas* atlas, const char* skeletonJson)
{
    Skeleton newSkeleton {};

    Json* root {};
    root = Json_create(skeletonJson);

    Json* jsonSkeleton = Json_getItem(root, "skeleton");
    if (jsonSkeleton)
    {
        newSkeleton.width = Json_getFloat(jsonSkeleton, "width", 0);
        newSkeleton.height = Json_getFloat(jsonSkeleton, "height", 0);
    };

    Json* jsonBones = Json_getItem(root, "bones");
    if (jsonBones)
    {
        newSkeleton.boneCount = jsonBones->size;

        i32 boneIndex {};
        for (Json* currentJsonBone = jsonBones->child; boneIndex < newSkeleton.boneCount; currentJsonBone = jsonBones->next, ++boneIndex)
        {
        };
    };

    Json* jsonSlots = Json_getItem(root, "slots");
    if (jsonSlots)
    {
    };

    return newSkeleton;
};

Skeleton CreateSkeletonUsingJsonFile(Atlas* atlas, const char* skeletonJsonFilePath)
{
    i32 length;
    Skeleton newSkeleton;

    const char* skeletonJson = globalPlatformServices->ReadEntireFile(&length, skeletonJsonFilePath);

    if (skeletonJson)
        newSkeleton = _CreateSkeleton(atlas, skeletonJson);
    else
        InvalidCodePath;

    globalPlatformServices->Free((void*)skeletonJson);

    return newSkeleton;
};

#endif //SKELETON_IMPL
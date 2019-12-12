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

struct Region_Attachment
{
    f32 width, height {};
    v2f scale {};
    Transform parentBoneSpace {};
    AtlasRegion region_image;
};

struct Bone
{
    Bone() = default;
    Bone(Init)
        : childBones { heap }
        , originalCollisionBoxVerts { heap }
    {
        Reserve($(childBones), 10);
    };

    Transform parentBoneSpace;
    Transform worldSpace;
    f32 initialRotation_parentBoneSpace {};
    v2f initialPos_parentBoneSpace {};
    v2f initialTranslationForMixing {};
    f32 initialRotationForMixing {};
    f32 length {};
    Bone* parentBone { nullptr };
    Dynam_Array<v2f> originalCollisionBoxVerts;
    Dynam_Array<Bone*> childBones;
    b isRoot { false };
    const char* name { nullptr };
};

struct Slot
{
    char* name { nullptr };
    Bone* bone { nullptr };
    Region_Attachment regionAttachment {};
};

struct Skeleton
{
    Skeleton() = default;
    Skeleton(const char* atlasFilePath, const char* jsonFilepath, i32 memParitionID);

    Dynam_Array<Bone> bones;
    Dynam_Array<Slot> slots;
    f32 width {}, height {};
};

Skeleton CopySkeleton(Skeleton src);
void ResetBonesToSetupPose(Skeleton&& skeleton);
Bone* GetBoneFromSkeleton(Skeleton skeleton, char* boneName);

#endif

#ifdef SKELETON_IMPL

//TODO: Not sure if this is working properly yet
Skeleton CopySkeleton(Skeleton src)
{
    Skeleton dest = src;

    CopyArray(src.bones, $(dest.bones));
    CopyArray(src.slots, $(dest.slots));

    for (i32 boneIndex {}; boneIndex < src.bones.size; ++boneIndex)
    {
        for (i32 childBoneIndex {}; childBoneIndex < src.bones.At(boneIndex).childBones.size; ++childBoneIndex)
        {
            const char* childBoneName = src.bones.At(boneIndex).childBones.At(childBoneIndex)->name;
            Bone* bone = GetBoneFromSkeleton(dest, (char*)childBoneName);
            dest.bones.At(boneIndex).childBones.At(childBoneIndex) = bone;
        }
    };

    return dest;
};

Skeleton::Skeleton(const char* atlasFilePath, const char* jsonFilePath, i32 memParitionID)
    : bones { memParitionID }
    , slots { memParitionID }
{
    i32 length;

    Reserve($(this->bones), 20);
    Reserve($(this->slots), 21);

    const char* skeletonJson = globalPlatformServices->ReadEntireFile($(length), jsonFilePath);

    Atlas* atlas = CreateAtlasFromFile(atlasFilePath, 0);

    if (skeletonJson)
    {
        Json* root {};
        root = Json_create(skeletonJson);

        Json* jsonSkeleton = Json_getItem(root, "skeleton"); /* clang-format off */BGZ_ASSERT(jsonSkeleton, "Unable to return valid json object for skeleton!"); /* clang-format on */
        Json* jsonBones = Json_getItem(root, "bones"); /* clang-format off */BGZ_ASSERT(jsonBones, "Unable to return valid json object for bones!"); /* clang-format on */
        Json* jsonSlots = Json_getItem(root, "slots"); /* clang-format off */BGZ_ASSERT(jsonSlots, "Unable to return valid json object for slots!"); /* clang-format on */
        Json* skins_json = Json_getItem(root, "skins");

        this->width = Json_getFloat(jsonSkeleton, "width", 0.0f);
        this->height = Json_getFloat(jsonSkeleton, "height", 0.0f);

        { //Read in Bone data
            i32 boneIndex {};
            for (Json* currentBone_json = jsonBones->child; boneIndex < jsonBones->size; currentBone_json = currentBone_json->next, ++boneIndex)
            {
                Bone newBone { Init::_ };
                PushBack($(this->bones), newBone);
                Bone* bone = GetLastElem(this->bones);

                bone->name = Json_getString(currentBone_json, "name", 0);
                if (StringCmp(bone->name, "root"))
                    bone->isRoot = true;
                bone->parentBoneSpace.scale.x = Json_getFloat(currentBone_json, "scaleX", 1.0f);
                bone->parentBoneSpace.scale.y = Json_getFloat(currentBone_json, "scaleY", 1.0f);
                bone->worldSpace.scale = { 1.0f, 1.0f };

                bone->parentBoneSpace.rotation = Json_getFloat(currentBone_json, "rotation", 0.0f);
                bone->initialRotation_parentBoneSpace = bone->parentBoneSpace.rotation;

                bone->parentBoneSpace.translation.x = Json_getFloat(currentBone_json, "x", 0.0f);
                bone->parentBoneSpace.translation.y = Json_getFloat(currentBone_json, "y", 0.0f);
                bone->initialPos_parentBoneSpace = bone->parentBoneSpace.translation;

                bone->length = Json_getFloat(currentBone_json, "length", 0.0f);

                { //Read in collision box data
                    Json* defaultSkinAttachments_json = Json_getItem(skins_json->child, "attachments");

                    char nameOfCollisionBoxAttachment[100] = { "box-" };
                    strcat(nameOfCollisionBoxAttachment, bone->name);
                    if (NOT StringCmp(bone->name, "root"))
                    {
                        Json* collisionBox_json = Json_getItem(defaultSkinAttachments_json, nameOfCollisionBoxAttachment)->child;
                        Json* verts_json = Json_getItem(collisionBox_json, "vertices")->child;

                        i32 numVerts = Json_getInt(collisionBox_json, "vertexCount", 0);
                        for (i32 i {}; i < numVerts; ++i)
                        {
                            PushBack($(bone->originalCollisionBoxVerts), v2f { verts_json->valueFloat, verts_json->next->valueFloat });
                            verts_json = verts_json->next->next;
                        };
                    }
                }

                if (Json_getString(currentBone_json, "parent", 0)) //If no parent then skip
                {
                    bone->parentBone = GetBoneFromSkeleton(*this, (char*)Json_getString(currentBone_json, "parent", 0));
                    PushBack($(bone->parentBone->childBones), bone);
                };
            };
        };

        { //Read in Slot data
            i32 slotIndex {};
            for (Json* currentSlot_json = jsonSlots->child; slotIndex < jsonSlots->size; currentSlot_json = currentSlot_json->next, ++slotIndex)
            {
                //Ignore creating slots here for collision boxes. Don't think I need it
                char* slotName = (char*)Json_getString(currentSlot_json, "name", 0);
                char slotName_firstThreeLetters[3] = { slotName[0], slotName[1], slotName[2] };
                if (NOT StringCmp(slotName_firstThreeLetters, "box"))
                {
                    //Insert slot info in reverse order to get correct draw order (since json file has the draw order flipped from spine application)
                    Slot newSlot {};
                    PushBack($(this->slots), newSlot);
                    Slot* slot = GetLastElem(this->slots);

                    slot->name = (char*)Json_getString(currentSlot_json, "name", 0);
                    if (StringCmp(slot->name, "left-hand"))
                        int x {};
                    slot->bone = GetBoneFromSkeleton(*this, (char*)Json_getString(currentSlot_json, "bone", 0));
                    slot->regionAttachment = [currentSlot_json, skins_json, atlas]() -> Region_Attachment {
                        Region_Attachment resultRegionAttch {};

                        const char* attachmentName = Json_getString(currentSlot_json, "attachment", 0);
                        Json* jsonDefaultSkin = Json_getItem(skins_json->child, "attachments");

                        i32 attachmentCounter {};
                        for (Json* currentBodyPartOfSkin_json = jsonDefaultSkin->child; attachmentCounter < jsonDefaultSkin->size; currentBodyPartOfSkin_json = currentBodyPartOfSkin_json->next, ++attachmentCounter)
                        {
                            Json* jsonAttachment = currentBodyPartOfSkin_json->child;

                            if (StringCmp(jsonAttachment->name, attachmentName))
                            {
                                resultRegionAttch.width = (f32)Json_getInt(jsonAttachment, "width", 0);
                                resultRegionAttch.height = (f32)Json_getInt(jsonAttachment, "height", 0);
                                resultRegionAttch.scale.x = (f32)Json_getFloat(jsonAttachment, "scaleX", 1.0f);
                                resultRegionAttch.scale.y = (f32)Json_getFloat(jsonAttachment, "scaleY", 1.0f);
                                resultRegionAttch.parentBoneSpace.translation.x = Json_getFloat(jsonAttachment, "x", 0.0f);
                                resultRegionAttch.parentBoneSpace.translation.y = Json_getFloat(jsonAttachment, "y", 0.0f);
                                resultRegionAttch.parentBoneSpace.rotation = Json_getFloat(jsonAttachment, "rotation", 0.0f);
                                resultRegionAttch.parentBoneSpace.scale.x = Json_getFloat(jsonAttachment, "scaleX", 1.0f);
                                resultRegionAttch.parentBoneSpace.scale.y = Json_getFloat(jsonAttachment, "scaleY", 1.0f);
                                resultRegionAttch.region_image = [atlas, attachmentName]() -> AtlasRegion {
                                    AtlasRegion resultAtlasRegion {};
                                    AtlasRegion* region = atlas->regions;

                                    while (region)
                                    {
                                        if (StringCmp(region->name, attachmentName))
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
    }
    else
    {
        InvalidCodePath;
    };

    globalPlatformServices->Free((void*)skeletonJson);
};

void ResetBonesToSetupPose(Skeleton&& skel)
{
    for (i32 boneIndex {}; boneIndex < skel.bones.size; ++boneIndex)
    {
        skel.bones.At(boneIndex).parentBoneSpace.rotation = skel.bones.At(boneIndex).initialRotation_parentBoneSpace;
        skel.bones.At(boneIndex).parentBoneSpace.translation = skel.bones.At(boneIndex).initialPos_parentBoneSpace;
    };
};

Bone* GetBoneFromSkeleton(Skeleton skeleton, char* boneName)
{
    Bone* bone {};

    for (i32 i = 0; i < skeleton.bones.size; ++i)
    {
        if (StringCmp(skeleton.bones.At(i).name, boneName))
        {
            bone = &skeleton.bones.At(i);
            break;
        };
    };

    BGZ_ASSERT(bone->name != nullptr, "Bone was not found!");

    return bone;
};

v2f ParentTransform_1Vector(v2f localCoords, Transform parentTransform)
{
    ConvertToCorrectPositiveRadian($(parentTransform.rotation));

    Coordinate_Space parentSpace {};
    parentSpace.origin = parentTransform.translation;
    parentSpace.xBasis = v2f { CosR(parentTransform.rotation), SinR(parentTransform.rotation) };
    parentSpace.yBasis = parentTransform.scale.y * PerpendicularOp(parentSpace.xBasis);
    parentSpace.xBasis *= parentTransform.scale.x;

    v2f transformedCoords {};

    //This equation rotates first then moves to correct world position
    transformedCoords = parentSpace.origin + (localCoords.x * parentSpace.xBasis) + (localCoords.y * parentSpace.yBasis);

    return transformedCoords;
};

f32 RecursivelyAddBoneRotations(f32 rotation, Bone bone)
{
    rotation += bone.parentBoneSpace.rotation;

    if (bone.isRoot)
        return rotation;
    else
        return RecursivelyAddBoneRotations(rotation, *bone.parentBone);
};

f32 WorldRotation_Bone(Bone bone)
{
    if (bone.isRoot)
        return 0;
    else
        return RecursivelyAddBoneRotations(bone.parentBoneSpace.rotation, *bone.parentBone);
};

v2f WorldTransform_Bone(v2f vertToTransform, Bone boneToGrabTransformFrom)
{
    v2f parentLocalPos = ParentTransform_1Vector(vertToTransform, boneToGrabTransformFrom.parentBoneSpace);

    if (boneToGrabTransformFrom.isRoot) //If root bone has been hit then exit recursion by returning world pos of main bone
    {
        return parentLocalPos;
    }
    else
    {
        return WorldTransform_Bone(parentLocalPos, *boneToGrabTransformFrom.parentBone);
    };
};

inline void UpdateBoneChainsWorldPositions_StartingFrom(Bone&& mainBone)
{
    if (mainBone.childBones.size > 0)
    {
        for (i32 childBoneIndex {}; childBoneIndex < mainBone.childBones.size; ++childBoneIndex)
        {
            Bone* childBone = mainBone.childBones[childBoneIndex];
            childBone->worldSpace.translation = WorldTransform_Bone(childBone->parentBoneSpace.translation, *childBone->parentBone);

            UpdateBoneChainsWorldPositions_StartingFrom($(*childBone));
        };
    };
}

void UpdateSkeletonBoneWorldPositions(Skeleton&& fighterSkel, v2f fighterWorldPos)
{
    Bone* root = &fighterSkel.bones[0];

    UpdateBoneChainsWorldPositions_StartingFrom($(*root));

    for (i32 i {}; i < fighterSkel.bones.size; ++i)
    {
        fighterSkel.bones.At(i).worldSpace.translation += fighterWorldPos;
        fighterSkel.bones.At(i).worldSpace.rotation = WorldRotation_Bone(fighterSkel.bones.At(i));

        if (root->parentBoneSpace.scale.x == -1.0f)
            fighterSkel.bones.At(i).worldSpace.rotation = PI - fighterSkel.bones.At(i).worldSpace.rotation;
    };

    root->worldSpace.translation = fighterWorldPos;
    root->parentBoneSpace.translation = fighterWorldPos;
};

#endif //SKELETON_IMPL
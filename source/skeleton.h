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
    
    Transform parentBoneSpace;
    Transform worldSpace;
    f32 initialRotation_parentBoneSpace {};
    v2f initialPos_parentBoneSpace {};
    v2f initialTranslationForMixing {};
    f32 initialRotationForMixing {};
    f32 length {};
    Bone* parentBone { nullptr };
    RunTimeArr<v2f> originalCollisionBoxVerts;
    RunTimeArr<Bone*> childBones;
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
    
    RunTimeArr<Bone> bones;
    RunTimeArr<Slot> slots;
    f32 width {}, height {};
};

void InitSkel(Skeleton&& skel, Memory_Partition&& memPart, const char* atlasFilePath, const char* jsonFilePath);
Bone InitBone(Memory_Partition&& memPart);
Bone* GetBoneFromSkeleton(Skeleton* skeleton, char* boneName);
void ResetBonesToSetupPose(Skeleton&& skeleton);
Skeleton CopySkeleton(Skeleton src);

#endif

#ifdef SKELETON_IMPL

Bone InitBone(Memory_Partition&& memPart)
{
    Bone bone{};
    InitArr($(bone.originalCollisionBoxVerts), &memPart, 10);
    InitArr($(bone.childBones), &memPart, 5);
    
    return bone;
};

void InitSkel(Skeleton&& skel, Memory_Partition&& memPart, const char* atlasFilePath, const char* jsonFilePath)
{
    i32 length;
    
    const char* skeletonJson = globalPlatformServices->ReadEntireFile($(length), jsonFilePath);
    
    Atlas* atlas = CreateAtlasFromFile(atlasFilePath);
    
    if (skeletonJson)
    {
        Json* root {};
        root = Json_create(skeletonJson);
        
        Json* jsonSkeleton = Json_getItem(root, "skeleton"); /* clang-format off */BGZ_ASSERT(jsonSkeleton, "Unable to return valid json object for skeleton!"); /* clang-format on */
        Json* jsonBones = Json_getItem(root, "bones"); /* clang-format off */BGZ_ASSERT(jsonBones, "Unable to return valid json object for bones!"); /* clang-format on */
        Json* jsonSlots = Json_getItem(root, "slots"); /* clang-format off */BGZ_ASSERT(jsonSlots, "Unable to return valid json object for slots!"); /* clang-format on */
        Json* skins_json = Json_getItem(root, "skins");
        
        skel.width = Json_getFloat(jsonSkeleton, "width", 0.0f);
        skel.height = Json_getFloat(jsonSkeleton, "height", 0.0f);
        
        { //Read in Bone data
            i32 boneIndex {};
            InitArr($(skel.bones), &memPart, jsonBones->size);
            for (Json* currentBone_json = jsonBones->child; boneIndex < jsonBones->size; currentBone_json = currentBone_json->next, ++boneIndex)
            {
                skel.bones.Push() = InitBone($(memPart));
                Bone* bone = &skel.bones[boneIndex];
                
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
                            bone->originalCollisionBoxVerts.Push() = v2f { verts_json->valueFloat, verts_json->next->valueFloat };
                            verts_json = verts_json->next->next;
                        };
                    }
                }
                
                if (Json_getString(currentBone_json, "parent", 0)) //If no parent then skip
                {
                    bone->parentBone = GetBoneFromSkeleton(&skel, (char*)Json_getString(currentBone_json, "parent", 0));
                    bone->parentBone->childBones.Push() = bone;
                };
            };
        };
        
        { //Read in Slot data
            i32 slotIndex {};
            InitArr($(skel.slots), &memPart, jsonBones->size);
            for (Json* currentSlot_json = jsonSlots->child; slotIndex < jsonSlots->size; currentSlot_json = currentSlot_json->next, ++slotIndex)
            {
                //Ignore creating slots here for collision boxes. Don't think I need it
                char* slotName = (char*)Json_getString(currentSlot_json, "name", 0);
                char slotName_firstThreeLetters[4] = { slotName[0], slotName[1], slotName[2], NULL };
                if (NOT StringCmp(slotName_firstThreeLetters, "box"))
                {
                    //Insert slot info in reverse order to get correct draw order (since json file has the draw order flipped from spine application)
                    skel.slots.Push() = Slot{};
                    Slot* slot = &skel.slots[slotIndex];
                    
                    slot->name = (char*)Json_getString(currentSlot_json, "name", 0);
                    if (StringCmp(slot->name, "left-hand"))
                        int x {};
                    slot->bone = GetBoneFromSkeleton(&skel, (char*)Json_getString(currentSlot_json, "bone", 0));
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

//TODO: Not sure if this is working properly yet
Skeleton CopySkeleton(Skeleton src)
{
    Skeleton dest = src;
#if 0
    
    CopyArray(src.bones, $(dest.bones));
    CopyArray(src.slots, $(dest.slots));
    
    for (i32 boneIndex {}; boneIndex < src.bones.length; ++boneIndex)
    {
        for (i32 childBoneIndex {}; childBoneIndex < src.bones[boneIndex].childBones.size; ++childBoneIndex)
        {
            const char* childBoneName = src.bones[boneIndex].childBones.At(childBoneIndex)->name;
            Bone* bone = GetBoneFromSkeleton(&dest, (char*)childBoneName);
            dest.bones[boneIndex].childBones.At(childBoneIndex) = bone;
        }
    };
#endif
    return dest;
};

void ResetBonesToSetupPose(Skeleton&& skel)
{
    for (i32 boneIndex {}; boneIndex < skel.bones.length; ++boneIndex)
    {
        skel.bones[boneIndex].parentBoneSpace.rotation = skel.bones[boneIndex].initialRotation_parentBoneSpace;
        skel.bones[boneIndex].parentBoneSpace.translation = skel.bones[boneIndex].initialPos_parentBoneSpace;
    };
};

Bone* GetBoneFromSkeleton(Skeleton* skeleton, char* boneName)
{
    Bone* bone {};
    
    for (i32 i = 0; i < skeleton->bones.length; ++i)
    {
        if (StringCmp(skeleton->bones[i].name, boneName))
        {
            bone = &skeleton->bones[i];
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
    if (mainBone.childBones.length > 0)
    {
        for (i32 childBoneIndex {}; childBoneIndex < mainBone.childBones.length; ++childBoneIndex)
        {
            Bone* childBone = mainBone.childBones[childBoneIndex];
            childBone->worldSpace.translation = WorldTransform_Bone(childBone->parentBoneSpace.translation, *childBone->parentBone);
            
            UpdateBoneChainsWorldPositions_StartingFrom($(*childBone));
        };
    };
}

void UpdateSkeletonBoneWorldTransforms(Skeleton&& fighterSkel, v2f fighterWorldPos)
{
    Bone* root = &fighterSkel.bones[0];
    
    UpdateBoneChainsWorldPositions_StartingFrom($(*root));
    
    for (i32 i {}; i < fighterSkel.bones.length; ++i)
    {
        fighterSkel.bones[i].worldSpace.translation += fighterWorldPos;
        fighterSkel.bones[i].worldSpace.rotation = WorldRotation_Bone(fighterSkel.bones[i]);
        
        if (root->parentBoneSpace.scale.x == -1.0f)
            fighterSkel.bones[i].worldSpace.rotation = PI - fighterSkel.bones[i].worldSpace.rotation;
    };
    
    root->worldSpace.translation = fighterWorldPos;
    root->parentBoneSpace.translation = fighterWorldPos;
};

#endif //SKELETON_IMPL
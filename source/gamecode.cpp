/*
    TODO List:
    1.) Figure out if stb_image has a way to modify how images are read (so I can have all reads go through my platform services struct)
    2.) Figure out a way to remove pixelsPerMeter constant that I have strune about in my skeleton and animaiton intialization code
*/

#if (DEVELOPMENT_BUILD)
#define BGZ_LOGGING_ON true
#define BGZ_ERRHANDLING_ON true
#else
#define BGZ_LOGGING_ON false
#define BGZ_ERRHANDLING_ON false
#endif

#define _CRT_SECURE_NO_WARNINGS // to surpress things like 'use printf_s func instead!'

#define BGZ_MAX_CONTEXTS 10000
#include <boagz/error_handling.h>
#include <stb/stb_image.h>

#define ATOMIC_TYPES_IMPL
#include "atomic_types.h"

global_variable i32 heap;

#include "memory_handling.h"
#include "array.h"
#include "dynamic_array.h"
#include "hashmap_str.h"
#include "ring_buffer.h"
#include "linked_list.h"
#include <utility>

#include "shared.h"
#include "renderer_stuff.h"
#include "gamecode.h"
#include "my_math.h"
#include "utilities.h"

global_variable Platform_Services* globalPlatformServices;
global_variable Rendering_Info* global_renderingInfo;
global_variable f32 deltaT;
global_variable f32 deltaTFixed;
global_variable i32 renderBuffer;

//Third Party source
#define STB_IMAGE_IMPLEMENTATION
#define STBI_MALLOC(sz) MallocSize(heap, sz)
#define STBI_REALLOC(p, newsz) ReAllocSize(heap, p, newsz)
#define STBI_FREE(p) DeAlloc(heap, p)
#include <stb/stb_image.h>
#include <boagz/error_context.cpp>

//User source
#define MEMORY_HANDLING_IMPL
#include "memory_handling.h"
#define DYNAMIC_ALLOCATOR_IMPL
#include "dynamic_allocator.h"
#define LINEAR_ALLOCATOR_IMPL
#include "linear_allocator.h"
#define COLLISION_DETECTION_IMPL
#include "collision_detection.h"
#define JSON_IMPL
#include "json.h"
#define ATLAS_IMPL
#include "atlas.h"
#define SKELETON_IMPL
#include "skeleton.h"
#define ANIMATION_IMPL
#include "animation.h"
#define FIGHTER_IMPL
#include "fighter.h"
#define GAME_RENDERER_STUFF_IMPL
#include "renderer_stuff.h"

//Move out to Renderer eventually
#if 0
local_func
Image CreateEmptyImage(i32 width, i32 height)
{
    Image image{};

    i32 numBytesPerPixel{4};
    image.data = (ui8*)MallocSize(heap, width*height*numBytesPerPixel);
    image.size = v2i{width, height};
    image.pitch = width*numBytesPerPixel;

    return image;
};

local_func
void GenerateSphereNormalMap(Image&& sourceImage)
{
    f32 invWidth = 1.0f / (f32)(sourceImage.size.width - 1);
    f32 invHeight = 1.0f / (f32)(sourceImage.size.height - 1);
                                
    ui8* row = (ui8*)sourceImage.data;
    for(i32 y = 0; y < sourceImage.size.height; ++y)
    {
        ui32* pixel = (ui32*)row;

        for(i32 x = 0; x < sourceImage.size.width; ++x)
        {
            v2f normalUV = {invWidth*(f32)x, invHeight*(f32)y};
            
            f32 normalX = 2.0f*normalUV.x - 1.0f;
            f32 normalY = 2.0f*normalUV.y - 1.0f;
                                        
            f32 rootTerm = 1.0f - normalX*normalX - normalY*normalY;
            f32 normalZ = 0.0f;

            v3f normal {0.0f, 0.0f, 1.0f};

            if(rootTerm >= 0.0f)
            {
                normalZ = Sqrt(rootTerm);
                normal = v3f{normalX, normalY, normalZ};
            };

            //Convert from -1 to 1 range to value between 0 and 255
            v4f color = {255.0f*(.5f*(normal.x + 1.0f)),
                         255.0f*(.5f*(normal.y + 1.0f)),
                         255.0f*(.5f*(normal.z + 1.0f)),
                         0.0f};

            *pixel++ = (((ui8)(color.a + .5f) << 24) |
                        ((ui8)(color.r + .5f) << 16) |
                        ((ui8)(color.g + .5f) << 8) |
                        ((ui8)(color.b + .5f) << 0));
            };

        row += sourceImage.pitch;
    };
};

local_func 
Image FlipImage(Image image)
{
    i32 widthInBytes = image.size.width * 4;
    ui8* p_topRowOfTexels = nullptr;
    ui8* p_bottomRowOfTexels = nullptr;
    ui8 temp = 0;
    i32 halfHeight = image.size.height / 2;

    for (i32 row = 0; row < halfHeight; ++row)
    {
        p_topRowOfTexels = image.data + row * widthInBytes;
        p_bottomRowOfTexels = image.data + (image.size.height - row - 1) * widthInBytes;

        for (i32 col = 0; col < widthInBytes; ++col)
        {
            temp = *p_topRowOfTexels;
            *p_topRowOfTexels = *p_bottomRowOfTexels;
            *p_bottomRowOfTexels = temp;
            p_topRowOfTexels++;
            p_bottomRowOfTexels++;
        };
    };

    return image;
};
#endif

inline b KeyPressed(Button_State KeyState)
{
    if (KeyState.Pressed && KeyState.NumTransitionsPerFrame)
    {
        return true;
    };

    return false;
};

inline b KeyComboPressed(Button_State KeyState1, Button_State KeyState2)
{
    if (KeyState1.Pressed && KeyState2.Pressed && (KeyState1.NumTransitionsPerFrame || KeyState2.NumTransitionsPerFrame))
    {
        return true;
    };

    return false;
};

inline b KeyHeld(Button_State KeyState)
{
    if (KeyState.Pressed && (KeyState.NumTransitionsPerFrame == 0))
    {
        return true;
    };

    return false;
};

inline b KeyComboHeld(Button_State KeyState1, Button_State KeyState2)
{
    if (KeyState1.Pressed && KeyState2.Pressed && (KeyState1.NumTransitionsPerFrame == 0 && KeyState2.NumTransitionsPerFrame == 0))
    {
        return true;
    };

    return false;
};

inline b KeyReleased(Button_State KeyState)
{
    if (!KeyState.Pressed && KeyState.NumTransitionsPerFrame)
    {
        return true;
    };

    return false;
};

f32 WidthInMeters(Image bitmap, f32 heightInMeters)
{
    f32 width_meters = bitmap.aspectRatio * heightInMeters;

    return width_meters;
};

f32 RecursivelyAddBoneRotations(f32 rotation, Bone bone)
{
    rotation += *bone.rotation_parentBoneSpace;

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
        return RecursivelyAddBoneRotations(*bone.rotation_parentBoneSpace, *bone.parentBone);
};

v2f ParentTransform_1Vector(v2f localCoords, Transform parentTransform)
{
    //With world space origin at 0, 0
    Coordinate_Space localSpace {};
    localSpace.origin = parentTransform.translation;
    localSpace.xBasis = v2f { CosR(parentTransform.rotation), SinR(parentTransform.rotation) };
    localSpace.yBasis = parentTransform.scale.y * PerpendicularOp(localSpace.xBasis);
    localSpace.xBasis *= parentTransform.scale.x;

    v2f transformedCoords {};

    //This equation rotates first then moves to correct world position
    transformedCoords = localSpace.origin + (localCoords.x * localSpace.xBasis) + (localCoords.y * localSpace.yBasis);

    return transformedCoords;
};

v2f WorldTransform_Bone(v2f vertToTransform, Bone boneToGrabTransformFrom)
{
    v2f parentLocalPos = ParentTransform_1Vector(vertToTransform, boneToGrabTransformFrom.transform);

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
            childBone->pos_worldSpace = WorldTransform_Bone(*childBone->pos_parentBoneSpace, *childBone->parentBone);

            UpdateBoneChainsWorldPositions_StartingFrom($(*childBone));
        };
    };
}

void UpdateSkeletonBoneWorldPositions(Skeleton&& fighterSkel, v2f fighterWorldPos)
{
    Bone* root = &fighterSkel.bones[0];
    Bone* pelvis = &fighterSkel.bones[1];

    root->transform.scale.x = 1.0f;

    UpdateBoneChainsWorldPositions_StartingFrom($(*root));

    for (i32 i {}; i < fighterSkel.bones.size; ++i)
    {
        //v2f flippedX = {fighterSkel.bones.At(i).worldPos.x * -1.0f, fighterSkel.bones.At(i).worldPos.y};
        //fighterSkel.bones.At(i).worldPos = flippedX;
        fighterSkel.bones.At(i).pos_worldSpace += fighterWorldPos;
        fighterSkel.bones.At(i).rotation_worldSpace = WorldRotation_Bone(fighterSkel.bones.At(i));
        //fighterSkel.bones.At(i).worldRotation = PI - fighterSkel.bones.At(i).worldRotation;
    };

    root->pos_worldSpace = fighterWorldPos;
    root->transform.translation = fighterWorldPos;
};

extern "C" void GameUpdate(Application_Memory* gameMemory, Platform_Services* platformServices, Rendering_Info* renderingInfo, Game_Sound_Output_Buffer* soundOutput, Game_Input* gameInput)
{
    auto TranslateCurrentMeasurementsToGameUnits = [](Skeleton&& skel, AnimationData&& animData) {
        f32 pixelsPerMeter { global_renderingInfo->_pixelsPerMeter };

        skel.width /= pixelsPerMeter;
        skel.height /= pixelsPerMeter;

        for (i32 boneIndex {}; boneIndex < skel.bones.size; ++boneIndex)
        {
            skel.bones.At(boneIndex).transform.translation.x /= pixelsPerMeter;
            skel.bones.At(boneIndex).transform.translation.y /= pixelsPerMeter;
            skel.bones.At(boneIndex).initialPos_parentBoneSpace.x /= pixelsPerMeter;
            skel.bones.At(boneIndex).initialPos_parentBoneSpace.y /= pixelsPerMeter;

            skel.bones.At(boneIndex).transform.rotation = Radians(skel.bones.At(boneIndex).transform.rotation);
            skel.bones.At(boneIndex).initialRotation_parentBoneSpace = Radians(skel.bones.At(boneIndex).initialRotation_parentBoneSpace);

            skel.bones.At(boneIndex).length /= pixelsPerMeter;
        };

        for (i32 slotI {}; slotI < skel.slots.size; ++slotI)
        {
            StringCmp(skel.slots.At(slotI).name, "left-hand");
                int x{};

            skel.slots.At(slotI).regionAttachment.height /= pixelsPerMeter;
            skel.slots.At(slotI).regionAttachment.width /= pixelsPerMeter;
            skel.slots.At(slotI).regionAttachment.rotation_parentBoneSpace = Radians(skel.slots.At(slotI).regionAttachment.rotation_parentBoneSpace);
            skel.slots.At(slotI).regionAttachment.pos_parentBoneSpace.x /= pixelsPerMeter;
            skel.slots.At(slotI).regionAttachment.pos_parentBoneSpace.y /= pixelsPerMeter;
        };

        for (i32 animIndex {}; animIndex < animData.animations.keyInfos.size; ++animIndex)
        {
            Animation* anim = (Animation*)&animData.animations.keyInfos.At(animIndex).value;

            if (anim->name)
            {
                for (i32 boneIndex {}; boneIndex < anim->bones.size; ++boneIndex)
                {
                    TranslationTimeline* boneTranslationTimeline = &anim->boneTranslationTimelines.At(boneIndex);
                    for (i32 keyFrameIndex {}; keyFrameIndex < boneTranslationTimeline->translations.size; ++keyFrameIndex)
                    {
                        boneTranslationTimeline->translations.At(keyFrameIndex).x /= pixelsPerMeter;
                        boneTranslationTimeline->translations.At(keyFrameIndex).y /= pixelsPerMeter;
                    }

                    RotationTimeline* boneRotationTimeline = &anim->boneRotationTimelines.At(boneIndex);
                    for (i32 keyFrameIndex {}; keyFrameIndex < boneRotationTimeline->angles.size; ++keyFrameIndex)
                    {
                        boneRotationTimeline->angles.At(keyFrameIndex) = Radians(boneRotationTimeline->angles.At(keyFrameIndex));
                    }
                };

                for (i32 hitBoxIndex {}; hitBoxIndex < anim->hitBoxes.size; ++hitBoxIndex)
                {
                    anim->hitBoxes.At(hitBoxIndex).size.width /= pixelsPerMeter;
                    anim->hitBoxes.At(hitBoxIndex).size.height /= pixelsPerMeter;
                    anim->hitBoxes.At(hitBoxIndex).worldPosOffset.x /= pixelsPerMeter;
                    anim->hitBoxes.At(hitBoxIndex).worldPosOffset.y /= pixelsPerMeter;
                };
            };
        }
    };

    BGZ_ERRCTXT1("When entering GameUpdate");

    const Game_Controller* keyboard = &gameInput->Controllers[0];
    const Game_Controller* gamePad = &gameInput->Controllers[1];

    Game_State* gState = (Game_State*)gameMemory->PermanentStorage;

    //Setup globals
    deltaT = platformServices->prevFrameTimeInSecs;
    deltaTFixed = platformServices->targetFrameTimeInSecs;
    globalPlatformServices = platformServices;
    global_renderingInfo = renderingInfo;

    Stage_Data* stage = &gState->stage;
    Fighter* player = &stage->player;
    Fighter* enemy = &stage->enemy;
    Fighter* enemy2 = &stage->enemy2;

    if (NOT gameMemory->Initialized)
    {
        BGZ_ERRCTXT1("When Initializing game memory and game state");

        gameMemory->Initialized = true;

        { //Initialize memory/allocator stuff
            InitApplicationMemory(gameMemory);
            heap = CreatePartitionFromMemoryBlock(gameMemory, Megabytes(800), DYNAMIC);
            InitDynamAllocator(heap);
        };

        *gState = {}; //Make sure everything gets properly defaulted (constructors are called that need to be)

        //Read in data
        Skeleton playerSkel { "data/yellow_god.atlas", "data/yellow_god.json", heap }; //TODO: In order to reduce the amount of time reading from json file think about how to implement one common skeleton/animdata file(s)
        Skeleton enemySkel { "data/yellow_god.atlas", "data/yellow_god.json", heap };
        AnimationData playerAnimData { "data/yellow_god.json", playerSkel };
        AnimationData enemyAnimData { "data/yellow_god.json", enemySkel };

        //Translate pixels to meters and degrees to radians (since spine exports everything in pixel/degree units)
        //TranslateCurrentMeasurementsToGameUnits($(playerSkel), $(playerAnimData));
        //TranslateCurrentMeasurementsToGameUnits($(enemySkel), $(enemyAnimData));

        //Stage Init
        stage->backgroundImg = LoadBitmap_BGRA("data/4k.jpg");
        stage->size.height = 40.0f;
        stage->size.width = WidthInMeters(stage->backgroundImg, stage->size.height);
        stage->centerPoint = { (f32)WidthInMeters(stage->backgroundImg, stage->size.height) / 2, (f32)stage->size.height / 2 };

        //Camera Init
        stage->camera.dilatePointOffset_normalized = { 0.0f, -0.3f };
        stage->camera.lookAt = { stage->size.width / 2.0f, 10.0f };
        stage->camera.zoomFactor = .4f;

        //Init fighters
        v2f playerWorldPos = { (stage->size.width / 2.0f) - 6.0f, 3.0f }, enemyWorldPos = { (stage->size.width / 2.0f) + 6.0f, 3.0f };
        HurtBox playerDefaultHurtBox { playerWorldPos, v2f { 2.0f, 8.9f }, v2f { 2.3f, 2.3f } };
        HurtBox enemyDefaultHurtBox { enemyWorldPos, v2f { 2.0f, 8.9f }, v2f { 2.3f, 2.3f } };
        *player = { playerSkel, playerAnimData, playerWorldPos, /*player height*/ playerSkel.height, playerDefaultHurtBox };
        *enemy = { enemySkel, enemyAnimData, enemyWorldPos, /*enemy height*/ enemySkel.height, enemyDefaultHurtBox };

        Slot* slot = &player->skel.slots[0];

        MixAnimations($(player->animData), "idle", "walk", .2f);
        MixAnimations($(player->animData), "walk", "run", .2f);
        MixAnimations($(player->animData), "right-jab", "idle", .1f);

        SetIdleAnimation($(player->animQueue), player->animData, "idle");
        SetIdleAnimation($(enemy->animQueue), enemy->animData, "idle");
    };

    if (globalPlatformServices->DLLJustReloaded)
    {
        /*
            player->world.pos = -1.0f;
            player->bone.pos = -3.0f;
        */
        BGZ_CONSOLE("Dll reloaded!");
        globalPlatformServices->DLLJustReloaded = false;
    };

    if (KeyHeld(keyboard->MoveRight))
    {
        player->world.translation.x += .1f;
        QueueAnimation($(player->animQueue), player->animData, "walk", PlayBackStatus::NEXT);
    };

    if (KeyHeld(keyboard->MoveLeft))
    {
        player->world.translation.x -= .1f;
    }

    if (KeyHeld(keyboard->MoveUp))
    {
        stage->camera.zoomFactor += .06f;
    };

    if (KeyHeld(keyboard->MoveDown))
    {
        stage->camera.zoomFactor -= .02f;
    };

    if (KeyPressed(keyboard->ActionLeft))
    {
        QueueAnimation($(player->animQueue), player->animData, "left-jab", PlayBackStatus::IMMEDIATE);
    };

    if (KeyComboHeld(keyboard->ActionLeft, keyboard->MoveRight))
    {
        QueueAnimation($(player->animQueue), player->animData, "run", PlayBackStatus::DEFAULT);
    }

    if (KeyPressed(keyboard->ActionRight))
    {
        QueueAnimation($(player->animQueue), player->animData, "right-cross", PlayBackStatus::IMMEDIATE);
    };

    player->currentAnim = UpdateAnimationState($(player->animQueue), deltaT);
    enemy->currentAnim = UpdateAnimationState($(enemy->animQueue), deltaT);
    defer { CleanUpAnimation($(player->currentAnim)); };
    defer { CleanUpAnimation($(enemy->currentAnim)); };

    ApplyAnimationToSkeleton($(player->skel), player->currentAnim);
    ApplyAnimationToSkeleton($(enemy->skel), enemy->currentAnim);

    UpdateSkeletonBoneWorldPositions($(player->skel), player->world.translation);
    UpdateSkeletonBoneWorldPositions($(enemy->skel), enemy->world.translation);

    UpdateCollisionBoxWorldPos_BasedOnCenterPoint($(player->hurtBox), player->world.translation);
    UpdateCollisionBoxWorldPos_BasedOnCenterPoint($(enemy->hurtBox), enemy->world.translation);

    for (i32 hitBoxIndex {}; hitBoxIndex < player->currentAnim.hitBoxes.size; ++hitBoxIndex)
    {
        UpdateHitBoxStatus($(player->currentAnim.hitBoxes.At(hitBoxIndex)), player->currentAnim.currentTime);

        if (player->currentAnim.hitBoxes.At(hitBoxIndex).isActive)
        {
            player->currentAnim.hitBoxes.At(hitBoxIndex).pos_worldSpace = { 0.0f, 0.0f };

            Bone* bone = GetBoneFromSkeleton(player->skel, player->currentAnim.hitBoxes.At(hitBoxIndex).boneName);
            UpdateCollisionBoxWorldPos_BasedOnCenterPoint($(player->currentAnim.hitBoxes.At(hitBoxIndex)), bone->pos_worldSpace);
            b collisionOccurred = CheckForFighterCollisions_AxisAligned(player->currentAnim.hitBoxes.At(hitBoxIndex), enemy->hurtBox);

            if (collisionOccurred)
                BGZ_CONSOLE("ahhahha");
        };
    };

    UpdateCamera(global_renderingInfo, stage->camera.lookAt, stage->camera.zoomFactor, stage->camera.dilatePointOffset_normalized);

    { //Render
        /*
            v2f pos_boneSpace = BoneTransform(slot->bone.transform);
            v2f pos_worldSpace = WorldTransform(pos_boneSpace, worldTransform)
            Push(pos_worldSpace, worldTransform);

            v2f pos_local = regionAttachemnt.localTransform.translation;
            v2f pos_boneSpace = BoneTransform(pos_local, bone->transform)
            v2f 

            Quadf image_localCoords = ProduceCoords(regionAtatchment.localTransform, width, height);
            Quadf image_boneCoords = ParentTransform(localCoords, bone->transform);
            Quadf image_worldCoords = ParentTransform(image_boneCoords, bone->worldTransform);

            Push(worldCoords);

            ConvertToCorrectPositiveRadian($(textureEntry.world.rotation));

            Object_Transform transform { .772f, v2f { .538f, .057f }, v2f { 1.0f, 1.0f } };
            Quadf imageTargetRect_parentBone = ProduceWorldCoordsFromCenterPoint(v2f { 0.0f, 0.0f }, textureEntry.dimensions, transform);
            imageTargetRect_parentBone.bottomLeft = { imageTargetRect_parentBone.bottomLeft.x, imageTargetRect_parentBone.bottomLeft.y };
            imageTargetRect_parentBone.bottomRight = { imageTargetRect_parentBone.bottomRight.x, imageTargetRect_parentBone.bottomRight.y };
            imageTargetRect_parentBone.topRight = { imageTargetRect_parentBone.topRight.x, imageTargetRect_parentBone.topRight.y };
            imageTargetRect_parentBone.topLeft = { imageTargetRect_parentBone.topLeft.x, imageTargetRect_parentBone.topLeft.y };

            Quadf thing = WorldTransform(imageTargetRect_parentBone, textureEntry.world);
        */
        auto DrawFighter = [](Fighter fighter) -> void {
            for (i32 slotIndex { 0 }; slotIndex < 2; ++slotIndex)
            {
                Slot* currentSlot = &fighter.skel.slots[slotIndex];

                AtlasRegion* region = &currentSlot->regionAttachment.region_image;

                if (StringCmp(currentSlot->bone->name, "left-hand"))
                    int x {};

                Array<v2f, 2> uvs2 = { v2f { region->u, region->v }, v2f { region->u2, region->v2 } };

                //v2f worldPosOfImage = ParentTransform_1Vector(v2f{0.0f, 0.0f}, Transform { currentSlot->regionAttachment.rotation_parentBoneSpace, currentSlot->regionAttachment.pos_parentBoneSpace, { 1.0f, 1.0f } });
                Quadf targetRect_localCoords = ProduceQuadFromCenterPoint(currentSlot->regionAttachment.pos_parentBoneSpace, currentSlot->regionAttachment.width, currentSlot->regionAttachment.height);

                Object_Transform boneTransform{currentSlot->regionAttachment.rotation_parentBoneSpace, currentSlot->regionAttachment.pos_parentBoneSpace, currentSlot->regionAttachment.scale_parentBoneSpace};
                Quadf targetRect_boneSpaceCoords = WorldTransform(targetRect_localCoords, boneTransform);

                ConvertToCorrectPositiveRadian($(currentSlot->bone->rotation_worldSpace));
                Object_Transform worldTransform{currentSlot->bone->rotation_worldSpace, currentSlot->bone->pos_worldSpace, *currentSlot->bone->scale};
                Quadf targetRect_worldSpaceCoords = WorldTransform(targetRect_boneSpaceCoords, worldTransform);

                PushTexture(global_renderingInfo, targetRect_worldSpaceCoords, region->page->rendererObject, v2f { currentSlot->regionAttachment.width, currentSlot->regionAttachment.height }, uvs2, region->name);

                //v2f worldPosOfImage = ParentTransform_1Vector(currentSlot->regionAttachment.pos_parentBoneSpace, Transform { currentSlot->bone->rotation_worldSpace, currentSlot->bone->pos_worldSpace, { 1.0f, 1.0f } });
                //f32 worldRotationOfImage = currentSlot->regionAttachment.rotation_parentBoneSpace + currentSlot->bone->rotation_worldSpace;
                //v2f worldSclaeOfImage = { currentSlot->regionAttachment.scale.x, currentSlot->regionAttachment.scale.y };
            };
        };

        //Push Fighters
        DrawFighter(*player);

#if 0
        //Push background
        Array<v2f, 2> uvs = { v2f { 0.0f, 0.0f }, v2f { 1.0f, 1.0f } };
        PushTexture(global_renderingInfo, stage->backgroundImg, stage->size.height, 0.0f, v2f { stage->size.width / 2.0f, stage->size.height / 2.0f }, v2f { 1.0f, 1.0f }, uvs, "background");

        for (i32 i {}; i < player->skel.bones.size; ++i)
        {
            Bone bone = player->skel.bones.At(i);
            PushRect(global_renderingInfo, bone.pos_worldSpace, 0.0f, { 1.0f, 1.0f }, { .2f, .2f }, { 1.0f, 0.0f, 0.0f });
        }
#endif

#if 0
        //Draw collision boxes
        PushRect(global_renderingInfo, enemy->hurtBox.worldPos, 0.0f, {1.0f, 1.0f}, enemy->hurtBox.size, {1.0f, 0.0f, 0.0f});
        PushRect(global_renderingInfo, player->hurtBox.worldPos, 0.0f, {1.0f, 1.0f}, player->hurtBox.size, {1.0f, 0.0f, 0.0f});

        for(i32 hitBoxIndex{}; hitBoxIndex < player->currentAnim.hitBoxes.size; ++hitBoxIndex)
        {
            if(player->currentAnim.hitBoxes.At(hitBoxIndex).isActive)
                PushRect(global_renderingInfo, player->currentAnim.hitBoxes.At(hitBoxIndex).worldPos, 0.0f, {1.0f, 1.0f}, player->currentAnim.hitBoxes.At(hitBoxIndex).size, {0.0f, 1.0f, 0.0f});
        };
#endif
    };
};

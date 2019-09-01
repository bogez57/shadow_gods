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
#include "math.h"
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
#define COLLISION_IMPL
#include "collisions.h"
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

v2f ParentTransform_1Vector(v2f localCoords, Transform parentTransform)
{
    //With world space origin at 0, 0
    Coordinate_Space localSpace{};
    localSpace.origin = parentTransform.translation;
    localSpace.xBasis = v2f{ CosR(parentTransform.rotation), SinR(parentTransform.rotation) };
    localSpace.yBasis = parentTransform.scale.y * PerpendicularOp(localSpace.xBasis);
    localSpace.xBasis *= parentTransform.scale.x;

    v2f transformedCoords{};

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
        for (i32 childBoneIndex{}; childBoneIndex < mainBone.childBones.size; ++childBoneIndex)
        {
            Bone* childBone = mainBone.childBones[childBoneIndex];
            childBone->worldPos = WorldTransform_Bone(*childBone->parentLocalPos, *childBone->parentBone);

            UpdateBoneChainsWorldPositions_StartingFrom($(*childBone));
        };
    };
}

void UpdateSkeletonBoneWorldPositions(Skeleton&& fighterSkel, v2f fighterWorldPos)
{
    Bone* root = &fighterSkel.bones[0];
    Bone* pelvis = &fighterSkel.bones[1];

    root->worldPos = fighterWorldPos;
    root->transform.translation = fighterWorldPos;

    UpdateBoneChainsWorldPositions_StartingFrom($(*root));
};

f32 RecursivelyAddBoneRotations(f32 rotation, Bone bone)
{
    rotation += *bone.parentLocalRotation;

    if (bone.isRoot)
        return rotation;
    else
        return RecursivelyAddBoneRotations(rotation, *bone.parentBone);
};

f32 WorldRotation_Bone(Bone bone)
{
    return RecursivelyAddBoneRotations(*bone.parentLocalRotation, *bone.parentBone);
};

extern "C" void GameUpdate(Application_Memory* gameMemory, Platform_Services* platformServices, Rendering_Info* renderingInfo, Game_Sound_Output_Buffer* soundOutput, Game_Input* gameInput)
{
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

        //Stage Init
        stage->backgroundImg = LoadBitmap_BGRA("data/4k.jpg");
        stage->size.height = 20.0f;
        stage->size.width = WidthInMeters(stage->backgroundImg, stage->size.height);
        stage->centerPoint = { (f32)WidthInMeters(stage->backgroundImg, stage->size.height) / 2, (f32)stage->size.height / 2 };

        InitRenderStuff(global_renderingInfo, /*screen dims*/ v2f{ 1280.0f, 720.0f }, /*look at point*/ viewPortDimensions_Meters(global_renderingInfo) / 2.0f, /*pixelsPerMeter*/ 100.0f);

        //Camera Init
        v2f viewDims = viewPortDimensions_Meters(global_renderingInfo);
        stage->camera.lookAt = { stage->size.width / 2.0f, 4.0f };
        stage->camera.dilatePoint_inScreenDims = viewDims / 2.0f;
        stage->camera.zoomFactor = 1.0f;

        //Init fighters
        v2f playerWorldPos = { (stage->size.width / 2.0f) - 2.0f, 3.0f }, enemyWorldPos = { (stage->size.width / 2.0f) + 2.0f, 3.0f };
        *player = {"data/yellow_god.atlas", "data/yellow_god.json", playerWorldPos, /*player height*/ 2.0f};
        *enemy = {"data/yellow_god.atlas", "data/yellow_god.json", enemyWorldPos, /*enemy height*/ 2.0f};

        SetIdleAnimation($(player->animQueue), player->animData, "idle");
    };

    if (globalPlatformServices->DLLJustReloaded)
    {
        BGZ_CONSOLE("Dll reloaded!");
        globalPlatformServices->DLLJustReloaded = false;
    };

    if (KeyHeld(keyboard->MoveRight))
    {
        player->world.translation.x += .1f;
    };

    if (KeyHeld(keyboard->MoveLeft))
    {
        player->world.translation.x -= .1f;
    }

    if (KeyHeld(keyboard->MoveUp))
    {
        stage->camera.zoomFactor += .02f;
    };

    if (KeyHeld(keyboard->MoveDown))
    {
        stage->camera.zoomFactor -= .02f;
    };

    if (KeyPressed(keyboard->ActionRight))
    {
        QueueAnimation($(player->animQueue), player->animData, "high_kick", PlayBackStatus::NEXT);
    };

    if (KeyPressed(keyboard->ActionLeft))
    {
        QueueAnimation($(player->animQueue), player->animData, "low_kick", PlayBackStatus::IMMEDIATE);
    };

    UpdateAnimationState($(player->animQueue), &player->skel.bones, deltaT);
    ApplyAnimationToSkeleton($(player->skel), $(player->animQueue));

    ChangeCameraSettings(global_renderingInfo, stage->camera.lookAt, stage->camera.zoomFactor, stage->camera.dilatePoint_inScreenDims);

    UpdateSkeletonBoneWorldPositions($(player->skel), player->world.translation);
    UpdateSkeletonBoneWorldPositions($(enemy->skel), enemy->world.translation);

    Array<v2f, 2> uvs = { v2f{ 0.0f, 0.0f }, v2f{ 1.0f, 1.0f } };
    PushTexture(global_renderingInfo, stage->backgroundImg, stage->size.height, 0.0f, v2f{ stage->size.width / 2.0f, stage->size.height / 2.0f }, v2f{ 1.0f, 1.0f }, uvs, "background");

    auto DrawFighter = [](Fighter fighter) -> void {
        for (i32 slotIndex{ 0 }; slotIndex < fighter.skel.slots.size; ++slotIndex)
        {
            Slot* currentSlot = &fighter.skel.slots[slotIndex];

			Bone* bone = GetBoneFromSkeleton($(fighter.skel), "root");

            AtlasRegion* region = &currentSlot->regionAttachment.region_image;
            Array<v2f, 2> uvs2 = { v2f{ region->u, region->v }, v2f{ region->u2, region->v2 } };

            v2f worldPosOfImage = WorldTransform_Bone(currentSlot->regionAttachment.parentBoneLocalPos, *currentSlot->bone);
            f32 worldRotationOfBone = WorldRotation_Bone(*currentSlot->bone);
            f32 worldRotationOfImage = currentSlot->regionAttachment.parentBoneLocalRotation + worldRotationOfBone;

            PushTexture(global_renderingInfo, region->page->rendererObject, v2f{ currentSlot->regionAttachment.width, currentSlot->regionAttachment.height }, worldRotationOfImage, worldPosOfImage, fighter.world.scale, uvs2, region->name);
        };
    };

    DrawFighter(*player);
    DrawFighter(*enemy);
};

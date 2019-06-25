/*
    TODO List:
    1.) Figure out if stb_image has a way to modify how images are read (so I can have all reads go through my platform services struct)
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
#include "memory_handling.h"
#include "array.h"
#include "dynamic_array.h"
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
global_variable i32 heap;
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

void InitFighter(Fighter&& fighter, const char* atlasFilePath, const char* skelJsonFilePath, v2f worldPosition)
{
    Atlas* atlas;
    atlas = CreateAtlasFromFile(atlasFilePath, 0);
    fighter.skel = CreateSkeletonUsingJsonFile(*atlas, skelJsonFilePath);
    fighter.world.pos = worldPosition;
    fighter.world.rotation = 0.0f;
    fighter.world.scale = {1.0f, 1.0f};
    fighter.skel.worldPos = &fighter.world.pos;
};

v2f ParentTransform_1Vector(v2f localCoords, v2f worldPos, f32 worldRotation, v2f worldScale)
{
    //With world space origin at 0, 0
    Coordinate_Space imageSpace{};
    imageSpace.origin = worldPos;
    imageSpace.xBasis = v2f{CosR(worldRotation), SinR(worldRotation)};
    imageSpace.yBasis = worldScale.y * PerpendicularOp(imageSpace.xBasis);
    imageSpace.xBasis *= worldScale.x;

    v2f transformedCoords{};

    //This equation rotates first then moves to correct world position
    transformedCoords = imageSpace.origin + (localCoords.x * imageSpace.xBasis) + (localCoords.y * imageSpace.yBasis);

    return transformedCoords;
};

v2f WorldTransform_Bone(v2f parentLocalPosOfChildBone, Bone mainBone)
{
    Bone parentBone = *mainBone.parentBone;
    v2f pelvisLocalPos = ParentTransform_1Vector(parentLocalPosOfChildBone, parentBone.parentLocalPos, parentBone.rotation, v2f{1.0f, 1.0f});

    if(NOT parentBone.parentBone)//If root bone has been hit then exit recursion by returning world pos of main bone
    {
        return pelvisLocalPos;
    }
    else
    {
        return WorldTransform_Bone(pelvisLocalPos, parentBone);
    };
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
        *gState = {}; //Make sure everything gets properly defaulted (constructors are called that need to be)

        {//Initialize memory/allocator stuff
            InitApplicationMemory(gameMemory);
            heap = CreatePartitionFromMemoryBlock(gameMemory, Megabytes(200), DYNAMIC);
            InitDynamAllocator(heap);
        };

        //Stage Init
        stage->backgroundImg = LoadBitmap_BGRA("data/4k.jpg");
        stage->size.height = 16.0f;
        stage->size.width = WidthInMeters(stage->backgroundImg, stage->size.height);
        stage->centerPoint = { (f32)WidthInMeters(stage->backgroundImg, stage->size.height) / 2, (f32)stage->size.height / 2 };

        InitRenderStuff(global_renderingInfo, /*screen dims*/v2f{1280.0f, 720.0f}, /*look at point*/viewPortDimensions_Meters(global_renderingInfo) / 2.0f, /*pixelsPerMeter*/100.0f);

        //Camera Init
        v2f viewDims = viewPortDimensions_Meters(global_renderingInfo);
        stage->camera.lookAt = viewDims / 2.0f;
        stage->camera.dilatePoint = v2f{0.0f, 0.0f};
        stage->camera.zoomFactor = 1.0f;

        //Player Init
        v2f playerWorldPos = {1.2f, -2.5f};
        InitFighter($(*player), "data/yellow_god.atlas", "data/yellow_god.json", playerWorldPos);
    };

    player->height = 2.0f;

    if (globalPlatformServices->DLLJustReloaded)
    {
        BGZ_CONSOLE("Dll reloaded!");
        globalPlatformServices->DLLJustReloaded = false;
    };

    if(KeyHeld(keyboard->MoveRight))
    {
        Bone* root = &player->skel.bones[0];
        root->rotation -= .01f;
    };

    if(KeyHeld(keyboard->MoveUp))
    {
        stage->camera.zoomFactor += .01f;
    };

    {//Set bones to setup pose
        Bone* root = &player->skel.bones[0];
        Bone* pelvis = &player->skel.bones[1];

        root->worldPos = player->world.pos;
        root->parentLocalPos = player->world.pos;
        PushRect(global_renderingInfo, root->worldPos, 0.0f, v2f{1.0f, 1.0f}, v2f{.1f, .1f}, v3f{0.0f, 0.0f, 1.0f});

        pelvis->worldPos = WorldTransform_Bone(pelvis->parentLocalPos, *pelvis);
        PushRect(global_renderingInfo, pelvis->worldPos, 0.0f, v2f{1.0f, 1.0f}, v2f{.1f, .1f}, v3f{0.0f, 1.0f, 0.0f});

        for(i32 childBoneIndex{}; childBoneIndex < pelvis->childBones.size; ++childBoneIndex)
        {
            Bone* childBone = pelvis->childBones[childBoneIndex];
            childBone->worldPos = WorldTransform_Bone(childBone->parentLocalPos, *childBone);
            PushRect(global_renderingInfo, childBone->worldPos, 0.0f, v2f{1.0f, 1.0f}, v2f{.1f, .1f}, v3f{1.0f, 0.0f, 0.0f});

            if(childBone->childBones.size > 0)
            {
                for(i32 childBoneIndex{}; childBoneIndex < childBone->childBones.size; ++childBoneIndex)
                {
                    Bone* childBone2 = childBone->childBones[childBoneIndex];
                    childBone2->worldPos = WorldTransform_Bone(childBone2->parentLocalPos, *childBone2);
                    PushRect(global_renderingInfo, childBone2->worldPos, 0.0f, v2f{1.0f, 1.0f}, v2f{.1f, .1f}, v3f{1.0f, 0.0f, 0.0f});

                    if(childBone2->childBones.size > 0)
                    {
                        for(i32 childBoneIndex{}; childBoneIndex < childBone2->childBones.size; ++childBoneIndex)
                        {
                            Bone* childBone3 = childBone2->childBones[childBoneIndex];
                            childBone3->worldPos = WorldTransform_Bone(childBone3->parentLocalPos, *childBone3);
                            PushRect(global_renderingInfo, childBone3->worldPos, 0.0f, v2f{1.0f, 1.0f}, v2f{.1f, .1f}, v3f{1.0f, 0.0f, 0.0f});
                            
                            if(childBone3->childBones.size > 0)
                            {
                                for(i32 childBoneIndex{}; childBoneIndex < childBone2->childBones.size; ++childBoneIndex)
                                {
                                    Bone* childBone4 = childBone3->childBones[childBoneIndex];
                                    childBone4->worldPos = WorldTransform_Bone(childBone4->parentLocalPos, *childBone4);
                                    PushRect(global_renderingInfo, childBone4->worldPos, 0.0f, v2f{1.0f, 1.0f}, v2f{.1f, .1f}, v3f{1.0f, 0.0f, 0.0f});
                                };
                            };
                        };
                    };
                };
            };
        };
    }

#if 0
    {//Next: 
        for(i32 slotIndex{0}; slotIndex < player->skel.slots.size; ++slotIndex)
        {
            Slot* currentSlot = &player->skel.slots[slotIndex];

            AtlasRegion* region = &currentSlot->regionAttachment.region_image;
            Array<v2f, 2> uvs2 = {v2f{region->u, region->v}, v2f{region->u2, region->v2}};
            PushTexture(global_renderingInfo, region->page->rendererObject, v2f{currentSlot->regionAttachment.width, currentSlot->regionAttachment.height}, player->world.rotation, currentSlot->bone->worldPos, player->world.scale, uvs2);
        };
    };
 #endif

    //Array<v2f, 2> uvs = {v2f{0.0f, 0.0f}, v2f{1.0f, 1.0f}};
    //PushTexture(global_renderingInfo, stage->backgroundImg, stage->size.height, 0.0f, v2f{0.0f, 0.0f}, v2f{1.0f, 1.0f}, uvs);

    ChangeCameraSettings(global_renderingInfo, stage->camera.lookAt, stage->camera.zoomFactor);

    //AtlasRegion* region = &player->skel.slots[0].regionAttachment.region_image;
    //Array<v2f, 2> uvs2 = {v2f{region->u, region->v}, v2f{region->u2, region->v2}};
    //PushTexture(global_renderingInfo, region->page->rendererObject, player->height, player->world.rotation, player->world.pos, player->world.scale, uvs2);
};


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
#include "runtime_array.h"
#include "debug_array.h"
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
global_variable f32 currentRotation;
global_variable NormalMap normalMap;

global_variable b firstTimeThrough{true};
global_variable v2i currentMousePos{};
global_variable v2i currentVectorToDraw{};
global_variable v2i originalMousePos{};

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

Quadf ParentTransform(Quadf localCoords, Transform transformInfo_world)
{
    Coordinate_Space parentSpace {};
    parentSpace.origin = transformInfo_world.translation;
    parentSpace.xBasis = v2f { CosR(transformInfo_world.rotation), SinR(transformInfo_world.rotation) };
    parentSpace.yBasis = transformInfo_world.scale.y * PerpendicularOp(parentSpace.xBasis);
    parentSpace.xBasis *= transformInfo_world.scale.x;
    
    Quadf transformedCoords {};
    for (i32 vertIndex {}; vertIndex < transformedCoords.vertices.Size(); ++vertIndex)
    {
        //This equation rotates first then moves to correct parent position
        transformedCoords.vertices[vertIndex] = parentSpace.origin + (localCoords.vertices[vertIndex].x * parentSpace.xBasis) + (localCoords.vertices[vertIndex].y * parentSpace.yBasis);
    };
    
    return transformedCoords;
};

extern "C" void GameUpdate(Application_Memory* gameMemory, Platform_Services* platformServices, Rendering_Info* renderingInfo, Game_Sound_Output_Buffer* soundOutput, Game_Input* gameInput)
{
    auto TranslateCurrentMeasurementsToGameUnits = [](Skeleton&& skel) {
        f32 pixelsPerMeter { global_renderingInfo->_pixelsPerMeter };
        
        skel.width /= pixelsPerMeter;
        skel.height /= pixelsPerMeter;
        
        for (i32 boneIndex {}; boneIndex < skel.bones.length; ++boneIndex)
        {
            skel.bones[boneIndex].parentBoneSpace.translation.x /= pixelsPerMeter;
            skel.bones[boneIndex].parentBoneSpace.translation.y /= pixelsPerMeter;
            skel.bones[boneIndex].initialPos_parentBoneSpace.x /= pixelsPerMeter;
            skel.bones[boneIndex].initialPos_parentBoneSpace.y /= pixelsPerMeter;
            
            skel.bones[boneIndex].parentBoneSpace.rotation = Radians(skel.bones[boneIndex].parentBoneSpace.rotation);
            skel.bones[boneIndex].initialRotation_parentBoneSpace = Radians(skel.bones[boneIndex].initialRotation_parentBoneSpace);
            
            skel.bones[boneIndex].length /= pixelsPerMeter;
        };
        
        for (i32 slotI {}; slotI < skel.slots.length; ++slotI)
        {
            skel.slots[slotI].regionAttachment.height /= pixelsPerMeter;
            skel.slots[slotI].regionAttachment.width /= pixelsPerMeter;
            skel.slots[slotI].regionAttachment.parentBoneSpace.rotation = Radians(skel.slots[slotI].regionAttachment.parentBoneSpace.rotation);
            skel.slots[slotI].regionAttachment.parentBoneSpace.translation.x /= pixelsPerMeter;
            skel.slots[slotI].regionAttachment.parentBoneSpace.translation.y /= pixelsPerMeter;
        };
        
#if 0
        for (i32 animIndex {}; animIndex < animData.animMap.animations.length; ++animIndex)
        {
            Animation* anim = &animData.animMap.animations[animIndex];
            
            if (anim->name)
            {
                for (i32 boneIndex {}; boneIndex < anim->bones.Size(); ++boneIndex)
                {
                    TranslationTimeline* boneTranslationTimeline = &anim->boneTranslationTimelines[boneIndex];
                    for (i32 keyFrameIndex {}; keyFrameIndex < boneTranslationTimeline->translations.Size(); ++keyFrameIndex)
                    {
                        boneTranslationTimeline->translations[keyFrameIndex].x /= pixelsPerMeter;
                        boneTranslationTimeline->translations[keyFrameIndex].y /= pixelsPerMeter;
                    }
                    
                    RotationTimeline* boneRotationTimeline = &anim->boneRotationTimelines[boneIndex];
                    for (i32 keyFrameIndex {}; keyFrameIndex < boneRotationTimeline->angles.Size(); ++keyFrameIndex)
                    {
                        boneRotationTimeline->angles[keyFrameIndex] = Radians(boneRotationTimeline->angles[keyFrameIndex]);
                    }
                };
                
                for (i32 hitBoxIndex {}; hitBoxIndex < anim->hitBoxes.length; ++hitBoxIndex)
                {
                    anim->hitBoxes[hitBoxIndex].size.width /= pixelsPerMeter;
                    anim->hitBoxes[hitBoxIndex].size.height /= pixelsPerMeter;
                    anim->hitBoxes[hitBoxIndex].worldPosOffset.x /= pixelsPerMeter;
                    anim->hitBoxes[hitBoxIndex].worldPosOffset.y /= pixelsPerMeter;
                };
            };
        }
#endif
    };
    
    BGZ_ERRCTXT1("When entering GameUpdate");
    
    const Game_Controller* keyboard = &gameInput->Controllers[0];
    const Game_Controller* gamePad = &gameInput->Controllers[1];
    
    Game_State* gState = (Game_State*)gameMemory->permanentStorage;
    
    //Setup globals
    deltaT = platformServices->prevFrameTimeInSecs;
    deltaTFixed = platformServices->targetFrameTimeInSecs;
    globalPlatformServices = platformServices;
    global_renderingInfo = renderingInfo;
    
    Stage_Data* stage = &gState->stage;
    Fighter* player = &stage->player;
    Fighter* enemy = &stage->enemy;
    Fighter* enemy2 = &stage->enemy2;
    
    Memory_Partition* framePart = GetMemoryPartition(gameMemory, "frame");
    Memory_Partition* levelPart = GetMemoryPartition(gameMemory, "level");
    
    if (NOT gameMemory->initialized)
    {
        BGZ_ERRCTXT1("When Initializing game memory and game state");
        
        gameMemory->initialized = true;
        
        *gState = {}; //Make sure everything gets properly defaulted/Initialized (constructors are called that need to be)
        
        //Stage Init
        stage->backgroundImg = LoadBitmap_BGRA("data/4k.jpg");
        stage->size.height = 10.0f;
        stage->size.width = 17.7777f;
        stage->centerPoint = { stage->size.width / 2, (f32)stage->size.height / 2 };
        
        //Camera Init
        stage->camera.dilatePointOffset_normalized = { 0.0f, -0.3f };
        stage->camera.lookAt = stage->centerPoint;
        stage->camera.zoomFactor = 1.0f;
        
        //Read in data
        Atlas* atlas = CreateAtlasFromFile("data/yellow_god.atlas");
        InitSkel($(player->skel), $(*levelPart), atlas, "data/yellow_god.json"); //TODO: In order to reduce the amount of time reading from json file think about how to implement one common skeleton/animdata file(s)
        TranslateCurrentMeasurementsToGameUnits($(player->skel)); //Translate pixels to meters and degrees to radians (since spine exports everything in pixel/degree units)
        
        gState->currentImageRegion = GetImageRegion(player->skel, "right-forearm");
        gState->normalMap = LoadBitmap_BGRA("data/yellow_god_normal_map.png");
        gState->lightAngle = 1.0f;
        gState->lightThreshold = 1.0f;
    };
    
    if (globalPlatformServices->DLLJustReloaded)
    {
        BGZ_CONSOLE("Dll reloaded!");
        globalPlatformServices->DLLJustReloaded = false;
        
        normalMap.mapData = gState->normalMap.data;
    };
    
    if (KeyHeld(keyboard->MoveRight))
    {
        gState->normalMapRotation += .01f;
    };
    
    if (KeyHeld(keyboard->MoveLeft))
    {
        gState->normalMapRotation -= .01f;
    };
    
    if (KeyHeld(keyboard->MoveUp))
        stage->camera.zoomFactor += .02f;
    
    if (KeyHeld(keyboard->MoveDown))
        stage->camera.zoomFactor -= .02f;
    
    if(KeyHeld(gameInput->mouseButtons[Mouse::LEFT_CLICK]))
    {
        if(firstTimeThrough)
        {
            originalMousePos = {gameInput->mouseX, gameInput->mouseY};
            firstTimeThrough = false;
        };
        
        currentMousePos = {gameInput->mouseX, gameInput->mouseY};
        currentVectorToDraw = currentMousePos - originalMousePos;
        
        BGZ_CONSOLE("vecX: %i, vecY: %i\n", currentVectorToDraw.x, currentVectorToDraw.y);
    };
    
    if(KeyReleased(gameInput->mouseButtons[Mouse::LEFT_CLICK]))
    {
        firstTimeThrough = true;
    };
    
    UpdateCamera(global_renderingInfo, stage->camera.lookAt, stage->camera.zoomFactor, stage->camera.dilatePointOffset_normalized);
    
    normalMap.rotation = currentRotation;
    
    { //Render
        auto DrawImage = [gState, stage](Region_Attachment* region) -> void {
            AtlasRegion* region_image = &region->region_image;
            Array<v2f, 2> uvs2 = { v2f { region_image->u, region_image->v }, v2f { region_image->u2, region_image->v2 } };
            
            Transform transform { { stage->size.width / 2.0f, 3.5f } /*Translation*/, gState->normalMapRotation, { 2.0f, 2.0f } /*Scale*/ };
            Quadf region_localCoords = ProduceQuadFromCenterPoint({ 0.0f, 0.0f }, region->width, region->height);
            Quadf region_worldSpaceCoords = ParentTransform(region_localCoords, transform);
            
            NormalMap normalMap {};
            normalMap.mapData = gState->normalMap.data;
            normalMap.rotation = gState->normalMapRotation;
            normalMap.scale = transform.scale;
            normalMap.lightAngle = gState->lightAngle;
            normalMap.lightThreshold = gState->lightThreshold;
            
            PushTexture(global_renderingInfo, region_worldSpaceCoords, region_image->page->rendererObject, normalMap, v2f { region->width, region->height }, uvs2, region_image->name);
        };
        
        //Push background
        Quadf targetRect_worldCoords = ProduceQuadFromCenterPoint(stage->centerPoint, stage->size.width, stage->size.height);
        PushRect(global_renderingInfo, targetRect_worldCoords, { 1.0f, 0.0f, 0.0f });
        
        v2f mousePos_meters { ((f32)gameInput->mouseX) / global_renderingInfo->_pixelsPerMeter, ((f32)gameInput->mouseY) / global_renderingInfo->_pixelsPerMeter };
        
        if (gameInput->mouseButtons[Mouse::LEFT_CLICK].Pressed)
        {
            Quadf targetRect_mousePos = ProduceQuadFromCenterPoint(mousePos_meters, 1.0f, 1.0f);
            PushRect(global_renderingInfo, targetRect_mousePos, { 0.0f, 1.0f, 0.0f });
        }
        
        DrawImage(&gState->currentImageRegion);
    };
    
    IsAllTempMemoryCleared(framePart);
    IsAllTempMemoryCleared(levelPart);
    Release($(*framePart));
    
    if (gState->isLevelOver)
        Release($(*levelPart));
};

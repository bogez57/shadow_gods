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

global_variable s32 heap;

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
global_variable s32 renderBuffer;

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
Image CreateEmptyImage(s32 width, s32 height)
{
    Image image{};
    
    s32 numBytesPerPixel{4};
    image.data = (u8*)MallocSize(heap, width*height*numBytesPerPixel);
    image.size = v2i{width, height};
    image.pitch = width*numBytesPerPixel;
    
    return image;
};

local_func
void GenerateSphereNormalMap(Image&& sourceImage)
{
    f32 invWidth = 1.0f / (f32)(sourceImage.size.width - 1);
    f32 invHeight = 1.0f / (f32)(sourceImage.size.height - 1);
    
    u8* row = (u8*)sourceImage.data;
    for(s32 y = 0; y < sourceImage.size.height; ++y)
    {
        u32* pixel = (u32*)row;
        
        for(s32 x = 0; x < sourceImage.size.width; ++x)
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
            
            *pixel++ = (((u8)(color.a + .5f) << 24) |
                        ((u8)(color.r + .5f) << 16) |
                        ((u8)(color.g + .5f) << 8) |
                        ((u8)(color.b + .5f) << 0));
        };
        
        row += sourceImage.pitch;
    };
};

local_func
Image FlipImage(Image image)
{
    s32 widthInBytes = image.size.width * 4;
    u8* p_topRowOfTexels = nullptr;
    u8* p_bottomRowOfTexels = nullptr;
    u8 temp = 0;
    s32 halfHeight = image.size.height / 2;
    
    for (s32 row = 0; row < halfHeight; ++row)
    {
        p_topRowOfTexels = image.data + row * widthInBytes;
        p_bottomRowOfTexels = image.data + (image.size.height - row - 1) * widthInBytes;
        
        for (s32 col = 0; col < widthInBytes; ++col)
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

inline bool KeyPressed(Button_State KeyState)
{
    if (KeyState.Pressed && KeyState.NumTransitionsPerFrame)
    {
        return true;
    };
    
    return false;
};

inline bool KeyComboPressed(Button_State KeyState1, Button_State KeyState2)
{
    if (KeyState1.Pressed && KeyState2.Pressed && (KeyState1.NumTransitionsPerFrame || KeyState2.NumTransitionsPerFrame))
    {
        return true;
    };
    
    return false;
};

inline bool KeyHeld(Button_State KeyState)
{
    if (KeyState.Pressed && (KeyState.NumTransitionsPerFrame == 0))
    {
        return true;
    };
    
    return false;
};

inline bool KeyComboHeld(Button_State KeyState1, Button_State KeyState2)
{
    if (KeyState1.Pressed && KeyState2.Pressed && (KeyState1.NumTransitionsPerFrame == 0 && KeyState2.NumTransitionsPerFrame == 0))
    {
        return true;
    };
    
    return false;
};

inline bool KeyReleased(Button_State KeyState)
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
    for (s32 vertIndex {}; vertIndex < transformedCoords.vertices.Size(); ++vertIndex)
    {
        //This equation rotates first then moves to correct parent position
        transformedCoords.vertices[vertIndex] = parentSpace.origin + (localCoords.vertices[vertIndex].x * parentSpace.xBasis) + (localCoords.vertices[vertIndex].y * parentSpace.yBasis);
    };
    
    return transformedCoords;
};

extern "C" void GameUpdate(Application_Memory* gameMemory, Platform_Services* platformServices, Rendering_Info* renderingInfo, Game_Sound_Output_Buffer* soundOutput, Game_Input* gameInput)
{
    auto TranslateCurrentMeasurementsToGameUnits = [](Skeleton&& skel, AnimationData&& animData) {
        f32 pixelsPerMeter { global_renderingInfo->_pixelsPerMeter };
        
        skel.width /= pixelsPerMeter;
        skel.height /= pixelsPerMeter;
        
        for (s32 boneIndex {}; boneIndex < skel.bones.length; ++boneIndex)
        {
            skel.bones[boneIndex].parentBoneSpace.translation.x /= pixelsPerMeter;
            skel.bones[boneIndex].parentBoneSpace.translation.y /= pixelsPerMeter;
            skel.bones[boneIndex].initialPos_parentBoneSpace.x /= pixelsPerMeter;
            skel.bones[boneIndex].initialPos_parentBoneSpace.y /= pixelsPerMeter;
            
            skel.bones[boneIndex].parentBoneSpace.rotation = Radians(skel.bones[boneIndex].parentBoneSpace.rotation);
            skel.bones[boneIndex].initialRotation_parentBoneSpace = Radians(skel.bones[boneIndex].initialRotation_parentBoneSpace);
            
            skel.bones[boneIndex].length /= pixelsPerMeter;
        };
        
        for (s32 slotI {}; slotI < skel.slots.length; ++slotI)
        {
            skel.slots[slotI].regionAttachment.height /= pixelsPerMeter;
            skel.slots[slotI].regionAttachment.width /= pixelsPerMeter;
            skel.slots[slotI].regionAttachment.parentBoneSpace.rotation = Radians(skel.slots[slotI].regionAttachment.parentBoneSpace.rotation);
            skel.slots[slotI].regionAttachment.parentBoneSpace.translation.x /= pixelsPerMeter;
            skel.slots[slotI].regionAttachment.parentBoneSpace.translation.y /= pixelsPerMeter;
        };
        
        for (s32 animIndex {}; animIndex < animData.animMap.animations.length; ++animIndex)
        {
            Animation* anim = &animData.animMap.animations[animIndex];
            
            if (anim->name)
            {
                for (s32 boneIndex {}; boneIndex < anim->bones.Size(); ++boneIndex)
                {
                    TranslationTimeline* boneTranslationTimeline = &anim->boneTranslationTimelines[boneIndex];
                    for (s32 keyFrameIndex {}; keyFrameIndex < boneTranslationTimeline->translations.Size(); ++keyFrameIndex)
                    {
                        boneTranslationTimeline->translations[keyFrameIndex].x /= pixelsPerMeter;
                        boneTranslationTimeline->translations[keyFrameIndex].y /= pixelsPerMeter;
                    }
                    
                    RotationTimeline* boneRotationTimeline = &anim->boneRotationTimelines[boneIndex];
                    for (s32 keyFrameIndex {}; keyFrameIndex < boneRotationTimeline->angles.Size(); ++keyFrameIndex)
                    {
                        boneRotationTimeline->angles[keyFrameIndex] = Radians(boneRotationTimeline->angles[keyFrameIndex]);
                    }
                };
                
                for (s32 hitBoxIndex {}; hitBoxIndex < anim->hitBoxes.length; ++hitBoxIndex)
                {
                    anim->hitBoxes[hitBoxIndex].size.width /= pixelsPerMeter;
                    anim->hitBoxes[hitBoxIndex].size.height /= pixelsPerMeter;
                    anim->hitBoxes[hitBoxIndex].worldPosOffset.x /= pixelsPerMeter;
                    anim->hitBoxes[hitBoxIndex].worldPosOffset.y /= pixelsPerMeter;
                };
            };
        }
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
        stage->size.height = 40.0f;
        stage->size.width = WidthInMeters(stage->backgroundImg, stage->size.height);
        stage->centerPoint = { (f32)WidthInMeters(stage->backgroundImg, stage->size.height) / 2, (f32)stage->size.height / 2 };
        
        //Camera Init
        stage->camera.dilatePointOffset_normalized = { 0.0f, -0.3f };
        stage->camera.lookAt = { stage->size.width / 2.0f, 10.0f };
        stage->camera.zoomFactor = .4f;
        
        //gState->openGLRenderTest = LoadBitmap_BGRA("data/left-bicep.png");
    };
    
    UpdateCamera(global_renderingInfo, stage->camera.lookAt, stage->camera.zoomFactor, stage->camera.dilatePointOffset_normalized);
    
    { //Render
        Transform rectTransform { {stage->centerPoint}, 2.0f, {1.0f, 1.0f} };
        Quadf targetRect_localCoords = ProduceQuadFromCenterPoint({0.0f, 0.0f}, 2.0f, 3.0f);
        Quadf targetRect_worldCoords = ParentTransform(targetRect_localCoords, rectTransform);
        PushRect(global_renderingInfo, targetRect_worldCoords, { 1.0f, 0.0f, 0.0f });
        
#if 0
        Quadf textureTargetRect_localCoords = ProduceQuadFromCenterPoint({0.0f, 0.0f}, gState->openGLRenderTest.aspectRatio * BitmapHeight_Meters(*global_renderingInfo, gState->openGLRenderTest), BitmapHeight_Meters(*global_renderingInfo, gState->openGLRenderTest));
        Quadf textureTargetRect_worldCoords = ParentTransform(textureTargetRect_localCoords, rectTransform);
        Array<v2f, 2> uvs = { v2f { 0.0f, 0.0f }, v2f { 1.0f, 1.0f } };
        PushTexture(global_renderingInfo, textureTargetRect_worldCoords, $(gState->openGLRenderTest), BitmapHeight_Meters(*global_renderingInfo, gState->openGLRenderTest), uvs, "left-bicep-image");
#endif
    };
    
    IsAllTempMemoryCleared(framePart);
    IsAllTempMemoryCleared(levelPart);
    Release($(*framePart));
    
    if (gState->isLevelOver)
        Release($(*levelPart));
};

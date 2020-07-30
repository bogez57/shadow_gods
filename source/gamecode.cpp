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

#include <boagz/error_handling.h>
#include <stb/stb_image.h>
#include "debug/debug.h"

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
#define OBJ_PARSE_IMPLEMENTATION
#include "fluery_obj_parser.h"

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
            v2 normalUV = {invWidth*(f32)x, invHeight*(f32)y};
            
            f32 normalX = 2.0f*normalUV.x - 1.0f;
            f32 normalY = 2.0f*normalUV.y - 1.0f;
            
            f32 rootTerm = 1.0f - normalX*normalX - normalY*normalY;
            f32 normalZ = 0.0f;
            
            v3 normal {0.0f, 0.0f, 1.0f};
            
            if(rootTerm >= 0.0f)
            {
                normalZ = Sqrt(rootTerm);
                normal = v3{normalX, normalY, normalZ};
            };
            
            //Convert from -1 to 1 range to value between 0 and 255
            v4 color = {255.0f*(.5f*(normal.x + 1.0f)),
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

void TestFunc()
{
    TIMED_SCOPE();
    
    int x = 3;
    x = 34;
    x += 4444;
};

void TestFunc2()
{
    for (int i {}; i < 5; ++i)
        TestFunc();
}

Quadf ParentTransform(Quadf localCoords, Transform transformInfo_world)
{
    Coordinate_Space parentSpace {};
    parentSpace.origin = transformInfo_world.translation;
    parentSpace.xBasis = v2 { CosR(transformInfo_world.rotation), SinR(transformInfo_world.rotation) };
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

extern "C" void GameUpdate(MemoryBlock* gameMemory, MemoryBlock* debugMemory, Platform_Services* platformServices, Rendering_Info* renderingInfo, Game_Sound_Output_Buffer* soundOutput, Game_Input* gameInput)
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
            
            skel.bones[boneIndex].parentBoneSpace.rotation = ToRadians(skel.bones[boneIndex].parentBoneSpace.rotation);
            skel.bones[boneIndex].initialRotation_parentBoneSpace = ToRadians(skel.bones[boneIndex].initialRotation_parentBoneSpace);
            
            skel.bones[boneIndex].length /= pixelsPerMeter;
        };
        
        for (s32 slotI {}; slotI < skel.slots.length; ++slotI)
        {
            skel.slots[slotI].regionAttachment.height /= pixelsPerMeter;
            skel.slots[slotI].regionAttachment.width /= pixelsPerMeter;
            skel.slots[slotI].regionAttachment.parentBoneSpace.rotation = ToRadians(skel.slots[slotI].regionAttachment.parentBoneSpace.rotation);
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
                        boneRotationTimeline->angles[keyFrameIndex] = ToRadians(boneRotationTimeline->angles[keyFrameIndex]);
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
    
    const Game_Controller* keyboard = &gameInput->Controllers[0];
    const Game_Controller* gamePad = &gameInput->Controllers[1];
    
    Game_State* gState = (Game_State*)gameMemory->permanentStorage;
    
    //Setup globals
    deltaT = platformServices->prevFrameTimeInSecs;
    deltaTFixed = platformServices->targetFrameTimeInSecs;
    globalPlatformServices = platformServices;
    global_renderingInfo = renderingInfo;
    
    Stage_Data* stage = &gState->stage;
    Fighter3D* fighter0 = &gState->fighter0;
    Fighter3D* fighter1 = &gState->fighter1;
    Camera3D* camera3d = &gState->camera3d;
    Camera2D* camera2d = &gState->camera2d;
    
    Memory_Partition* framePart = GetMemoryPartition(gameMemory, "frame");
    Memory_Partition* levelPart = GetMemoryPartition(gameMemory, "level");
    
    //TODO: Get software renderer running again so I can draw rectangles again
    if (NOT gameMemory->initialized)
    {
        TestFunc();
        
        gameMemory->initialized = true;
        
        *gState = {}; //Make sure everything gets properly defaulted/Initialized (constructors are called that need to be)
        
        InitRenderer(global_renderingInfo, 60.0f/*fov*/, 16.0f/9.0f/*aspect*/, .1f/*nearPlane*/, 100.0f/*farPlane*/);
        
        //Stage Init
        stage->backgroundImg = LoadBitmap_BGRA("data/4k.jpg");
        stage->size.height = 40.0f;
        stage->size.width = WidthInMeters(stage->backgroundImg, stage->size.height);
        stage->centerPoint = { (f32)WidthInMeters(stage->backgroundImg, stage->size.height) / 2, (f32)stage->size.height / 2 };
        
#if 0
        //Camera Init
        camera3d->worldPos = {0.0f, 0.0f, -7.0f};
        camera3d->rotation = {0.0f, 0.0f, 0.0f};
#endif
        
        f32 heightOfScreen_meters = 720.0f / global_renderingInfo->_pixelsPerMeter;
        f32 widthMeters = (heightOfScreen_meters * stage->backgroundImg.aspectRatio)/2.0f;
        camera2d->lookAt = v2{widthMeters, heightOfScreen_meters/2.0f};
        camera2d->dilatePointOffset_normalized = {0.0f, -0.3f};
        camera2d->zoomFactor = 1.0f;
        
#if 0
        InitArr($(fighter0->mesh.vertAttribs), levelPart, 100);
        
        fighter0->mesh.vertAttribs.Push(-.5f * 2.0f); fighter0->mesh.vertAttribs.Push(-.5f * 2.0f); fighter0->mesh.vertAttribs.Push(0.0f * 2.0f);//0
        fighter0->mesh.vertAttribs.Push(0.0f); fighter0->mesh.vertAttribs.Push(0.0f);                        //tex coord
        fighter0->mesh.vertAttribs.Push(1.0f); fighter0->mesh.vertAttribs.Push(0.0f); fighter0->mesh.vertAttribs.Push(1.0f);//normal
        
        fighter0->mesh.vertAttribs.Push(+.5f * 2.0f); fighter0->mesh.vertAttribs.Push(+.5f * 2.0f); fighter0->mesh.vertAttribs.Push(0.0f * 2.0f);//1
        fighter0->mesh.vertAttribs.Push(1.0f); fighter0->mesh.vertAttribs.Push(1.0f);                        //tex Coord
        fighter0->mesh.vertAttribs.Push(1.0f); fighter0->mesh.vertAttribs.Push(0.0f); fighter0->mesh.vertAttribs.Push(0.0f);//color
        
        fighter0->mesh.vertAttribs.Push(-.5f * 2.0f); fighter0->mesh.vertAttribs.Push(+.5f * 2.0f); fighter0->mesh.vertAttribs.Push(0.0f * 2.0f);//2
        fighter0->mesh.vertAttribs.Push(0.0f); fighter0->mesh.vertAttribs.Push(1.0f);                       //tex Coord
        fighter0->mesh.vertAttribs.Push(1.0f); fighter0->mesh.vertAttribs.Push(0.0f); fighter0->mesh.vertAttribs.Push(0.0f);//color
        
        fighter0->mesh.vertAttribs.Push(-.5f * 2.0f); fighter0->mesh.vertAttribs.Push(-.5f * 2.0f); fighter0->mesh.vertAttribs.Push(0.0f * 2.0f);//0
        fighter0->mesh.vertAttribs.Push(0.0f); fighter0->mesh.vertAttribs.Push(0.0f);                        //tex coord
        fighter0->mesh.vertAttribs.Push(1.0f); fighter0->mesh.vertAttribs.Push(0.0f); fighter0->mesh.vertAttribs.Push(1.0f);//normal
        
        fighter0->mesh.vertAttribs.Push(+.5f * 2.0f); fighter0->mesh.vertAttribs.Push(-.5f * 2.0f); fighter0->mesh.vertAttribs.Push(0.0f * 2.0f);//3
        fighter0->mesh.vertAttribs.Push(+1.0f); fighter0->mesh.vertAttribs.Push(0.0f);                      //tex Coord
        fighter0->mesh.vertAttribs.Push(1.0f); fighter0->mesh.vertAttribs.Push(0.0f); fighter0->mesh.vertAttribs.Push(0.0f);//color
        
        fighter0->mesh.vertAttribs.Push(+.5f * 2.0f); fighter0->mesh.vertAttribs.Push(+.5f * 2.0f); fighter0->mesh.vertAttribs.Push(0.0f * 2.0f);//1
        fighter0->mesh.vertAttribs.Push(1.0f); fighter0->mesh.vertAttribs.Push(1.0f);                        //tex Coord
        fighter0->mesh.vertAttribs.Push(1.0f); fighter0->mesh.vertAttribs.Push(0.0f); fighter0->mesh.vertAttribs.Push(0.0f);//color
        
        InitArr($(fighter0->mesh.indicies), levelPart, 6);
        fighter0->mesh.indicies.Push(0);
        fighter0->mesh.indicies.Push(1);
        fighter0->mesh.indicies.Push(2);
        fighter0->mesh.indicies.Push(3);
        fighter0->mesh.indicies.Push(4);
        fighter0->mesh.indicies.Push(5);
        
        fighter0->id = InitVertexBuffer(global_renderingInfo, fighter0->mesh.vertAttribs, fighter0->mesh.indicies);
        
        Image fighterTexture = LoadBitmap_BGRA("data/1080p.jpg");
        fighter0->textureID = LoadTexture(global_renderingInfo, fighterTexture);
        
        fighter0->worldTransform.translation = {+1.0f, 0.0f, 0.0f};
#endif
    };
    
    if(KeyHeld(keyboard->MoveRight))
    {
        //fighter0->worldTransform.translation.x += 0.1f;
    };
    
    //UpdateCamera3D(global_renderingInfo, camera3d->worldPos, camera3d->rotation);
    UpdateCamera2D(global_renderingInfo, camera2d->lookAt,  camera2d->zoomFactor);
    
    TestFunc2();
    
    { //Render
        //Push background
        
#if 1
        Array<v2, 2> uvs = { v2 { 0.0f, 0.0f }, v2 { 1.0f, 1.0f } };
        Quadf targetRect_worldCoords = ProduceQuadFromCenterPoint(stage->centerPoint, stage->size.width, stage->size.height);
        PushRect(global_renderingInfo, targetRect_worldCoords, v3{0.0f, 1.0f, 0.4f});
#endif
        
        {//Print out previous frame debug data
            if(debugMemory->initialized)
            {
                DebugState* debugState = (DebugState*)debugMemory->permanentStorage;
                
                for(int i{}; i < debugState->numStoredDebugEvents; ++i)
                {
                    StoredDebugEvent* debugEvent = &debugState->allStoredDebugEvents[i];
                    
                    printf("fileName: %s\n", debugEvent->fileName);
                    printf("functionName: %s\n", debugEvent->functionName);
                    printf("line number: %i\n", debugEvent->lineNumber);
                    printf("hit count: %llu\n", (unsigned long long)debugEvent->timeStamps[debugEvent->timeStampCount - 1].hitCount);
                    printf("cylces to complete: %llu\n\n", (unsigned long long)debugEvent->timeStamps[debugEvent->timeStampCount - 1].cycleCount);
                };
            }
        };
        
#if 0
        //World Transform
        Mat4x4 fighter0_worldTransformMatrix = ProduceWorldTransformMatrix(fighter0->worldTransform.translation, fighter0->worldTransform.rotation, fighter0->worldTransform.scale);
        Mat4x4 fighter1_worldTransformMatrix = ProduceWorldTransformMatrix(fighter1->worldTransform.translation, fighter1->worldTransform.rotation, fighter1->worldTransform.scale);
        PushGeometry(global_renderingInfo, fighter0->id, fighter0->textureID, fighter0->mesh.indicies, fighter0_worldTransformMatrix);
#endif
        
        IsAllTempMemoryCleared(framePart);
        IsAllTempMemoryCleared(levelPart);
        Release($(*framePart));
        
        if (gState->isLevelOver)
            Release($(*levelPart));
    };
};

extern "C" void DebugFrameEnd(MemoryBlock* debugMemory)
{
    //Store frame debug information
    DebugState* debugState = (DebugState*)debugMemory->permanentStorage;
    
    if(NOT debugMemory->initialized)
    {
        debugMemory->initialized = true;
        InitDebugState(debugState);
    };
    
    UpdateDebugState(debugState);
    EndOfFrame_ResetTimingInfo();
};

#if 0
//Render previous frame data
if(debugMemory->initialized)
{
    struct FrameBar
    {
        f32 width{.4f};
        f32 startingHeight{1.2f};
        f32 currentHeight{1.2f};
        v2 worldPos{.4f, .4f};
    };
    
    DebugState* debugState = (DebugState*)debugMemory->permanentStorage;
    
    //Just take aggregate of all scope times and display as rect for now
    u64 totalFrameCycleCount{};
    for(int i{}; i < debugState->numStoredDebugEvents; ++i)
    {
        StoredDebugEvent* debugEvent = &debugState->allStoredDebugEvents[i];
        
        printf("fileName: %s\n", debugEvent->fileName);
        printf("functionName: %s\n", debugEvent->functionName);
        printf("line number: %i\n", debugEvent->lineNumber);
        printf("hit count: %llu\n", (unsigned long long)debugEvent->timeStamps[debugEvent->timeStampCount - 1].hitCount);
        printf("cylces to complete: %llu\n\n", (unsigned long long)debugEvent->timeStamps[debugEvent->timeStampCount - 1].cycleCount);
        
        totalFrameCycleCount += debugEvent->timeStamps[debugEvent->timeStampCount - 1].cycleCount;
    };
    BGZ_CONSOLE("Total cycles: %llu ----------------- \n\n", totalFrameCycleCount);
    f32 frameCycleCountNormalized = (f32)((double)totalFrameCycleCount / 2000000.0);
    
    local_persist FrameBar frameBars[20];
    local_persist s32 frameBarIndex{};
    local_persist bool frameBarBufInit{false};
    
    if(NOT frameBarBufInit)
    {
        f32 frameBar_perFrameMoveDistance{frameBars[0].width + .3f};
        frameBarBufInit = true;
        
        for(s32 i{}; i < 20; ++i)
        {
            frameBars[i].worldPos.x += (frameBar_perFrameMoveDistance * i);
        };
    };
    
    frameBars[frameBarIndex].currentHeight = frameBars[frameBarIndex].startingHeight * frameCycleCountNormalized;
    
    for(s32 i{}; i < 20; ++i)
    {
        Quadf frameBar_newWorldPos = _ProduceQuadFromBottomLeftPoint(frameBars[i].worldPos, frameBars[i].width, frameBars[i].currentHeight);
        PushRect(global_renderingInfo, frameBar_newWorldPos, v3{1.0f, 0.0f, 0.0f});
    };
    
    if(frameBarIndex < 20)
        ++frameBarIndex;
    else
        frameBarIndex = 0;
};
#endif
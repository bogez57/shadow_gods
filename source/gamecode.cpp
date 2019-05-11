/*
    TODO List:

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

#define ATOMIC_TYPES_IMPL
#include "atomic_types.h"
#include "memory_handling.h"
#include "array.h"
#include "dynamic_array.h"
#include "ring_buffer.h"
#include "linked_list.h"
#include <utility>

#include "atlas.h"
#include "shared.h"
#include "gamecode.h"
#include "math.h"
#include "utilities.h"

global_variable Platform_Services* globalPlatformServices;
global_variable Game_Render_Cmds globalRenderCmds;
global_variable f32 deltaT;
global_variable f32 deltaTFixed;
global_variable f32 viewportWidth;
global_variable f32 viewportHeight;
global_variable i32 heap;
global_variable i32 renderBuffer;

const i32 bytesPerPixel{4};

#define MEMORY_HANDLING_IMPL
#include "memory_handling.h"
#define DYNAMIC_ALLOCATOR_IMPL
#include "dynamic_allocator.h"
#define LINEAR_ALLOCATOR_IMPL
#include "linear_allocator.h"
#define COLLISION_IMPL
#include "collisions.h"
#define ATLAS_IMPL
#include "atlas.h"
#define JSON_IMPL
#include "json.h"
#define SKELETON_IMPL
#include "skeleton.h"
#define RENDERER_STUFF_IMPL
#include "renderer_stuff.h"

// Third Party
#include <boagz/error_context.cpp>

local_func auto FlipImage(Image image) -> Image
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

Quadf WorldTransform(Quadf localCoords, Transform transformInfo_world)
{
    //With world space origin at 0, 0
    Coordinate_Space imageSpace{};
    imageSpace.origin = transformInfo_world.pos;
    imageSpace.xBasis = v2f{CosR(transformInfo_world.rotation), SinR(transformInfo_world.rotation)};
    imageSpace.yBasis = transformInfo_world.scale.y * PerpendicularOp(imageSpace.xBasis);
    imageSpace.xBasis *= transformInfo_world.scale.x;

    Quadf transformedCoords{};
    for(i32 vertIndex{}; vertIndex < transformedCoords.vertices.Size(); ++vertIndex)
    {
        //This equation rotates first then moves to correct world position
        transformedCoords.vertices.At(vertIndex) = imageSpace.origin + (localCoords.vertices.At(vertIndex).x * imageSpace.xBasis) + (localCoords.vertices.At(vertIndex).y * imageSpace.yBasis);
    };

    return transformedCoords;
};

Quadf CameraTransform(Quadf worldCoords, Game_Camera camera)
{
    Quadf transformedCoords{};

    v2f translationToCameraSpace = camera.viewCenter - camera.lookAt;

    for(i32 vertIndex{}; vertIndex < 4; vertIndex++) 
    {
        worldCoords.vertices[vertIndex] += translationToCameraSpace;
    };

    transformedCoords = DilateAboutArbitraryPoint(camera.dilatePoint, camera.zoomFactor, worldCoords);

    return transformedCoords;
};

enum Render_Entry_Type
{
    EntryType_Image
};

struct RenderEntry_Header
{
    Render_Entry_Type type;
};

struct RenderEntry_Image
{
    RenderEntry_Header header;
    Transform world;
    Image normalMap;
    Image imageData;
};

void InitRenderBufferInfo(RenderBufferInfo&& renderBufInfo)
{
    renderBufInfo.baseAddress = PushType(renderBuffer, ui8, 1);
    Release(renderBuffer);//clear out ui8 type I just pushed to grab base address of render buffer allocator
    renderBufInfo.size = MemoryPartitionSize(renderBuffer);
};

void PushImage(RenderBufferInfo&& bufferInfo, Image imageToDraw, Image normalMap, Transform image_worldTransformInfo)
{
    RenderEntry_Image* imageEntry = PushType(renderBuffer, RenderEntry_Image, 1);

    imageEntry->header.type = EntryType_Image;
    imageEntry->world = image_worldTransformInfo;
    imageEntry->normalMap = normalMap;
    imageEntry->imageData = imageToDraw;

    ++bufferInfo.entryCount;
};

void Render(Game_State* gState, Image&& colorBuffer, RenderBufferInfo renderBufferInfo, Game_Camera camera)
{
    ui8* currentRenderBufferEntry = renderBufferInfo.baseAddress;
    for(i32 entryNumber = 0; entryNumber < renderBufferInfo.entryCount; ++entryNumber)
    {
        RenderEntry_Header* entryHeader = (RenderEntry_Header*)currentRenderBufferEntry;
        switch(entryHeader->type)
        {
            case EntryType_Image:
            {
                RenderEntry_Image* imageEntry = (RenderEntry_Image*)currentRenderBufferEntry;
                Quadf imageTargetRect = ProduceQuadFromBottomLeftPoint(v2f{0.0f, 0.0f}, (f32)imageEntry->imageData.size.width, (f32)imageEntry->imageData.size.height);

                ConvertToCorrectPositiveRadian($(imageEntry->world.rotation));

                Quadf imageTargetRect_world = WorldTransform(imageTargetRect, imageEntry->world);
                Quadf imageTargetRect_camera = CameraTransform(imageTargetRect_world, camera);

                DrawImageSlowly($(colorBuffer), imageTargetRect_camera, imageEntry->imageData, gState->lightAngle, gState->lightThreshold, imageEntry->normalMap, imageEntry->world.rotation, imageEntry->world.scale);

                currentRenderBufferEntry += sizeof(RenderEntry_Image);
            }break;

            InvalidDefaultCase;
        }
    };
};

extern "C" void GameUpdate(Application_Memory* gameMemory, Game_Offscreen_Buffer* gameBackBuffer, Platform_Services* platformServices, Game_Render_Cmds renderCmds, Game_Sound_Output_Buffer* soundOutput, Game_Input* gameInput)
{
    BGZ_ERRCTXT1("When entering GameUpdate");

    const Game_Controller* keyboard = &gameInput->Controllers[0];
    const Game_Controller* gamePad = &gameInput->Controllers[1];

    Game_State* gState = (Game_State*)gameMemory->PermanentStorage;

    //Setup globals
    deltaT = platformServices->prevFrameTimeInSecs;
    deltaTFixed = platformServices->targetFrameTimeInSecs;
    globalPlatformServices = platformServices;
    globalRenderCmds = renderCmds;

    Stage_Data* stage = &gState->stage;
    Fighter* player = &stage->player;
    Fighter* enemy = &stage->enemy;
    Fighter* enemy2 = &stage->enemy2;

    if (NOT gameMemory->Initialized)
    {
        BGZ_ERRCTXT1("When Initializing game memory and game state");

        gameMemory->Initialized = true;
        *gState = {}; //Make sure everything gets properly defaulted (constructors are called that need to be)

        gState->colorBuffer.data = (ui8*)gameBackBuffer->memory;
        gState->colorBuffer.size = v2i{gameBackBuffer->width, gameBackBuffer->height};
        gState->colorBuffer.pitch = gameBackBuffer->pitch;

        viewportWidth = 1280.0f;
        viewportHeight = 720.0f;

        {//Initialize memory/allocator stuff
            InitApplicationMemory(gameMemory);
            heap = CreatePartitionFromMemoryBlock(gameMemory, Megabytes(100), DYNAMIC);
            renderBuffer = CreatePartitionFromMemoryBlock(gameMemory, Megabytes(10), LINEAR);

            InitDynamAllocator(heap);
            InitLinearAllocator(renderBuffer);
        };

        //Init render buffer stuff
        InitRenderBufferInfo($(gState->renderBuffData));

        //Stage Init
        stage->info.backgroundImg.data = platformServices->LoadBGRAImage("data/1440p.jpg", $(stage->info.backgroundImg.size.width), $(stage->info.backgroundImg.size.height));
        stage->info.backgroundImg.pitch = stage->info.backgroundImg.size.width * bytesPerPixel;
        stage->info.size.x = (f32)stage->info.backgroundImg.size.x;
        stage->info.size.y = (f32)stage->info.backgroundImg.size.y;
        stage->info.centerPoint = { (f32)stage->info.size.width / 2, (f32)stage->info.size.height / 2 };

        //Camera Init
        stage->camera.viewWidth = viewportWidth;
        stage->camera.viewHeight = viewportHeight;
        stage->camera.lookAt = { stage->info.centerPoint.x, stage->info.centerPoint.y };
        stage->camera.viewCenter = { stage->camera.viewWidth / 2.0f, stage->camera.viewHeight / 2.0f };
        stage->camera.dilatePoint = stage->camera.viewCenter - v2f {0.0f, 200.0f};
        stage->camera.zoomFactor = 1.0f;

        //Player Init
        player->image.data = platformServices->LoadBGRAImage("data/left-bicep.png", $(player->image.size.width), $(player->image.size.height));
        player->image.pitch = player->image.size.width * bytesPerPixel;
        player->world.pos = {800.0f, 600.0f};
        player->world.rotation = 0.0f;
        player->world.scale = {1.0f, 1.0f};
        player->image.opacity = .5f;
        
        //Enemy Init
        enemy->image.data = platformServices->LoadBGRAImage("data/test_body_back.bmp", $(enemy->image.size.width), $(enemy->image.size.height));
        enemy->image.pitch = enemy->image.size.width * bytesPerPixel;
        enemy->world.pos = {200.0f, 150.0f};
        enemy->world.rotation = 0.0f;
        enemy->world.scale = {2.3f, 2.3f};
        enemy->image.opacity = .7f;

        gState->normalMap.data = platformServices->LoadBGRAImage("data/test.png", $(gState->normalMap.size.width), $(gState->normalMap.size.height));

        //Create empty image
        auto CreateEmptyImage = [](i32 width, i32 height) -> Image
                            {
                                Image image{};

                                image.data = (ui8*)MallocSize(heap, width*height*bytesPerPixel);
                                image.size = v2i{width, height};
                                image.pitch = width*bytesPerPixel;

                                return image;
                            };

        auto GenerateSphereNormalMap = [](Image&& sourceImage) -> void
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
                                    }

                                    row += sourceImage.pitch;
                                }
                            };
        
        gState->composite = CreateEmptyImage((i32)stage->info.size.x, (i32)stage->info.size.y);

        gState->lightThreshold = 1.0f;
        gState->lightAngle = 1.0f;

        //Render to Image
        v2f origin{0.0f, 0.0f};
        origin += 300.0f;
        origin.x += 100.0f;
    };

    if (globalPlatformServices->DLLJustReloaded)
    {
        BGZ_CONSOLE("Dll reloaded!");
        globalPlatformServices->DLLJustReloaded = false;
    };

    if(KeyHeld(keyboard->MoveRight))
    {
        stage->camera.lookAt += 2.0f;
    };

    if(KeyHeld(keyboard->MoveLeft))
    {
        player->world.pos += 10.0f;
    };

    if(KeyHeld(keyboard->MoveUp))
    {
        stage->camera.zoomFactor += .01f;
    };

    PushImage($(gState->renderBuffData), player->image, gState->normalMap, player->world);

    //Essentially local fighter coordinates
    Quadf playerTargetRect = ProduceQuadFromBottomLeftPoint(v2f{0.0f, 0.0f}, (f32)player->image.size.width, (f32)player->image.size.height);
    Quadf enemyTargetRect = ProduceQuadFromBottomLeftPoint(v2f{0.0f, 0.0f}, (f32)enemy->image.size.width, (f32)enemy->image.size.height);

    ConvertToCorrectPositiveRadian($(player->world.rotation));

    Render(gState, $(gState->colorBuffer), gState->renderBuffData, stage->camera);

    Release(renderBuffer);
    gState->renderBuffData.entryCount = 0;
};
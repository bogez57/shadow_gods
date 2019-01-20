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
#define MEMHANDLING_IMPL
#include "memory_handling.h"
#include "array.h"
#include "dynamic_array.h"
#include "ring_buffer.h"
#include "linked_list.h"

#include "atlas.h"
#include "shared.h"
#include "dynamic_allocator.h"
#include "gamecode.h"
#include "math.h"
#include "utilities.h"

global_variable Platform_Services* globalPlatformServices;
global_variable Game_Render_Cmds globalRenderCmds;
global_variable f32 deltaT;
global_variable f32 deltaTFixed;
global_variable f32 viewportWidth;
global_variable f32 viewportHeight;
global_variable Dynamic_Allocator dynamAllocator { 0 };

#define COLLISION_IMPL
#include "collisions.h"
#define ATLAS_IMPL
#include "atlas.h"
#define JSON_IMPL
#include "json.h"
#define SKELETON_IMPL
#include "skeleton.h"

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
        p_topRowOfTexels = image.Data + row * widthInBytes;
        p_bottomRowOfTexels = image.Data + (image.size.height - row - 1) * widthInBytes;

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

extern "C" void GameUpdate(Application_Memory* gameMemory, Platform_Services* platformServices, Game_Render_Cmds renderCmds, Game_Sound_Output_Buffer* soundOutput, Game_Input* gameInput)
{
    BGZ_ERRCTXT1("When entering GameUpdate");

    const Game_Controller* keyboard = &gameInput->Controllers[0];
    const Game_Controller* gamePad = &gameInput->Controllers[1];

    Game_State* gameState = (Game_State*)gameMemory->PermanentStorage;
    deltaT = platformServices->prevFrameTimeInSecs;
    globalPlatformServices = platformServices;
    globalRenderCmds = renderCmds;

    Stage_Data* stage = &gameState->stage;
    Fighter* player = &stage->player;

    if (NOT gameMemory->Initialized)
    {
        gameMemory->Initialized = true;

        BGZ_ERRCTXT1("When Initializing game memory and game state");

        InitApplicationMemory(gameMemory);
        CreateRegionFromMemory(gameMemory, Megabytes(500));

        viewportWidth = 1280.0f;
        viewportHeight = 720.0f;

        deltaTFixed = platformServices->targetFrameTimeInSecs;
        deltaT = platformServices->prevFrameTimeInSecs;

        { //Init stage
            *stage = {};

            stage->info.displayImage.Data = platformServices->LoadRGBAImage("data/4k.jpg", &stage->info.displayImage.size.width, &stage->info.displayImage.size.height);
            stage->info.displayImage = FlipImage(stage->info.displayImage); // Since opengl will read-in image upside down
            stage->info.currentTexture = renderCmds.LoadTexture(stage->info.displayImage); // TODO: Move out to renderer
            stage->info.size.width = (f32)stage->info.displayImage.size.width;
            stage->info.size.height = (f32)stage->info.displayImage.size.height;
            stage->info.centerPoint = { (f32)stage->info.size.width / 2, (f32)stage->info.size.height / 2 };

            stage->camera.viewWidth = viewportWidth;
            stage->camera.viewHeight = viewportHeight;
            stage->camera.lookAt = { stage->info.centerPoint.x, stage->info.centerPoint.y - 600.0f };
            stage->camera.viewCenter = { stage->camera.viewWidth / 2.0f, stage->camera.viewHeight / 2.0f };
            stage->camera.dilatePoint = stage->camera.viewCenter - v2f { 0.0f, 200.0f };
            stage->camera.zoomFactor = 1.0f;
        };

        { //Init player
            gameState->atlas = CreateAtlasFromFile("data/yellow_god.atlas", 0);

            *player = {};
            player->skel = CreateSkeletonUsingJsonFile(*gameState->atlas, "data/yellow_god.json");
            player->skel.worldPos = { stage->info.centerPoint.x, stage->info.centerPoint.y - 1000.0f };
            player->worldPos = &player->skel.worldPos;
        };
    };

    if (globalPlatformServices->DLLJustReloaded)
    {
        BGZ_CONSOLE("Dll reloaded!");

        { //Perform necessary operations to keep spine working with live code editing/input playback
            globalPlatformServices->DLLJustReloaded = false;
        };
    };

    if (KeyComboPressed(keyboard->ActionLeft, keyboard->MoveRight))
    {
    }

    if (KeyPressed(keyboard->ActionRight))
    {
    }

    if (KeyHeld(keyboard->MoveRight))
    {
        player->worldPos->x += 2.0f;
    }

    if (KeyHeld(keyboard->MoveLeft))
    {
        player->worldPos->x -= 2.0f;
    }

    { // Render
        renderCmds.Init();

        renderCmds.ClearScreen();

        { // Draw Level Background
            Coordinate_System backgroundWorldSpace {};
            Coordinate_System backgroundCameraSpace {};
            Drawable_Rect backgroundCanvas {};

            backgroundWorldSpace.Origin = { 0.0f, 0.0f };

            { // Transform to Camera Space
                v2f translationToCameraSpace = stage->camera.viewCenter - stage->camera.lookAt;
                backgroundCameraSpace.Origin = backgroundWorldSpace.Origin + translationToCameraSpace;
            };

            backgroundCanvas = ProduceRectFromBottomLeftPoint(backgroundCameraSpace.Origin, (f32)stage->info.size.width, (f32)stage->info.size.height);

            backgroundCanvas = DilateAboutArbitraryPoint(stage->camera.dilatePoint, stage->camera.zoomFactor, backgroundCanvas);

            renderCmds.DrawBackground(stage->info.currentTexture.ID, backgroundCanvas, v2f { 0.0f, 0.0f }, v2f { 1.0f, 1.0f });
        };

        { // Draw player
            //root bone
            player->skel.bones[0].worldPos = player->skel.worldPos;

            { //Update bone world positions
                player->skel.bones[1].worldPos = player->skel.bones[0].worldPos + player->skel.bones[1].parentLocalPos;
            };

            { //Translate to camera space position
                v2f translationToCameraSpace = stage->camera.viewCenter - stage->camera.lookAt;
                player->skel.bones[1].worldPos = player->skel.bones[1].worldPos + translationToCameraSpace;
            };

            Slot slot = player->skel.slots[1];

            v2f UVArray[4] = {
                v2f { slot.regionAttachment.imageInfo.u2, slot.regionAttachment.imageInfo.v2 },
                v2f { slot.regionAttachment.imageInfo.u, slot.regionAttachment.imageInfo.v2 },
                v2f { slot.regionAttachment.imageInfo.u, slot.regionAttachment.imageInfo.v },
                v2f { slot.regionAttachment.imageInfo.u2, slot.regionAttachment.imageInfo.v },
            };

            Drawable_Rect spineImage {
                v2f { player->skel.bones[1].worldPos.x, player->skel.bones[1].worldPos.y },
                v2f { player->skel.bones[1].worldPos.x + 100.0f, player->skel.bones[1].worldPos.y },
                v2f { player->skel.bones[1].worldPos.x + 100.0f, player->skel.bones[1].worldPos.y + 100.0f },
                v2f { player->skel.bones[1].worldPos.x, player->skel.bones[1].worldPos.y + 100.0f },
            };

            Texture* texture = (Texture*)slot.regionAttachment.imageInfo.page->rendererObject;

            globalRenderCmds.DrawTexture(texture->ID, spineImage, UVArray);
        };
    };
};
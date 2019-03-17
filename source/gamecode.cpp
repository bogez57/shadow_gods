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

local_func void
DrawRectangle(Game_Offscreen_Buffer* Buffer, Rectf rect, f32 r, f32 g, f32 b)
{    
    //Since I'm dealing with int pixels below
    Recti rectToDraw {};
    rectToDraw.min = RoundFloat32ToInt32(rect.min);
    rectToDraw.max = RoundFloat32ToInt32(rect.max);

    {//Make sure we don't try to draw outside current screen space
        if(rectToDraw.min.x < 0)
            rectToDraw.min.x = 0;

        if(rectToDraw.min.y < 0)
            rectToDraw.min.y = 0;

        if(rectToDraw.max.x > Buffer->width)
            rectToDraw.max.x = Buffer->width;

        if(rectToDraw.max.y > Buffer->height)
            rectToDraw.max.y = Buffer->height;
    };

    ui32 Color = ((RoundFloat32ToUInt32(r * 255.0f) << 16) |
                  (RoundFloat32ToUInt32(g * 255.0f) << 8) |
                  (RoundFloat32ToUInt32(b * 255.0f) << 0));

    ui8* currentRow = ((ui8*)Buffer->memory + rectToDraw.min.x*Buffer->bytesPerPixel + rectToDraw.min.y*Buffer->pitch);
    for(i32 column = rectToDraw.min.y; column < rectToDraw.max.y; ++column)
    {
        ui32* Pixel = (ui32*)currentRow;
        for(i32 row = rectToDraw.min.x; row < rectToDraw.max.x; ++row)
        {            
            *Pixel++ = Color;
        }
        
        currentRow += Buffer->pitch;
    }
}

local_func void
DrawImage(Game_Offscreen_Buffer* Buffer, Image image, Rectf rect)
{
    ui32* imagePixel = (ui32*)image.data;

    //Since I'm dealing with int pixels below
    Recti targetRect{};
    targetRect.min = RoundFloat32ToInt32(rect.min);
    targetRect.max = RoundFloat32ToInt32(rect.max);

    {//Make sure we don't try to draw outside current screen space
        if(targetRect.min.x < 0)
            targetRect.min.x = 0;

        if(targetRect.min.y < 0)
            targetRect.min.y = 0;

        if(targetRect.max.x > Buffer->width)
            targetRect.max.x = Buffer->width;

        if(targetRect.max.y > Buffer->height)
            targetRect.max.y = Buffer->height;
    };

    ui8* currentRow = ((ui8*)Buffer->memory + targetRect.min.x*Buffer->bytesPerPixel + targetRect.min.y*Buffer->pitch);
    for(i32 column = targetRect.min.y; column < targetRect.max.y; ++column)
    {
        ui32* screenPixel = (ui32*)currentRow;

        for(i32 row = targetRect.min.x; row < targetRect.max.x; ++row)
        {            
            ui8 blendedPixel_R, blendedPixel_G, blendedPixel_B; 
            auto [screenPxl_R, screenPxl_G, screenPxl_B, screenPxl_A] = GetRGBAValues(*screenPixel, BGRA);
            auto [imagePxl_R, imagePxl_G, imagePxl_B, imagePxl_A] = GetRGBAValues(*imagePixel, BGRA);

            {//Linear Blend
                f32 blendPercent = (f32)imagePxl_A / 255.0f;

                blendedPixel_R = screenPxl_R + (ui8)(blendPercent * (imagePxl_R - screenPxl_R));
                blendedPixel_G = screenPxl_G + (ui8)(blendPercent * (imagePxl_G - screenPxl_G));
                blendedPixel_B = screenPxl_B + (ui8)(blendPercent * (imagePxl_B - screenPxl_B));
            };

            *screenPixel = ((0xFF << 24) |
                           (blendedPixel_R << 16) |
                           (blendedPixel_G << 8) |
                           (blendedPixel_B << 0));

            ++screenPixel;
            ++imagePixel ;
        }
        
        currentRow += Buffer->pitch;
    }
}

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

    if (NOT gameMemory->Initialized)
    {
        BGZ_ERRCTXT1("When Initializing game memory and game state");

        gameMemory->Initialized = true;

        viewportWidth = 1280.0f;
        viewportHeight = 720.0f;

        InitApplicationMemory(gameMemory);
        CreateRegionFromMemory(gameMemory, Megabytes(500));

        //Stage Init
        stage->info.size.x = viewportWidth;
        stage->info.size.y = viewportHeight;
        stage->info.backgroundImg.data = platformServices->LoadBGRAbitImage("data/mountain.jpg", &stage->info.backgroundImg.size.width, &stage->info.backgroundImg.size.height);

        //Player Init
        player->image.data = platformServices->LoadBGRAbitImage("data/test_body.bmp", &player->image.size.width, &player->image.size.height);
        player->worldPos = {300.0f, 200.0f};
        player->rotationAngle = 0.0f;
    };

    if (globalPlatformServices->DLLJustReloaded)
    {
        BGZ_CONSOLE("Dll reloaded!");
        globalPlatformServices->DLLJustReloaded = false;
    };

    if(KeyHeld(keyboard->MoveRight))
    {
        player->rotationAngle += 2.0f;
    }

    { // Render
        Drawable_Rect playerTargetRect{};
        {
            //With world space origin at 0, 0
            Coordinate_Space playerSpace{};
            playerSpace.origin = player->worldPos;
            playerSpace.xBasis = {CosInRadians(Radians(player->rotationAngle)), SinInRadians(Radians(player->rotationAngle))};
            playerSpace.yBasis = PerpendicularOp(playerSpace.xBasis);

            //Create Rect from world origin so rotation happens correctly
            v2f worldOrigin {0.0f, 0.0f};
            playerTargetRect = ProduceRectFromBottomLeftPoint(worldOrigin, (f32)player->image.size.width, (f32)player->image.size.height);

            //This equation rotates first then moves to correct world position
            playerTargetRect.BottomLeft = playerSpace.origin + (playerTargetRect.BottomLeft.x * playerSpace.xBasis) + (playerTargetRect.BottomLeft.y * playerSpace.yBasis);
            playerTargetRect.BottomRight = playerSpace.origin + (playerTargetRect.BottomRight.x * playerSpace.xBasis) + (playerTargetRect.BottomRight.y * playerSpace.yBasis);
            playerTargetRect.TopRight = playerSpace.origin + (playerTargetRect.TopRight.x * playerSpace.xBasis) + (playerTargetRect.TopRight.y * playerSpace.yBasis);
            playerTargetRect.TopLeft = playerSpace.origin + (playerTargetRect.TopLeft.x * playerSpace.xBasis) + (playerTargetRect.TopLeft.y * playerSpace.yBasis);
        };

        Rectf backgroundTargetRect{v2f{0, 0}, v2f{1280.0f, 720.0f}};
        DrawImage(gameBackBuffer, stage->info.backgroundImg, backgroundTargetRect);

        {//Just for debug purposes so I can see 4 corners of player target rect
            Rectf playerCorner1 = {
                v2f{playerTargetRect.BottomLeft.x - 10.0f, playerTargetRect.BottomLeft.y}, 
                v2f{playerTargetRect.BottomLeft.x, playerTargetRect.BottomLeft.y + 10.0f} 
            };

            Rectf playerCorner2 = {
                v2f{playerTargetRect.BottomRight.x, playerTargetRect.BottomRight.y}, 
                v2f{playerTargetRect.BottomRight.x + 10.0f, playerTargetRect.BottomRight.y + 10.0f} 
            };

            Rectf playerCorner3 = {
                v2f{playerTargetRect.TopRight.x, playerTargetRect.TopRight.y}, 
                v2f{playerTargetRect.TopRight.x + 10.0f, playerTargetRect.TopRight.y + 10.0f} 
            };

            Rectf playerCorner4 = {
                v2f{playerTargetRect.TopLeft.x - 10.0f, playerTargetRect.TopLeft.y}, 
                v2f{playerTargetRect.TopLeft.x, playerTargetRect.TopLeft.y + 10.0f} 
            };

            DrawRectangle(gameBackBuffer, playerCorner1, 0.0f, 0.0f, 1.0f);
            DrawRectangle(gameBackBuffer, playerCorner2, 0.0f, 0.0f, 1.0f);
            DrawRectangle(gameBackBuffer, playerCorner3, 0.0f, 0.0f, 1.0f);
            DrawRectangle(gameBackBuffer, playerCorner4, 0.0f, 0.0f, 1.0f);
        };
    };
};
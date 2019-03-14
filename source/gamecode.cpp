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
DrawRectangle(Game_Offscreen_Buffer* Buffer, v2f minCoord, v2f maxCoord, f32 r, f32 g, f32 b)
{    
    v2i minRectCoords{};
    v2i maxRectCoords{};

    minRectCoords.x = RoundFloat32ToInt32(minCoord.x);
    minRectCoords.y = RoundFloat32ToInt32(minCoord.y);
    maxRectCoords.x = RoundFloat32ToInt32(maxCoord.x);
    maxRectCoords.y = RoundFloat32ToInt32(maxCoord.y);

    {//Make sure we don't try to draw outside current screen space
        if(minRectCoords.x < 0)
            minRectCoords.x = 0;

        if(minRectCoords.y < 0)
            minRectCoords.y = 0;

        if(maxRectCoords.x > Buffer->width)
            maxRectCoords.x = Buffer->width;

        if(maxRectCoords.y > Buffer->height)
            maxRectCoords.y = Buffer->height;
    };

    ui32 Color = ((RoundFloat32ToUInt32(r * 255.0f) << 16) |
                  (RoundFloat32ToUInt32(g * 255.0f) << 8) |
                  (RoundFloat32ToUInt32(b * 255.0f) << 0));

    ui8* currentRow = ((ui8*)Buffer->memory + minRectCoords.x*Buffer->bytesPerPixel + minRectCoords.y*Buffer->pitch);
    for(i32 column = minRectCoords.y; column < maxRectCoords.y; ++column)
    {
        ui32* Pixel = (ui32*)currentRow;
        for(i32 row = minRectCoords.x; row < maxRectCoords.x; ++row)
        {            
            *Pixel++ = Color;
        }
        
        currentRow += Buffer->pitch;
    }
}

local_func void
DrawImage(Game_Offscreen_Buffer* Buffer, Image image, v2i targetPos)
{
    ui32* imagePixel = (ui32*)image.data;

    ui8* currentRow = ((ui8*)Buffer->memory + targetPos.x*Buffer->bytesPerPixel + targetPos.y*Buffer->pitch);
    for(i32 column = targetPos.y; column < image.size.height + targetPos.y; ++column)
    {
        ui32* screenPixel = (ui32*)currentRow;

        for(i32 row = targetPos.x; row < image.size.width + targetPos.x; ++row)
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

    Game_State* gameState = (Game_State*)gameMemory->PermanentStorage;

    //Setup globals
    deltaT = platformServices->prevFrameTimeInSecs;
    deltaTFixed = platformServices->targetFrameTimeInSecs;
    globalPlatformServices = platformServices;
    globalRenderCmds = renderCmds;

    Stage_Data* stage = &gameState->stage;
    Fighter* player = &stage->player;

    if (NOT gameMemory->Initialized)
    {
        BGZ_ERRCTXT1("When Initializing game memory and game state");

        gameMemory->Initialized = true;

        viewportWidth = 1280.0f;
        viewportHeight = 720.0f;

        InitApplicationMemory(gameMemory);
        CreateRegionFromMemory(gameMemory, Megabytes(500));

        gameState->backgroundImg.data = platformServices->LoadBGRAbitImage("data/mountain.jpg", &gameState->backgroundImg.size.width, &gameState->backgroundImg.size.height);
        gameState->image.data = platformServices->LoadBGRAbitImage("data/test_head.bmp", &gameState->image.size.width, &gameState->image.size.height);
        gameState->torso.data = platformServices->LoadBGRAbitImage("data/test_body.bmp", &gameState->torso.size.width, &gameState->torso.size.height);
    };

    if (globalPlatformServices->DLLJustReloaded)
    {
        BGZ_CONSOLE("Dll reloaded!");
        globalPlatformServices->DLLJustReloaded = false;
    };

    if(KeyHeld(keyboard->MoveRight))
    {
        gameState->targetPos3.x += 1;
    }

    { // Render
        //Origin - bottom left
        v2i targetPos{0, 0};
        DrawImage(gameBackBuffer, gameState->backgroundImg, targetPos);
        DrawImage(gameBackBuffer, gameState->torso, gameState->targetPos3);
        DrawImage(gameBackBuffer, gameState->image, gameState->targetPos3 + v2i{100, 100});
    };
};
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

local_func void
DrawRectangle(Game_Offscreen_Buffer* Buffer, v2f minCoord, v2f maxCoord, f32 r, f32 g, f32 b)
{    
    v2i roundedMinCoords{};
    v2i roundedMaxCoords{};

    roundedMinCoords.x = RoundFloat32ToInt32(minCoord.x);
    roundedMinCoords.y = RoundFloat32ToInt32(minCoord.y);
    roundedMaxCoords.x = RoundFloat32ToInt32(maxCoord.x);
    roundedMaxCoords.y = RoundFloat32ToInt32(maxCoord.y);

    {//Make sure we don't try to draw outside current screen space
        if(roundedMinCoords.x < 0)
            roundedMinCoords.x = 0;

        if(roundedMinCoords.y < 0)
            roundedMinCoords.y = 0;

        if(roundedMaxCoords.x > Buffer->width)
            roundedMaxCoords.x = Buffer->width;

        if(roundedMaxCoords.y > Buffer->height)
            roundedMaxCoords.y = Buffer->height;
    };

    ui32 Color = ((RoundFloat32ToUInt32(r * 255.0f) << 16) |
                  (RoundFloat32ToUInt32(g * 255.0f) << 8) |
                  (RoundFloat32ToUInt32(b * 255.0f) << 0));

    ui8* Row = ((ui8*)Buffer->memory + roundedMinCoords.x*Buffer->bytesPerPixel + roundedMinCoords.y*Buffer->pitch);
    for(i32 Y = roundedMinCoords.y; Y < roundedMaxCoords.y; ++Y)
    {
        ui32* Pixel = (ui32*)Row;
        for(i32 X = roundedMinCoords.x; X < roundedMaxCoords.x; ++X)
        {            
            *Pixel++ = Color;
        }
        
        Row += Buffer->pitch;
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

        i32 imageWidth{}, imageHeight{};
        //ui8* imageData = platformServices->LoadRGBAImage("", &imageWidth, &imageHeight);
    };

    if (globalPlatformServices->DLLJustReloaded)
    {
        BGZ_CONSOLE("Dll reloaded!");
        globalPlatformServices->DLLJustReloaded = false;
    };

    if (KeyComboPressed(keyboard->ActionLeft, keyboard->MoveRight))
    {
    }

    if (KeyPressed(keyboard->ActionRight))
    {
    }

    if (KeyHeld(keyboard->MoveRight))
    {
    }

    if (KeyHeld(keyboard->MoveLeft))
    {
    }

    { // Render
        v2f minRectCoords{0.0f, 0.0f};
        v2f maxRectCoords{(f32)gameBackBuffer->width, (f32)gameBackBuffer->height};
        DrawRectangle(gameBackBuffer, minRectCoords, maxRectCoords, 0.0f, 1.0f, 1.0f);
    };
};
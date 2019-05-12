#pragma once

#include "math.h"
#include "utilities.h"

struct Button_State
{
    //1 button transition == from button up to down or vice versa (up->down or down->up), NOT up->down->up (which would be one full button press and release)
    //Capturing these transistions just helps to ensure we do not miss any button presses during a frame (so each press that was polled and captured by
    //the OS will be reflected within our current input scheme)
    i32 NumTransitionsPerFrame;
    b32 Pressed;
};

struct Game_Controller
{
    b32 IsConnected;
    b32 IsAnalog;

    v2f LThumbStick;
    v2f RThumbStick;

    union
    {
        Button_State Buttons[14];
        struct
        {
            Button_State MoveUp;
            Button_State MoveDown;
            Button_State MoveLeft;
            Button_State MoveRight;

            Button_State ActionUp;
            Button_State ActionDown;
            Button_State ActionLeft;
            Button_State ActionRight;

            Button_State LeftShoulder;
            Button_State RightShoulder;
            Button_State LeftTrigger;
            Button_State RightTrigger;

            Button_State Back;
            Button_State Start;
            //All buttons must be added above this line
        };
    };
};

auto ClearTransitionCounts(Game_Controller* Controller) -> void
{
    for (int ButtonIndex = 0; ButtonIndex < ArrayCount(Controller->Buttons); ++ButtonIndex)
    {
        Controller->Buttons[ButtonIndex].NumTransitionsPerFrame = 0;
    };
}

struct Game_Input
{
    Game_Controller Controllers[5];
};

struct Game_Sound_Output_Buffer
{
};

struct Game_Offscreen_Buffer
{
    //Pixels are alwasy 32-bits wide, Memory Order BB GG RR XX
    void *memory;
    i32 width;
    i32 height;
    i32 pitch;
};

struct Platform_Services
{
    char* (*ReadEntireFile)(i32&&, const char*);
    bool (*WriteEntireFile)(const char*, void*, ui32);
    void (*FreeFileMemory)(void*);
    ui8* (*LoadBGRAImage)(const char*, i32&&, i32&&);
    void* (*Malloc)(sizet);
    void* (*Calloc)(sizet, sizet);
    void* (*Realloc)(void*, sizet);
    void (*Free)(void*);
    b DLLJustReloaded { false };
    f32 prevFrameTimeInSecs {};
    f32 targetFrameTimeInSecs {};
    f32 realLifeTimeInSecs {};
};

enum ChannelType
{
    RGBA = 1,
    BGRA
};

local_func v4f
UnPackPixelValues(ui32 pixel, ChannelType channelType)
{
    v4f result {};

    ui32* pixelInfo = &pixel;

    switch(channelType)
    {
        case RGBA:
        {
            result.r = (f32)*((ui8*)pixelInfo + 0);
            result.g = (f32)*((ui8*)pixelInfo + 1);
            result.b = (f32)*((ui8*)pixelInfo + 2);
            result.a = (f32)*((ui8*)pixelInfo + 3);
        }break;

        case BGRA:
        {
            result.b = (f32)*((ui8*)pixelInfo + 0);
            result.g = (f32)*((ui8*)pixelInfo + 1);
            result.r = (f32)*((ui8*)pixelInfo + 2);
            result.a = (f32)*((ui8*)pixelInfo + 3);
        }break;

        default : break;
    };

    return result;
};

struct Game_Render_Cmd_Buffer
{
    ui8* baseAddress;
    ui32 usedAmount;
    ui32 size;
    i32 entryCount;
};

////////RENDER/GAME STUFF - NEED TO MOVE OUT////////////////////////////////////////////

struct Coordinate_Space
{
    v2f origin;
    v2f xBasis;
    v2f yBasis;
};

struct Transform
{
    f32 rotation;
    v2f pos;
    v2f scale;
};

struct Image
{
    ui8* data;
    v2i size;
    ui32 pitch;
    f32 opacity {1.0f};
};

struct Texture
{
    ui32 ID;
    v2i size;
};


#pragma once

#include "my_math.h"
#include "utilities.h"

struct Button_State
{
    //1 button transition == from button up to down or vice versa (up->down or down->up), NOT up->down->up (which would be one full button press and release)
    //Capturing these transistions just helps to ensure we do not miss any button presses during a frame (so each press that was polled and captured by
    //the OS will be reflected within our current input scheme)
    s32 NumTransitionsPerFrame;
    b32 Pressed;
};

struct Game_Controller
{
    b32 IsConnected;
    b32 IsAnalog;
    
    v2 LThumbStick;
    v2 RThumbStick;
    
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

void ClearTransitionCounts(Button_State* buttons)
{
    for(s32 buttonIndex{}; buttonIndex < ArrayCount(buttons); ++buttonIndex)
        buttons[buttonIndex].NumTransitionsPerFrame = 0;
};

void ClearTransitionCounts(Game_Controller* Controller)
{
    for (int ButtonIndex = 0; ButtonIndex < ArrayCount(Controller->Buttons); ++ButtonIndex)
        Controller->Buttons[ButtonIndex].NumTransitionsPerFrame = 0;
}

enum Mouse
{
    LEFT_CLICK,
    RIGHT_CLICK,
    WHEEL_CLICK
};

struct Game_Input
{
    s32 mouseX, mouseY;//TODO: Support mouse wheel pos
    Button_State mouseButtons[3];//Left click, right click, and mouse wheel click
    Game_Controller Controllers[5];//0 index is reserved for keyboard
};

struct Game_Sound_Output_Buffer
{
};

struct Game_Offscreen_Buffer
{
    //Pixels are alwasy 32-bits wide, Memory Order BB GG RR XX
    void *memory;
    s32 width;
    s32 height;
    s32 pitch;
};

#define PLATFORM_WORK_QUEUE_CALLBACK(name) void name(void *data)
typedef PLATFORM_WORK_QUEUE_CALLBACK(platform_work_queue_callback);

struct Platform_Services
{
    char* (*ReadEntireFile)(s32&&, const char*);
    bool  (*WriteEntireFile)(const char*, void*, u32);
    void (*FreeFileMemory)(void*);
    u8* (*LoadBGRAImage)(const char*, s32&&, s32&&);
    void* (*Malloc)(sizet);
    void* (*Calloc)(sizet, sizet);
    void* (*Realloc)(void*, sizet);
    void (*Free)(void*);
    void (*AddWorkQueueEntry)(platform_work_queue_callback, void*);
    void (*FinishAllWork)(void);
    bool DLLJustReloaded { false };
    f32 prevFrameTimeInSecs {};
    f32 targetFrameTimeInSecs {};
    f32 realLifeTimeInSecs {};
};

enum ChannelType
{
    RGBA = 1,
    BGRA
};

local_func v4
UnPackPixelValues(u32 pixel, ChannelType channelType)
{
    v4 result {};
    
    u32* pixelInfo = &pixel;
    
    switch(channelType)
    {
        case RGBA:
        {
            result.r = (f32)*((u8*)pixelInfo + 0);
            result.g = (f32)*((u8*)pixelInfo + 1);
            result.b = (f32)*((u8*)pixelInfo + 2);
            result.a = (f32)*((u8*)pixelInfo + 3);
        }break;
        
        case BGRA:
        {
            result.b = (f32)*((u8*)pixelInfo + 0);
            result.g = (f32)*((u8*)pixelInfo + 1);
            result.r = (f32)*((u8*)pixelInfo + 2);
            result.a = (f32)*((u8*)pixelInfo + 3);
        }break;
        
        default : break;
    };
    
    return result;
};

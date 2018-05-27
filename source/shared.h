#pragma once

#include "types.h"
#include "utilities.h"

struct Game_Memory
{
    bool IsInitialized{false};

    uint32 SizeOfPermanentStorage{};
    void* PermanentStorage{nullptr};
    uint64 SizeOfTemporaryStorage{};
    void* TemporaryStorage{nullptr};
    uint64 TotalSize{};
};

struct Button_State
{
    //1 button transition == from button up to down or vice versa, NOT up->down->up (which would be one full button press and release)
    //Capturing these transistions just helps to ensure we do not miss a button press (e.g. if user presses button once at
    //beginning of the frame, and then again at the end assuming multiple polling, with this scheme we would be able to capture 
    //both presses instead of just the one at the beginning).
    int NumTransitionsPerFrame; 
    bool32 Pressed;
};

struct Game_Controller
{
    bool32 IsConnected;
    bool32 IsAnalog;    

    vec2 LThumbStick;
    vec2 RThumbStick;

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

struct Texture
{
    unsigned char* ImageData;
    int Width;
    int Height;
    uint ID;
};

struct Game_Render_Cmds
{
    void (*DrawRect)(vec2, vec2, vec2, vec2, vec3);
    void (*ClearScreen)();
    void (*DrawTexture)(Texture, vec2, float32, float32);
    void (*DrawBackground)(Texture, vec2, vec2, float32);
    uint (*LoadTexture)(Texture);
    void (*Init)();
};

struct Read_File_Result {void* FileContents{nullptr}; uint32 FileSize{};};
struct Platform_Services
{
    Read_File_Result (*ReadEntireFile)(const char*);
    bool (*WriteEntireFile)(const char*, void*, uint32);
    void (*FreeFileMemory)(void*);
    unsigned char* (*LoadRGBAImage)(const char*, int*, int*);
};
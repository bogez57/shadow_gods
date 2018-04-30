#pragma once

#include "types.h"
#include "utilities.h"

struct Game_Memory
{
    bool IsInitialized{false};

    uint32 PermanentStorageSize{};
    void* PermanentStorage{nullptr};
    uint64 TemporaryStorageSize{};
    void* TemporaryStorage{nullptr};
};

struct Button_State
{
    //1 button transition == from button up to down or vice versa, NOT up->down->up (which would be one full button press and release)
    //Storing these transisitons is probably more of an issue at lower frame rates (sub 60fps)
    int NumTransitionsPerFrame; 
    bool32 Pressed;
};

struct Game_Controller
{
    bool32 IsConnected;
    bool32 IsAnalog;    
    
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
    Game_Controller Controllers[2];
};

struct Game_Sound_Output_Buffer
{

};

struct Game_Render_Cmd_Buffer
{

};

struct Read_File_Result {void* FileContents{nullptr}; uint32 FileSize{};};
struct Platform_Services
{
    Read_File_Result (*ReadEntireFile)(const char*);
    bool (*WriteEntireFile)(const char*, void*, uint32);
    void (*FreeFileMemory)(void*);
};
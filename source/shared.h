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

struct Read_File_Result {void* FileContents{nullptr}; uint32 FileSize{};};
struct Platform_Services
{
    Read_File_Result (*ReadEntireFile)(const char*);
    bool (*WriteEntireFile)(const char*, void*, uint32);
    void (*FreeFileMemory)(void*);
    unsigned char* (*LoadRGBAImage)(const char*, int*, int*);
};

////////////////////////////////////////////
/*
    All Game/Render related things below. Will move out eventually since platform layer does not need to know about anything
    here. Only here in the meantime for easier experimenting with render through TestArena Render_Cmd 
*/
////////////////////////////////////////////

struct Texture
{
    unsigned char* ImageData;
    int Width;
    int Height;
    uint ID;
};

struct Rect
{
    union 
    {
        vec2 Corners[4];
        struct
        {
            vec2 BottomLeft;
            vec2 BottomRight;
            vec2 TopRight;
            vec2 TopLeft;
        };
    };
};

struct Coordinate_Space
{
    vec2 Origin;
    vec2 XBasis;
    vec2 YBasis;
};

struct Player
{
    float32 Width;
    float32 Height;
    vec2 WorldPos;
    Texture CurrentTexture;
};

struct Level
{
    float32 Width;
    float32 Height;
    vec2 CenterPoint;
    Texture BackgroundTexture;
};

struct Camera
{
    vec2 LookAt;
    vec2 ViewCenter;
    float32 ViewWidth;
    float32 ViewHeight;
    float32 ZoomFactor;
};

struct Game_State
{
    Camera GameCamera;
    Level GameLevel;
    Player Fighter;
};

struct Game_Render_Cmds
{
    void (*ClearScreen)();
    void (*TestArena)(Game_State*);
    void (*DrawRect)(vec2, vec2);
    void (*DrawBackground)(uint, vec2, vec2, vec2);
    void (*DrawTexture)(uint, Rect, vec2, vec2);
    uint (*LoadTexture)(Texture);
    void (*Init)();
};

auto
ProduceRectFromCenter(vec2 OriginPoint, float32 Width, float32 Height) -> Rect
{
    Rect Result;
    vec2 MinPoint;
    vec2 MaxPoint;

    MinPoint = {
        OriginPoint.x - (Width / 2),
        OriginPoint.y - (Height / 2)};
    MaxPoint = {
        OriginPoint.x + (Width / 2),
        OriginPoint.y + (Height / 2)};

    Result.BottomLeft = MinPoint;
    Result.BottomRight.x = MinPoint.x + Width;
    Result.BottomRight.y = MinPoint.y;
    Result.TopRight = MaxPoint;
    Result.TopLeft.x = MinPoint.x;
    Result.TopLeft.y = MaxPoint.y;

    return Result;
};

auto 
ProduceRectFromBottomLeft(vec2 OriginPoint, float32 Width, float32 Height) -> Rect
{
    Rect Result;

    Result.BottomLeft = OriginPoint;
    Result.BottomRight.x = OriginPoint.x + Width;
    Result.BottomRight.y = OriginPoint.y;
    Result.TopRight.x = OriginPoint.x + Width;
    Result.TopRight.y = OriginPoint.y + Height;
    Result.TopLeft.x = OriginPoint.x;
    Result.TopLeft.y = OriginPoint.y + Height;

    return Result;
};
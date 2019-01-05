#pragma once

#include "atomic_types.h"
#include "math.h"
#include "utilities.h"
#include "memory_handling.h"

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

struct Platform_Services
{
    char* (*ReadEntireFile)(i64*, const char*);
    bool (*WriteEntireFile)(const char*, void*, ui32);
    void (*FreeFileMemory)(void*);
    ui8* (*LoadRGBAImage)(const char*, int*, int*);
    void* (*Malloc)(sizet);
    void* (*Calloc)(sizet, sizet);
    void* (*Realloc)(void*, sizet);
    void (*Free)(void*);
    b DLLJustReloaded { false };
    f32 prevFrameTimeInSecs {};
    f32 targetFrameTimeInSecs {};
    f32 realLifeTimeInSecs {};
};

////////RENDER/GAME STUFF - NEED TO MOVE OUT////////////////////////////////////////////

struct Image
{
    ui8* Data;
    v2i size;
};

struct Texture
{
    ui32 ID;
    v2i size;
};

struct Drawable_Rect
{
    union
    {
        v2f Corners[4];
        struct
        {
            v2f BottomLeft;
            v2f BottomRight;
            v2f TopRight;
            v2f TopLeft;
        };
    };
};

struct Coordinate_System
{
    v2f Origin;
    v2f XBasis;
    v2f YBasis;
};

auto ProduceRectFromCenterPoint(v2f OriginPoint, f32 Width, f32 Height) -> Drawable_Rect
{
    Drawable_Rect Result;
    v2f MinPoint;
    v2f MaxPoint;

    MinPoint = { OriginPoint.x - (Width / 2), OriginPoint.y - (Height / 2) };
    MaxPoint = { OriginPoint.x + (Width / 2), OriginPoint.y + (Height / 2) };

    Result.BottomLeft = MinPoint;
    Result.BottomRight.x = MinPoint.x + Width;
    Result.BottomRight.y = MinPoint.y;
    Result.TopRight = MaxPoint;
    Result.TopLeft.x = MinPoint.x;
    Result.TopLeft.y = MaxPoint.y;

    return Result;
};

auto ProduceRectFromBottomMidPoint(v2f OriginPoint, f32 Width, f32 Height) -> Drawable_Rect
{
    Drawable_Rect Result;

    Result.BottomLeft = { OriginPoint.x - (Width / 2.0f), OriginPoint.y };
    Result.BottomRight = { OriginPoint.x + (Width / 2.0f), OriginPoint.y };
    Result.TopRight = { Result.BottomRight.x, OriginPoint.y + Height };
    Result.TopLeft = { Result.BottomLeft.x, OriginPoint.y + Height };

    return Result;
};

auto ProduceRectFromBottomLeftPoint(v2f OriginPoint, f32 Width, f32 Height) -> Drawable_Rect
{
    Drawable_Rect Result;

    Result.BottomLeft = OriginPoint;
    Result.BottomRight.x = OriginPoint.x + Width;
    Result.BottomRight.y = OriginPoint.y;
    Result.TopRight.x = OriginPoint.x + Width;
    Result.TopRight.y = OriginPoint.y + Height;
    Result.TopLeft.x = OriginPoint.x;
    Result.TopLeft.y = OriginPoint.y + Height;

    return Result;
};

auto LinearRotation(f32 RotationInDegress, v2f VectorToRotate) -> v2f
{
    f32 RotationInRadians = RotationInDegress * (PI / 180.0f);
    v2f RotatedXBasis = VectorToRotate.x * v2f { Cos(RotationInRadians), Sin(RotationInRadians) };
    v2f RotatedYBasis = VectorToRotate.y * v2f { -Sin(RotationInRadians), Cos(RotationInRadians) };
    v2f NewRotatedVector = RotatedXBasis + RotatedYBasis;

    return NewRotatedVector;
};

auto DilateAboutArbitraryPoint(v2f PointOfDilation, f32 ScaleFactor, Drawable_Rect RectToDilate) -> Drawable_Rect
{
    Drawable_Rect DilatedRect {};

    for (i32 CornerIndex = 0; CornerIndex < ArrayCount(RectToDilate.Corners); ++CornerIndex)
    {
        v2f Distance = PointOfDilation - RectToDilate.Corners[CornerIndex];
        Distance *= ScaleFactor;
        DilatedRect.Corners[CornerIndex] = PointOfDilation - Distance;
    };

    return DilatedRect;
};

auto RotateAboutArbitraryPoint(v2f PointOfRotation, f32 DegreeOfRotation, Drawable_Rect RectToRotate) -> Drawable_Rect
{
    Drawable_Rect RotatedRect {};

    for (i32 CornerIndex = 0; CornerIndex < ArrayCount(RectToRotate.Corners); ++CornerIndex)
    {
    };

    return RotatedRect;
};

struct Game_Render_Cmds
{
    void (*ClearScreen)();
    void (*DrawRect)(v2f, v2f, v4f);
    void (*DrawBackground)(ui32, Drawable_Rect, v2f, v2f);
    void (*DrawTexture)(ui32, Drawable_Rect, v2f*);
    Texture (*LoadTexture)(Image);
    void (*Init)();
};

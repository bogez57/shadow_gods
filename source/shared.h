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
    ui8* (*LoadBGRAbitImage)(const char*, i32&&, i32&&);
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
    ui8* data;
    v2i size;
    ui32 pitch;
    f32 opacity {1.0f};
};

struct Rectf
{
    v2f min;
    v2f max;
};

struct Recti
{
    v2i min;
    v2i max;
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

struct Texture
{
    ui32 ID;
    v2i size;
};

enum ChannelType
{
    RGBA = 1,
    BGRA
};

local_func v4f
GetRGBAValues(ui32 pixel, ChannelType channelType)
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

auto LinearBlend(ui32 foregroundColor, ui32 backgroundColor, ChannelType colorFormat) 
{
    struct Result {ui8 blendedPixel_R, blendedPixel_G, blendedPixel_B;};
    Result blendedColor{};
    
    v4f foreGroundColors = GetRGBAValues(foregroundColor, colorFormat);
    v4f backgroundColors = GetRGBAValues(backgroundColor, colorFormat);

    f32 blendPercent = foreGroundColors.a / 255.0f;

    blendedColor.blendedPixel_R = (ui8)Lerp(backgroundColors.r, foreGroundColors.r, blendPercent);
    blendedColor.blendedPixel_G = (ui8)Lerp(backgroundColors.g, foreGroundColors.g, blendPercent);
    blendedColor.blendedPixel_B = (ui8)Lerp(backgroundColors.b, foreGroundColors.b, blendPercent);

    return blendedColor;
};

auto ProduceRectFromCenterPoint(v2f OriginPoint, f32 width, f32 height) -> Rectf
{
    Rectf Result;

    Result.min = { OriginPoint.x - (width / 2), OriginPoint.y - (height / 2) };
    Result.max = { OriginPoint.x + (width / 2), OriginPoint.y + (height / 2) };

    return Result;
};

auto ProduceRectFromBottomMidPoint(v2f OriginPoint, f32 width, f32 height) -> Rectf
{
    Rectf Result;

    Result.min = { OriginPoint.x - (width / 2.0f), OriginPoint.y };
    Result.max = { OriginPoint.x + (width / 2.0f), OriginPoint.y + height };

    return Result;
};

Drawable_Rect ProduceRectFromBottomLeftPoint(v2f originPoint, f32 width, f32 height)
{
    Drawable_Rect Result;

    Result.BottomLeft = originPoint;
    Result.BottomRight = { originPoint.x + width, originPoint.y };
    Result.TopRight = { originPoint.x + width, originPoint.y + height };
    Result.TopLeft = { originPoint.x, originPoint.y + height };

    return Result;
};

auto LinearRotation(f32 RotationInDegress, v2f VectorToRotate) -> v2f
{
    f32 RotationInRadians = RotationInDegress * (PI / 180.0f);
    v2f RotatedXBasis = VectorToRotate.x * v2f { CosInRadians(RotationInRadians), SinInRadians(RotationInRadians) };
    v2f RotatedYBasis = VectorToRotate.y * v2f { -SinInRadians(RotationInRadians), CosInRadians(RotationInRadians) };
    v2f NewRotatedVector = RotatedXBasis + RotatedYBasis;

    return NewRotatedVector;
};

auto DilateAboutArbitraryPoint(v2f PointOfDilation, f32 ScaleFactor, Rectf RectToDilate) -> Rectf
{
    Rectf DilatedRect {};

    v2f Distance = PointOfDilation - RectToDilate.min;
    Distance *= ScaleFactor;
    DilatedRect.min = PointOfDilation - Distance;

    Distance = PointOfDilation - RectToDilate.max;
    Distance *= ScaleFactor;
    DilatedRect.max= PointOfDilation - Distance;

    return DilatedRect;
};

auto RotateAboutArbitraryPoint(v2f PointOfRotation, f32 DegreeOfRotation, Rectf RectToRotate) -> Rectf
{
    Rectf RotatedRect {};

    return RotatedRect;
};

struct Game_Render_Cmds
{
    void (*ClearScreen)();
    void (*DrawStuff)();
    void (*DrawRect)(v2f, v2f, v4f);
    void (*DrawBackground)(ui32, Rectf, v2f, v2f);
    void (*DrawTexture)(ui32, Rectf, v2f*);
    void (*DrawLine)(v2f, v2f);
    Texture (*LoadTexture)(Image);
    void (*Init)();
};

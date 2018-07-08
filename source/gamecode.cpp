#if (DEVELOPMENT_BUILD)
    #define BGZ_LOGGING_ON true
    #define BGZ_ERRHANDLING_ON true
#else
    #define BGZ_LOGGING_ON false
    #define BGZ_ERRHANDLING_ON false
#endif

#include <boagz/error_handling.h>
#include <boagz/error_context.cpp>

#include "gamecode.h"
#include "shared.h"
#include "math.h"

global_variable float32 ViewportWidth;
global_variable float32 ViewportHeight;

local_func auto 
InitMemoryChunk(Memory_Chunk* MemoryChunkToInit, uint64 SizeToReserve, uint64* StartingAddress) -> void
{
    MemoryChunkToInit->BaseAddress = StartingAddress;
    MemoryChunkToInit->Size = SizeToReserve;
    MemoryChunkToInit->UsedMemory = 0;
};

#define PushStruct(MemoryChunk, Type) (Type*)PushStruct_(MemoryChunk, sizeof(Type));
auto
PushStruct_(Memory_Chunk* MemoryChunk, uint64 Size) -> void*
{
    BGZ_ASSERT((MemoryChunk->UsedMemory + Size) <= MemoryChunk->Size);
    void* Result = MemoryChunk->BaseAddress + MemoryChunk->UsedMemory;
    MemoryChunk->UsedMemory += Size;

    return Result;
}

extern "C" void
GameUpdate(Game_Memory* GameMemory, Platform_Services PlatformServices, Game_Render_Cmds RenderCmds, 
                    Game_Sound_Output_Buffer* SoundOutput, const Game_Input* GameInput)
{
    Game_State* GameState = (Game_State*)GameMemory->PermanentStorage;

    Camera* GameCamera = &GameState->GameCamera;
    Limb* F1Head = &GameState->Head;
    Limb* F1Torso = &GameState->Torso;
    Limb* F1LeftThigh = &GameState->LeftThigh;
    Limb* F1RightThigh = &GameState->RightThigh;
    Player* Fighter1 = &GameState->Fighter1;
    Level* GameLevel = &GameState->GameLevel;

    const Game_Controller* Keyboard = &GameInput->Controllers[0];
    const Game_Controller* GamePad = &GameInput->Controllers[1];

    if(!GameMemory->IsInitialized)
    {
        GameMemory->IsInitialized = true;
        ViewportWidth = 1280.0f;
        ViewportHeight = 720.0f;

        {//Init Game State
            GameLevel->BackgroundTexture.ImageData = PlatformServices.LoadRGBAImage(
                                                                            "4k.jpg", 
                                                                            &GameLevel->BackgroundTexture.Width,
                                                                            &GameLevel->BackgroundTexture.Height);
            GameLevel->BackgroundTexture.ID = RenderCmds.LoadTexture(GameLevel->BackgroundTexture);
            GameLevel->Width = (float32)GameLevel->BackgroundTexture.Width;
            GameLevel->Height = (float32)GameLevel->BackgroundTexture.Height;
            GameLevel->CenterPoint = {GameLevel->Width / 2, GameLevel->Height / 2};

            F1Head->CurrentTexture.ImageData = PlatformServices.LoadRGBAImage(
                                                     "test/Head.JPEG", 
                                                     &F1Head->Width,
                                                     &F1Head->Height);
            F1Head->CurrentTexture.ID = RenderCmds.LoadTexture(F1Head->CurrentTexture);

            F1Torso->CurrentTexture.ImageData = PlatformServices.LoadRGBAImage(
                                                     "test/Torso.JPEG", 
                                                     &F1Torso->Width,
                                                     &F1Torso->Height);
            F1Torso->CurrentTexture.ID = RenderCmds.LoadTexture(F1Torso->CurrentTexture);

            F1LeftThigh->CurrentTexture.ImageData = PlatformServices.LoadRGBAImage(
                                                     "test/Left Thigh.JPEG", 
                                                     &F1LeftThigh->Width,
                                                     &F1LeftThigh->Height);
            F1LeftThigh->CurrentTexture.ID = RenderCmds.LoadTexture(F1LeftThigh->CurrentTexture);

            F1RightThigh->CurrentTexture.ImageData = PlatformServices.LoadRGBAImage(
                                                     "test/Right Thigh.JPEG", 
                                                     &F1RightThigh->Width,
                                                     &F1RightThigh->Height);
            F1RightThigh->CurrentTexture.ID = RenderCmds.LoadTexture(F1RightThigh->CurrentTexture);

            Fighter1->Limbs[0] = F1Head;
            Fighter1->WorldPos = GameLevel->CenterPoint;
            Fighter1->Scale = 1.0f;
            Fighter1->DegreeOfRotation = 0.0f;

            GameCamera->ViewWidth = ViewportWidth;
            GameCamera->ViewHeight = ViewportHeight;
            GameCamera->LookAt = GameLevel->CenterPoint;
            GameCamera->ViewCenter = {GameCamera->ViewWidth/2.0f, GameCamera->ViewHeight/2.0f};
            GameCamera->DilatePoint = GameCamera->ViewCenter - vec2{0.0f, 200.0f};
            GameCamera->ZoomFactor = 1.0f;
        };
    }

    if(Keyboard->MoveUp.Pressed)
    {
    }

    if(Keyboard->MoveDown.Pressed)
    {
    }

    if(Keyboard->MoveRight.Pressed)
    {
    }

    if(Keyboard->MoveLeft.Pressed)
    {
    }

    if(Keyboard->ActionUp.Pressed)
    {
        GameCamera->ZoomFactor += .01f;
    }

    if(Keyboard->ActionDown.Pressed)
    {
        GameCamera->ZoomFactor -= .01f;
    }

    if(Keyboard->ActionRight.Pressed)
    {
    }

    if(Keyboard->ActionLeft.Pressed)
    {
    }

    {//Render
        RenderCmds.Init();

        RenderCmds.ClearScreen();

        {//Draw Level Background
            Coordinate_System BackgroundWorldSpace{};
            Coordinate_System BackgroundCameraSpace{};
            Drawable_Rect BackgroundCanvas{};

            BackgroundWorldSpace.Origin = {0.0f, 0.0f};

            {//Transform to Camera Space
                vec2 TranslationToCameraSpace = GameCamera->ViewCenter - GameCamera->LookAt;
                BackgroundCameraSpace.Origin = BackgroundWorldSpace.Origin + TranslationToCameraSpace;
            };

            BackgroundCanvas = ProduceRectFromBottomLeftPoint(
                                        BackgroundCameraSpace.Origin, 
                                        GameLevel->Width, 
                                        GameLevel->Height);

            BackgroundCanvas = DilateAboutPoint(GameCamera->DilatePoint, GameCamera->ZoomFactor, BackgroundCanvas);

            RenderCmds.DrawTexture(GameLevel->BackgroundTexture.ID, BackgroundCanvas, vec2{0.0f, 0.0f}, vec2{1.0f, 1.0f});
        };

        {//Draw Players
            Coordinate_System FighterWorldSpace{};
            Coordinate_System FighterCameraSpace{};
            Drawable_Rect Limb{};

            FighterWorldSpace.Origin = Fighter1->WorldPos;

            { //Transform to Camera Space
                vec2 TranslationToCameraSpace = GameCamera->ViewCenter - GameCamera->LookAt;
                FighterCameraSpace.Origin = FighterWorldSpace.Origin + TranslationToCameraSpace;
            };

            for(int32 LimbIndex{0}; LimbIndex < ArrayCount(Fighter1->Limbs); ++LimbIndex)
            {
                Fighter1->Limbs[LimbIndex]->WorldPos = {FighterCameraSpace.Origin.x + 20.0f, FighterCameraSpace.Origin.y + 0.0f};

                Limb = ProduceRectFromBottomLeftPoint(
                                        Fighter1->Limbs[LimbIndex]->WorldPos, 
                                        (float32)Fighter1->Limbs[LimbIndex]->Width,
                                        (float32)Fighter1->Limbs[LimbIndex]->Height);

                RenderCmds.DrawTexture(
                                Fighter1->Limbs[LimbIndex]->CurrentTexture.ID, 
                                Limb, 
                                vec2{0.0f, 0.0f}, 
                                vec2{1.0f, 1.0f});
            };
        };
    };
};

/*

Coordinate_System FighterWorldSpace{};
Coordinate_System FighterCameraSpace{};
Drawable_Rect FighterRect{};

FighterWorldSpace.Origin = Fighters[FighterIndex]->WorldPos;

{ //Transform to Camera Space
    vec2 TranslationToCameraSpace = GameCamera->ViewCenter - GameCamera->LookAt;
    FighterCameraSpace.Origin = FighterWorldSpace.Origin + TranslationToCameraSpace;
};

{ //Perform Transformations (Rotation, Scale, etc.)
    //Translate to origin
    vec2 SpaceTempOrigin = FighterCameraSpace.Origin - FighterCameraSpace.Origin;

    float32 RotatedAngle = Fighters[FighterIndex]->DegreeOfRotation * (PI / 180.0f);
    float32 ScaleFactor = Fighters[FighterIndex]->Scale;
    FighterCameraSpace.XBasis = {ScaleFactor * (Cos(RotatedAngle)), ScaleFactor * (Sin(RotatedAngle))};
    FighterCameraSpace.YBasis = {ScaleFactor * (-Sin(RotatedAngle)), ScaleFactor * (Cos(RotatedAngle))};

    FighterRect = ProduceRectFromBottomLeftPoint(SpaceTempOrigin, Fighters[FighterIndex]->Size.Width, Fighters[FighterIndex]->Size.Height);

    for (int CornerIndex = 0; CornerIndex < ArrayCount(FighterRect.Corners); ++CornerIndex)
    {
        vec2 NewCoordX = FighterRect.Corners[CornerIndex].x * FighterCameraSpace.XBasis;
        vec2 NewCoordY = FighterRect.Corners[CornerIndex].y * FighterCameraSpace.YBasis;
        FighterRect.Corners[CornerIndex] = NewCoordX + NewCoordY;

        //Translate Back to Origin
        FighterRect.Corners[CornerIndex] += FighterCameraSpace.Origin;
    };
};

FighterRect = DilateAboutPoint(GameCamera->DilatePoint, GameCamera->ZoomFactor, FighterRect);

RenderCmds.DrawTexture(Fighters[FighterIndex]->CurrentTexture.ID, FighterRect, vec2{0.0f, 0.0f}, vec2{1.0f, 1.0f});

*/
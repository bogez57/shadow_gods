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
    Player* Fighter1 = &GameState->Fighter1;
    Player* Fighter2 = &GameState->Fighter2;
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

            Fighter1->CurrentTexture.ImageData = PlatformServices.LoadRGBAImage(
                                                                        "Fighter.jpg", 
                                                                        &Fighter1->CurrentTexture.Width,
                                                                        &Fighter1->CurrentTexture.Height);
            Fighter1->CurrentTexture.ID = RenderCmds.LoadTexture(Fighter1->CurrentTexture);
            Fighter1->WorldPos = GameLevel->CenterPoint - vec2{400.0f, 300.0f};
            Fighter1->Size.Width = 100.0f;
            Fighter1->Size.Height = 200.0f;

            Fighter2->CurrentTexture.ImageData = PlatformServices.LoadRGBAImage(
                                                                        "Fighter.jpg", 
                                                                        &Fighter2->CurrentTexture.Width,
                                                                        &Fighter2->CurrentTexture.Height);
            Fighter2->CurrentTexture.ID = RenderCmds.LoadTexture(Fighter2->CurrentTexture);
            Fighter2->WorldPos = GameLevel->CenterPoint + vec2{300.0f, -300.0f};
            Fighter2->Size.Width = 100.0f;
            Fighter2->Size.Height = 200.0f;

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
        Fighter1->WorldPos.y += 5.0f;
    }

    if(Keyboard->MoveDown.Pressed)
    {
        Fighter1->WorldPos.y -= 5.0f;
    }

    if(Keyboard->MoveRight.Pressed)
    {
        Fighter1->WorldPos.x += 5.0f;
    }

    if(Keyboard->MoveLeft.Pressed)
    {
        Fighter1->WorldPos.x -= 5.0f;
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
        GameCamera->LookAt.x += 2.0f;
    }

    if(Keyboard->ActionLeft.Pressed)
    {
        GameCamera->LookAt.x -= 2.0f;
    }

    {//Render
        RenderCmds.Init();

        RenderCmds.ClearScreen();

        Player* Fighters[2] = {&GameState->Fighter1, &GameState->Fighter2};

        {//Draw Level Background
            Coordinate_Space BackgroundWorldSpace{};
            Coordinate_Space BackgroundCameraSpace{};
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
            for(int32 FighterIndex = 0; FighterIndex < ArrayCount(Fighters); ++FighterIndex)
            {
                Coordinate_Space FighterWorldSpace{};
                Coordinate_Space FighterCameraSpace{};
                Drawable_Rect FighterRect{};

                FighterWorldSpace.Origin = Fighters[FighterIndex]->WorldPos;

                { //Transform to Camera Space
                    vec2 TranslationToCameraSpace = GameCamera->ViewCenter - GameCamera->LookAt;
                    FighterCameraSpace.Origin = FighterWorldSpace.Origin + TranslationToCameraSpace;
                };

                FighterRect = ProduceRectFromBottomLeftPoint(
                                            FighterCameraSpace.Origin, 
                                            Fighters[FighterIndex]->Size.Width, 
                                            Fighters[FighterIndex]->Size.Height);

                FighterRect = DilateAboutPoint(GameCamera->DilatePoint, GameCamera->ZoomFactor, FighterRect);

                RenderCmds.DrawTexture(Fighters[FighterIndex]->CurrentTexture.ID, FighterRect, vec2{0.0f, 0.0f}, vec2{1.0f, 1.0f});
            };
        };

    }
}

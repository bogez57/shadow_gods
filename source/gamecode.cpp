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

extern "C" void
GameUpdate(Game_Memory* GameMemory, Platform_Services PlatformServices, Game_Render_Cmds RenderCmds, 
                    Game_Sound_Output_Buffer* SoundOutput, const Game_Input* GameInput)
{
    Game_State* GameState = (Game_State*)GameMemory->PermanentStorage;

    const Game_Controller* Keyboard = &GameInput->Controllers[0];
    const Game_Controller* GamePad = &GameInput->Controllers[1];

    if(!GameMemory->IsInitialized)
    {
        GameMemory->IsInitialized = true;
        ViewportWidth = 1280.0f;
        ViewportHeight = 720.0f;

        RenderCmds.Init();

        {//Init Game State
            GameState->GameLevel.BackgroundTexture.ImageData = PlatformServices.LoadRGBAImage(
                                                                                "1440p.jpg", 
                                                                                &GameState->GameLevel.BackgroundTexture.Width,
                                                                                &GameState->GameLevel.BackgroundTexture.Height);
            GameState->Fighter.CurrentTexture.ImageData = PlatformServices.LoadRGBAImage(
                                                                                "Fighter.jpg", 
                                                                                &GameState->Fighter.CurrentTexture.Width,
                                                                                &GameState->Fighter.CurrentTexture.Height);

            GameState->GameLevel.BackgroundTexture.ID = RenderCmds.LoadTexture(GameState->GameLevel.BackgroundTexture);
            GameState->Fighter.CurrentTexture.ID = RenderCmds.LoadTexture(GameState->Fighter.CurrentTexture);

            GameState->GameLevel.Width = (float32)GameState->GameLevel.BackgroundTexture.Width;
            GameState->GameLevel.Height = (float32)GameState->GameLevel.BackgroundTexture.Height;
            GameState->GameLevel.CenterPoint = {GameState->GameLevel.Width / 2, GameState->GameLevel.Height / 2};

            GameState->GameCamera.WorldPos = {GameState->GameLevel.Width / 2, GameState->GameLevel.Height / 2};
            GameState->GameCamera.ViewWidth = ViewportWidth;
            GameState->GameCamera.ViewHeight = ViewportHeight;
            GameState->GameCamera.ViewCenter = {GameState->GameCamera.ViewWidth / 2, GameState->GameCamera.ViewHeight / 2,};

            GameState->Fighter.WorldPos = {GameState->GameLevel.Width * .45f, GameState->GameLevel.Height * .45f};
            GameState->Fighter.Width = 100.0f;
            GameState->Fighter.Height = 200.0f;
        };
    }

    Camera* GameCamera = &GameState->GameCamera;
    Player* Fighter = &GameState->Fighter;
    Level* GameLevel = &GameState->GameLevel;

    GameCamera->ZoomFactor = {0.0f};

    RenderCmds.ClearScreen();

    if(Keyboard->MoveUp.Pressed)
    {
        Fighter->WorldPos.y += 1.0f;
    }

    if(Keyboard->MoveDown.Pressed)
    {
        Fighter->WorldPos.y -= 1.0f;
    }

    if(Keyboard->MoveRight.Pressed)
    {
        Fighter->WorldPos.x += 5.0f;
    }

    if(Keyboard->MoveLeft.Pressed)
    {
        Fighter->WorldPos.x -= 5.0f;
    }

    if(Keyboard->ActionUp.Pressed)
    {
        GameCamera->WorldPos.y += 2.0f;
    }

    if(Keyboard->ActionDown.Pressed)
    {
        GameCamera->WorldPos.y -= 2.0f;
    }

    if(Keyboard->ActionRight.Pressed)
    {
        GameCamera->WorldPos.x += 2.0f;
    }

    if(Keyboard->ActionLeft.Pressed)
    {
        GameCamera->WorldPos.x -= 2.0f;
    }

    {//Render

        {//Draw Level Background
            Rect CameraWorldCoords = ProduceRectFromCenterPoint(GameCamera->WorldPos, GameCamera->ViewWidth, GameCamera->ViewHeight);
            vec2 MinDisplayUV{CameraWorldCoords.MinPoint.x / GameLevel->Width, CameraWorldCoords.MinPoint.y / GameLevel->Height};
            vec2 MaxDisplayUV{CameraWorldCoords.MaxPoint.x / GameLevel->Width, CameraWorldCoords.MaxPoint.y / GameLevel->Height};

            Rect CameraViewCoords = ProduceRectFromCenterPoint(GameCamera->ViewCenter, GameCamera->ViewWidth, GameCamera->ViewHeight);

            RenderCmds.DrawTexture(GameLevel->BackgroundTexture.ID, CameraViewCoords, MinDisplayUV, MaxDisplayUV);
        };

        {//Draw Player
            vec2 FighterRelativeDistanceFromCamera{};
            vec2 FighterViewSpacePosition{};

            {//Convert Player world position to camera space position
                FighterRelativeDistanceFromCamera = {GameCamera->WorldPos - Fighter->WorldPos};

                FighterViewSpacePosition = {AbsoluteVal(FighterRelativeDistanceFromCamera.x - (GameCamera->ViewWidth / 2)),
                                            AbsoluteVal(FighterRelativeDistanceFromCamera.y - (GameCamera->ViewHeight / 2))};
            };

            Rect FighterViewSpacePos = ProduceRectFromCenterPoint(FighterViewSpacePosition, Fighter->Width, Fighter->Height);

            RenderCmds.DrawRect(FighterViewSpacePos.MinPoint, FighterViewSpacePos.MaxPoint);

            vec2 MinFighterTextureUV{0.0f, 0.0f};
            vec2 MaxFighterTextureUV{1.0f, 1.0f};
            RenderCmds.DrawTexture(Fighter->CurrentTexture.ID, FighterViewSpacePos, MinFighterTextureUV, MaxFighterTextureUV);
        };
    };
}

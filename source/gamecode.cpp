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
                                                                                "4k.jpg", 
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

            GameState->GameCamera.FocusPoint = {GameState->GameLevel.Width / 2, GameState->GameLevel.Height / 2};
            GameState->GameCamera.ViewWidth = ViewportWidth;
            GameState->GameCamera.ViewHeight = ViewportHeight;

            GameState->Fighter.WorldPos = {GameState->GameLevel.Width * .45f, GameState->GameLevel.Height * .45f};
            GameState->Fighter.Width = 100.0f;
            GameState->Fighter.Height = 200.0f;
        };
    }

    Camera* GameCamera = &GameState->GameCamera;
    Player* Fighter = &GameState->Fighter;
    Level* GameLevel = &GameState->GameLevel;
    Texture* BackgroundTexture = &GameState->GameLevel.BackgroundTexture;

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
        GameCamera->FocusPoint.y += 2.0f;
    }

    if(Keyboard->ActionDown.Pressed)
    {
        GameCamera->FocusPoint.y -= 2.0f;
    }

    {//Render

        {//Draw Player

            vec2 FighterRelativeDistanceFromCamera{};
            vec2 FighterCameraSpacePosition{};

            { //Convert Player world position to camera space position
                FighterRelativeDistanceFromCamera = {GameCamera->FocusPoint - Fighter->WorldPos};

                FighterCameraSpacePosition = {AbsoluteVal(FighterRelativeDistanceFromCamera.x - (GameCamera->ViewWidth / 2)),
                                              AbsoluteVal(FighterRelativeDistanceFromCamera.y - (GameCamera->ViewHeight / 2))};
            };

            vec2 FighterMinPoint = {FighterCameraSpacePosition.x - (Fighter->Width / 2), 
                                    FighterCameraSpacePosition.y - (Fighter->Height / 2)};
            vec2 FighterMaxPoint = {FighterCameraSpacePosition.x + (Fighter->Width / 2), 
                                    FighterCameraSpacePosition.y + (Fighter->Height / 2)};

            RenderCmds.DrawRect(FighterMinPoint, FighterMaxPoint);
        };
    };
}

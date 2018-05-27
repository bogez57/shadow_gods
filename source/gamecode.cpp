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
                                                                                "Halloween.jpg", 
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

            GameState->Fighter.Position = {200.0f, 200.0f};
        };
    }

    Camera* GameCamera = &GameState->GameCamera;
    Player* Fighter = &GameState->Fighter;
    Texture* BackgroundTexture = &GameState->GameLevel.BackgroundTexture;

    GameCamera->ZoomFactor = {0.0f};

    RenderCmds.ClearScreen();

    if(Keyboard->MoveUp.Pressed)
    {
        Fighter->Position.Y += 5.0f;
    }

    if(Keyboard->MoveDown.Pressed)
    {
        Fighter->Position.Y -= 5.0f;
    }

    if(Keyboard->MoveRight.Pressed)
    {
        Fighter->Position.X += 5.0f;
    }

    if(Keyboard->MoveLeft.Pressed)
    {
        Fighter->Position.X -= 5.0f;
    }

    if(Keyboard->ActionUp.Pressed)
    {
        GameCamera->FocusPoint.X += 3.0f;
    }

    if(Keyboard->ActionDown.Pressed)
    {
        GameCamera->FocusPoint.Y += 3.0f;
    }

    RenderCmds.DrawBackground(*BackgroundTexture, GameCamera->FocusPoint, vec2{GameCamera->ViewWidth, GameCamera->ViewHeight});

    {//Draw Player
        vec2 FighterPos = Fighter->Position;
        vec3 FighterColor{1.0f, 1.0f, 1.0f};

        float32 FighterWidth{100.0f};
        float32 FighterHeight{200.0f};

        RenderCmds.DrawTexture(Fighter->CurrentTexture, FighterPos, FighterWidth, FighterHeight);
    }
}
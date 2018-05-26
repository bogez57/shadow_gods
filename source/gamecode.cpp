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

        RenderCmds.Init();

        {//Init Game State
            GameState->GameCamera.Position = {GameState->GameLevel.Width/ 2, GameState->GameLevel.Height/ 2};
            GameState->Fighter.Position = {100.0f, 100.0f};

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
    }

    if(Keyboard->ActionDown.Pressed)
    {
    }

    RenderCmds.DrawTexture(*BackgroundTexture, vec2{0.0f, 0.0f}, (float32)BackgroundTexture->Width, (float32)BackgroundTexture->Height);

    {//Draw Player
        vec2 FighterPos = Fighter->Position;
        vec3 FighterColor{1.0f, 1.0f, 1.0f};

        float32 FighterWidth{100.0f};
        float32 FighterHeight{200.0f};

        RenderCmds.DrawRect(vec2{FighterPos.X, FighterPos.Y}, vec2{FighterPos.X + FighterWidth, FighterPos.Y},
                            vec2{FighterPos.X + FighterWidth, FighterPos.Y + FighterHeight},
                            vec2{FighterPos.X, FighterPos.Y + FighterHeight}, FighterColor);
        RenderCmds.DrawTexture(Fighter->CurrentTexture, FighterPos, FighterWidth, FighterHeight);
    }
}
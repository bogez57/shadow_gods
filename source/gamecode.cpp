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

    Camera* GameCamera = &GameState->GameCamera;
    Player* Fighter = &GameState->Fighter;
    Level* GameLevel = &GameState->GameLevel;

    const Game_Controller* Keyboard = &GameInput->Controllers[0];
    const Game_Controller* GamePad = &GameInput->Controllers[1];

    if(!GameMemory->IsInitialized)
    {
        GameMemory->IsInitialized = true;
        ViewportWidth = 1280.0f;
        ViewportHeight = 720.0f;

        RenderCmds.Init();

        {//Init Game State
            GameLevel->BackgroundTexture.ImageData = PlatformServices.LoadRGBAImage(
                                                                            "1440p.jpg", 
                                                                            &GameLevel->BackgroundTexture.Width,
                                                                            &GameLevel->BackgroundTexture.Height);
            GameLevel->BackgroundTexture.ID = RenderCmds.LoadTexture(GameLevel->BackgroundTexture);
            GameLevel->Width = (float32)GameLevel->BackgroundTexture.Width;
            GameLevel->Height = (float32)GameLevel->BackgroundTexture.Height;
            GameLevel->CenterPoint = {GameLevel->Width / 2, GameLevel->Height / 2};

            Fighter->CurrentTexture.ImageData = PlatformServices.LoadRGBAImage(
                                                                        "Fighter.jpg", 
                                                                        &Fighter->CurrentTexture.Width,
                                                                        &Fighter->CurrentTexture.Height);
            Fighter->CurrentTexture.ID = RenderCmds.LoadTexture(Fighter->CurrentTexture);
            Fighter->WorldPos = {200.0f, 300.0f};
            Fighter->Size.Width = 100.0f;
            Fighter->Size.Height = 200.0f;

            GameCamera->ViewWidth = ViewportWidth;
            GameCamera->ViewHeight = ViewportHeight;
            GameCamera->LookAt = {GameLevel->Width/2.0f, GameLevel->Height/2.0f};
            GameCamera->ViewCenter = {GameCamera->ViewWidth/2.0f, GameCamera->ViewHeight/2.0f};
            GameCamera->DilatePoint = GameCamera->ViewCenter;
            GameCamera->ZoomFactor = 1.0f;
        };
    }

    RenderCmds.ClearScreen();

    if(Keyboard->MoveUp.Pressed)
    {
        Fighter->WorldPos.y += 5.0f;
    }

    if(Keyboard->MoveDown.Pressed)
    {
        Fighter->WorldPos.y -= 5.0f;
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

    RenderCmds.TestArena(GameState);
}

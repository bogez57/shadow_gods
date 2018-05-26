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

global_variable float32 WindowWidth = 1280.0f;
global_variable float32 WindowHeight = 720.0f;

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

        GameState->Fighter.Origin = {100.0f, 100.0f};
        GameState->GameCamera.Position = {WindowWidth/2, WindowHeight/2};
        GameState->MoveWidth = WindowWidth;
        GameState->MoveHeight = WindowHeight;

        RenderCmds.Init();

        int ImageWidth{};
        int ImageHeight{};
        GameState->BackgroundTexture.ImageData = PlatformServices.LoadRGBAImage("Halloween.jpg", &ImageWidth, &ImageHeight);
        GameState->BackgroundTexture.Width = ImageWidth;
        GameState->BackgroundTexture.Height = ImageHeight;

        GameState->BackgroundTexture.ID = RenderCmds.LoadTexture(GameState->BackgroundTexture);
    }
    
    Player* Fighter = &GameState->Fighter;
    Camera* GameCamera = &GameState->GameCamera;
    Texture* BackgroundTexture = &GameState->BackgroundTexture;
    GameCamera->ZoomFactor = {0.0f};

    RenderCmds.ClearScreen();

    if(Keyboard->MoveUp.Pressed)
    {
        Fighter->Origin.Y += 3.0f;
    }

    if(Keyboard->MoveDown.Pressed)
    {
        Fighter->Origin.Y -= 3.0f;
    }

    if(Keyboard->MoveRight.Pressed)
    {
        Fighter->Origin.X += 3.0f;
    }

    if(Keyboard->MoveLeft.Pressed)
    {
        Fighter->Origin.X -= 3.0f;
    }

    if(Keyboard->ActionUp.Pressed)
    {
        GameCamera->ZoomFactor -= 1.4f;
    }

    if(Keyboard->ActionDown.Pressed)
    {
        GameCamera->ZoomFactor += 1.4f;
    }

    RenderCmds.DrawTexture(*BackgroundTexture, (GameState->MoveWidth += GameCamera->ZoomFactor * 2), (GameState->MoveHeight += GameCamera->ZoomFactor));

    {//Draw Player
        vec2 PlayerBL{Fighter->Origin.X + 100.0f, Fighter->Origin.Y + 0.0f};
        vec2 PlayerBR{Fighter->Origin.X + 200.0f, Fighter->Origin.Y + 0.0f};
        vec2 PlayerTR{Fighter->Origin.X + 200.0f, Fighter->Origin.Y + 100.0f};
        vec2 PlayerTL{Fighter->Origin.X + 100.0f, Fighter->Origin.Y + 100.0f};
        vec3 PlayerColor{1.0f, 1.0f, 1.0f};

        PlayerBL.X -= GameCamera->ZoomFactor;
        PlayerBR.X += GameCamera->ZoomFactor;
        PlayerTR.X += GameCamera->ZoomFactor;
        PlayerTL.X -= GameCamera->ZoomFactor;

        PlayerTR.Y += (GameCamera->ZoomFactor * 2); 
        PlayerTL.Y += (GameCamera->ZoomFactor * 2); 

        RenderCmds.DrawRect(PlayerBL, PlayerBR, PlayerTR, PlayerTL, PlayerColor);
    }
}
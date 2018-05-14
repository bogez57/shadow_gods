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

        GameState->BL = {100.0f, 100.0f};
        GameState->BR = {500.0f, 100.0f};
        GameState->TR = {500.0f, 500.0f};
        GameState->TL = {100.0f, 200.0f};
        GameState->Color = {0.4f, 0.1f, 0.3f};
    }

    RenderCmds.ClearScreen();

    if (Keyboard->MoveUp.Pressed)
    {
        GameState->TR.X += 1;
    }

    RenderCmds.DrawQuad(GameState->BL, GameState->BR, GameState->TR, GameState->TL, GameState->Color);
}
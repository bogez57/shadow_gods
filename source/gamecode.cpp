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

        GameState->Floor.Y = 500.0f;
    }

    vec2 BRBottomLeft{0.0f, 0.0f};
    vec2 BRBottomRight{WindowWidth, 0.0f};
    vec2 BRTopRight{WindowWidth, WindowHeight};
    vec2 BRTopLeft{0.0f, WindowHeight};
    vec3 BRColor{0.4f, 0.4f, 0.4f};

    if(Keyboard->MoveUp.Pressed)
    {
        GameState->Floor.Y += 4.0f;
    }

    if(Keyboard->MoveDown.Pressed)
    {
        GameState->Floor.Y -= 4.0f;
    }

    RenderCmds.ClearScreen();

    {//Draw Background
        RenderCmds.DrawRect(BRBottomLeft, BRBottomRight, BRTopRight, BRTopLeft, BRColor);

        BRTopRight.Y = GameState->Floor.Y;
        BRTopLeft.Y = GameState->Floor.Y;
        BRColor.R = 0.1f;
        BRColor.G = 0.0f;
        BRColor.B = 0.8f;

        RenderCmds.DrawRect(BRBottomLeft, BRBottomRight, BRTopRight, BRTopLeft, BRColor);
    }
}
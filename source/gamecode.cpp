#if (DEVELOPMENT_BUILD)
    #define BGZ_LOGGING_ON true
    #define BGZ_ERRHANDLING_ON true
#else
    #define BGZ_LOGGING_ON false
    #define BGZ_ERRHANDLING_ON false
#endif

#include <boagz/error_handling.h>
#include <boagz/error_context.cpp>

#include "shared.h"
#include "math.h"

extern "C" void
GameUpdate(Game_Memory* GameMemory, Platform_Services PlatformServices, Game_Render_Cmd_Buffer* RenderCmds, 
                    Game_Sound_Output_Buffer* SoundOutput, Game_Input* GameInput)
{
    Game_Controller* Keyboard = &GameInput->Controllers[0];
    Game_Controller* GamePad = &GameInput->Controllers[1];

    if(GamePad->LThumbStick.X > .2f)
    {
        BGZ_CONSOLE("ThmbsssstiickX\n");
    }

    if(GamePad->LThumbStick.X < -.1f)
    {
        BGZ_CONSOLE("Thmbsssstiick-X\n");
    }

    if(Keyboard->MoveUp.Pressed && Keyboard->MoveUp.NumTransitionsPerFrame > 0)
    {
        BGZ_CONSOLE("Keyboard pressed!");
    }

    if(GamePad->MoveUp.Pressed && GamePad->MoveUp.NumTransitionsPerFrame > 0)
    {
        BGZ_CONSOLE("Character Moves up\n");
    }

    if(GamePad->ActionUp.Pressed)
    {
        BGZ_CONSOLE("Character PunchesQ\n");
    }

    if(GamePad->LeftShoulder.Pressed)
    {
        BGZ_CONSOLE("Character vomits\n");
    }
}
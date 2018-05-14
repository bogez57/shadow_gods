#pragma once
#include <Windows.h>
#include "shared.h"

namespace Win32
{
    struct Game_Code
    {
        HMODULE DLLHandle{};
        void (*UpdateFunc)(Game_Memory*, Platform_Services, Game_Render_Cmds, Game_Sound_Output_Buffer*, const Game_Input* const);
        FILETIME PreviousDLLWriteTime{};
    };
}
using GameUpdateFuncPtr = void(*)(Game_Memory*, Platform_Services, Game_Render_Cmds, Game_Sound_Output_Buffer*, const Game_Input* const);


namespace Win32::Dbg
{
    struct Game_Replay_State
    {
        bool InputRecording{false};
        bool InputPlayBack{false};
    };
}
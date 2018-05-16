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
    const uint32 MaxAllowableRecordedInputs{4000};
    struct Game_Replay_State
    {
        Game_Input* RecordedInputs{nullptr};
        uint32 InputCount{};
        uint32 MaxInputStructsRecorded{};
        void* OriginalRecordedGameState{nullptr};

        bool InputRecording{false};
        bool InputPlayBack{false};
    };
}
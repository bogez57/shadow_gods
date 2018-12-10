#pragma once
#include <Windows.h>
#include "shared.h"

namespace Win32
{
    struct Game_Code
    {
        HMODULE DLLHandle {};
        void (*UpdateFunc)(Application_Memory*, Platform_Services*, Game_Render_Cmds, Game_Sound_Output_Buffer*, Game_Input*);
        FILETIME PreviousDLLWriteTime {};
    };
} // namespace Win32

using GameUpdateFuncPtr = void (*)(Application_Memory*, Platform_Services*, Game_Render_Cmds, Game_Sound_Output_Buffer*, Game_Input*);

namespace Win32::Dbg
{
    const ui32 MaxAllowableRecordedInputs { 4000 };
    struct Game_Replay_State
    {
        Game_Input* RecordedInputs { nullptr };
        ui32 InputCount {};
        ui32 TotalInputStructsRecorded {};
        void* OriginalRecordedGameState { nullptr };

        bool InputRecording { false };
        bool InputPlayBack { false };
    };
} // namespace Win32::Dbg
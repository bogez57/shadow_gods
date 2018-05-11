#pragma once
#include <Windows.h>
#include "shared.h"

namespace Win32
{
    struct Game_Code
    {
        HMODULE DLLHandle{};
        void (*UpdateFunc)(Game_Memory*, Platform_Services, Game_Render_Cmds, Game_Sound_Output_Buffer*, Game_Input*);
        FILETIME PreviousDLLWriteTime{};
    };
}
using GameUpdateFuncPtr = void(*)(Game_Memory*, Platform_Services, Game_Render_Cmds, Game_Sound_Output_Buffer*, Game_Input*);


namespace Win32::Dbg
{
    struct Game_Replay_State
    {
        HANDLE GameStateFile{};
        void* GameStateDataForReplay{nullptr};
        void* GameMemoryBlock{nullptr};
        uint64 TotalGameMemorySize{};

        HANDLE InputFile{nullptr};
        bool InputRecording{false};
        HANDLE InputPlayBackFile{nullptr};
        bool InputPlayBack{false};
    };
}
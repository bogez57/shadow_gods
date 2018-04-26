#pragma once
#include <Windows.h>
#include "shared.h"

struct Game_Code
{
    HMODULE DLLHandle{};
    void (*UpdateFunc)(Game_Memory*, Platform_Services, Game_Render_Cmd_Buffer*, Game_Sound_Output_Buffer*, Game_Input*);
    FILETIME PreviousDLLWriteTime{};
};

using GameUpdateFuncPtr = void(*)(Game_Memory*, Platform_Services, Game_Render_Cmd_Buffer*, Game_Sound_Output_Buffer*, Game_Input*);
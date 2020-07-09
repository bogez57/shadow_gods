#pragma once
#include <Windows.h>
#include "shared.h"

namespace Win32
{
    struct Offscreen_Buffer
    {
        //Pixels are alwasy 32-bits wide, Memory Order BB GG RR XX
        BITMAPINFO Info;
        void *memory;
        int width;
        int height;
        int pitch;
        int bytesPerPixel;
    };
    
    struct Game_Code
    {
        HMODULE DLLHandle {};
        void (*UpdateFunc)(Application_Memory*, Platform_Services*, Rendering_Info*, Game_Sound_Output_Buffer*, Game_Input*);
        void (*DebugFrameEnd)(Application_Memory* debugMemory);
        FILETIME PreviousDLLWriteTime {};
    };
    
    struct Window_Dimension
    {
        int width;
        int height;
    };
} // namespace Win32

using GameUpdateFuncPtr = void (*)(Application_Memory*, Platform_Services*, Rendering_Info*, Game_Sound_Output_Buffer*, Game_Input*);
using GameDebugFrameEndFuncPtr = void (*)(Application_Memory* debugMemory);

namespace Win32::Dbg
{
    const s32 MaxAllowableRecordedInputs { 4000 };
    struct Game_Replay_State
    {
        Game_Input* RecordedInputs { nullptr };
        s32 InputCount {};
        s32 TotalInputStructsRecorded {};
        void* OriginalRecordedGameState { nullptr };
        
        bool  InputRecording { false };
        bool  InputPlayBack { false };
    };
} // namespace Win32::Dbg
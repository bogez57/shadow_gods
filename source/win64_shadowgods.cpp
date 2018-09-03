/*

    ToDo List:

    - Load XInput through LoadLibrary() (To help avoid likely "Xinput.dll not found on certain windows platforms")
    - Make current frame timing more accurate/in-sync with monitor refresh rate
    - Pass deltatime to game
    - Fix dll reloading which currently free's and reload's game code twice upon building. I believe this is because
    when comparing file times between old and new dll write times the game tends to update fast enough that the times
    are still the same upon the second run through of the loop since the last run through. 
*/

#if (DEVELOPMENT_BUILD)

    #define BGZ_LOGGING_ON true
#else

    #define BGZ_LOGGING_ON false
#endif

#include <Windows.h>
#include <xinput.h>
#include <io.h> 
#include <fcntl.h> 
#include <stdio.h>
#include <GL/glew.h>
#include <GL/wglew.h>
#include <gl/gl.h>
#include <boagz/timing.h>
#include <boagz/error_handling.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include "atomic_types.h"
#include "math.h"
#include "utilities.h"
#include "win64_shadowgods.h"
#include "shared.h"

global_variable ui32 WindowWidth{1280};
global_variable ui32 WindowHeight{720};
global_variable Game_Memory GameMemory{};
global_variable bool GameRunning{};

namespace Win32::Dbg
{
    local_func auto
    LogErr(const char *ErrMessage) -> void
    {
        BGZ_CONSOLE("Windows error code: %d\n", GetLastError());
        BGZ_CONSOLE(ErrMessage);
    };

    local_func auto
    Malloc(sizet Size) -> void*
    {
        void* Result{};

        Result = malloc(Size);

        return Result;
    };

    local_func auto
    Calloc(sizet Count, sizet Size) -> void*
    {
        void* Result{};

        Result = calloc(Count, Size);

        return Result;
    };

    local_func auto
    Free(void* PtrToFree) -> void
    {
        free(PtrToFree);
    };

    local_func auto
    FreeFileMemory(void *FileMemory)
    {
        if (FileMemory)
        {
            VirtualFree(FileMemory, 0, MEM_RELEASE);
        }
    };

    local_func auto
    WriteEntireFile(const char *FileName, void *Memory, ui32 MemorySize) -> bool
    {
        b32 Result = false;

        HANDLE FileHandle = CreateFile(FileName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
        if (FileHandle != INVALID_HANDLE_VALUE)
        {
            DWORD BytesWritten;
            if (WriteFile(FileHandle, Memory, MemorySize, &BytesWritten, 0))
            {
                //File read successfully
                Result = (BytesWritten == MemorySize);
            }
            else
            {
                Win32::Dbg::LogErr("Unable to write to file!");
                InvalidCodePath;
            }

            CloseHandle(FileHandle);
        }
        else
        {
            Win32::Dbg::LogErr("Unable to create file handle!");
            InvalidCodePath;
        }

        return (Result);
    };

    local_func auto
    ReadEntireFile(const char *FileName) -> Read_File_Result
    {
        Read_File_Result Result{};

        HANDLE FileHandle = CreateFile(FileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

        if (FileHandle != INVALID_HANDLE_VALUE)
        {
            LARGE_INTEGER FileSize{};
            if (GetFileSizeEx(FileHandle, &FileSize))
            {
                ui32 FileSize32 = SafeTruncateUInt64(FileSize.QuadPart);
                Result.FileContents = VirtualAlloc(0, FileSize32, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

                if (Result.FileContents)
                {
                    DWORD BytesRead;
                    if (ReadFile(FileHandle, Result.FileContents, FileSize32, &BytesRead, 0) &&
                        (FileSize32 == BytesRead))
                    {
                        //File read successfully
                        Result.FileSize = FileSize32;
                    }
                    else
                    {
                        FreeFileMemory(Result.FileContents);
                        Result.FileContents = 0;
                    }
                }
                else
                {
                    Win32::Dbg::LogErr("Unable to allocate memory for read file!");
                    InvalidCodePath;
                }
            }
            else
            {
                Win32::Dbg::LogErr("Unable to get size of the file!");
                InvalidCodePath;
            }
        }
        else
        {
            Win32::Dbg::LogErr("Unable to get file handle!");
            InvalidCodePath;
        }

        return Result;
    };

    local_func auto
    ReadFileOfLength(ui32* length, const char* FilePath) -> char*
    {
        char *data;
        FILE *file;
        errno_t err;

        if ((err = fopen_s(&file, FilePath, "rb")) != 0)
        {
            BGZ_ASSERT(1 == 0);
        };

        fseek(file, 0, SEEK_END);
        *length = (i32)ftell(file);
        fseek(file, 0, SEEK_SET);

        data = (char*)malloc(*length);
        fread(data, 1, *length, file);
        fclose(file);

        return data;
    };

    local_func auto
    LoadRGBAImage(const char* ImagePath, int* Width, int* Height) -> unsigned char*
    {
        int DesiredChannels = 4;
        int NumOfLoadedChannels{};
        unsigned char* ImageData = stbi_load(ImagePath, Width, Height, &NumOfLoadedChannels, DesiredChannels);
        BGZ_ASSERT(ImageData);

        return ImageData;
    }

    local_func auto
    UseConsole() -> void
    {
        //Create a console for this application
        AllocConsole();

        // Get STDOUT handle
        HANDLE ConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
        int SystemOutput = _open_osfhandle(intptr_t(ConsoleOutput), _O_TEXT);
        FILE *COutputHandle = _fdopen(SystemOutput, "w");

        // Get STDERR handle
        HANDLE ConsoleError = GetStdHandle(STD_ERROR_HANDLE);
        int SystemError = _open_osfhandle(intptr_t(ConsoleError), _O_TEXT);
        FILE *CErrorHandle = _fdopen(SystemError, "w");

        // Get STDIN handle
        HANDLE ConsoleInput = GetStdHandle(STD_INPUT_HANDLE);
        int SystemInput = _open_osfhandle(intptr_t(ConsoleInput), _O_TEXT);
        FILE *CInputHandle = _fdopen(SystemInput, "r");

        // Redirect the CRT standard input, output, and error handles to the console
        freopen_s(&CInputHandle, "CONIN$", "r", stdin);
        freopen_s(&COutputHandle, "CONOUT$", "w", stdout);
        freopen_s(&CErrorHandle, "CONOUT$", "w", stderr);
    };

    inline auto
    GetFileTime(const char *FileName) -> FILETIME
    {
        FILETIME TimeFileWasLastWrittenTo{};
        WIN32_FILE_ATTRIBUTE_DATA FileData{};

        if (GetFileAttributesEx(FileName, GetFileExInfoStandard, &FileData))
        {
            TimeFileWasLastWrittenTo = FileData.ftLastWriteTime;
        }

        return TimeFileWasLastWrittenTo;
    }

    local_func auto
    LoadGameCodeDLL(const char *GameCodeDLL) -> Game_Code
    {
        Game_Code GameCode{};
        const char *GameCodeTempDLL = "build/game_temp.dll";

        GameCode.PreviousDLLWriteTime = GetFileTime(GameCodeDLL);

        //Copy app code dll file to a temp file and load that temp file so that original app code dll can be written to while exe
        //is running. This is For live editing purposes. Code is currently being looped because the modified source dll that gets compiled
        //apparently isn't unlocked by Windows in time for it to be copied upon the first few CopyFile() function calls.
        bool CopyFileFuncNotWorking{true};
        ui32 Counter{};
        ui32 MaxAllowedLoops{5000};

        while (CopyFileFuncNotWorking)
        {
            if (CopyFile(GameCodeDLL, GameCodeTempDLL, FALSE) != 0)
            {
                GameCode.DLLHandle = LoadLibrary(GameCodeTempDLL);

                if (GameCode.DLLHandle)
                {
                    GameCode.UpdateFunc = (GameUpdateFuncPtr)GetProcAddress(GameCode.DLLHandle, "GameUpdate");
                    CopyFileFuncNotWorking = false;

                    if (!GameCode.UpdateFunc)
                    {
                        InvalidCodePath;
                    }
                }
                else
                {
                    InvalidCodePath;
                }
            };

            ++Counter;

            if (Counter == MaxAllowedLoops)
            {
                //DLL took too long to be unlocked by windows so breaking exe to avoid possible infinite loop
                InvalidCodePath;
            }
        };

        return GameCode;
    };

    local_func auto
    FreeGameCodeDLL(Game_Code *GameCode, Platform_Services* platformServices) -> void
    {
        if (GameCode->DLLHandle != INVALID_HANDLE_VALUE)
        {
            FreeLibrary(GameCode->DLLHandle);
            GameCode->DLLHandle = 0;
            GameCode->UpdateFunc = nullptr;
            platformServices->DLLJustReloaded = true;
        };
    };

    local_func auto
    InitInputRecording(Win32::Dbg::Game_Replay_State *GameReplayState) -> void
    {
        GameReplayState->InputRecording = true;
        GameReplayState->TotalInputStructsRecorded = 0;
        GameReplayState->InputCount = 0;
        memcpy(GameReplayState->OriginalRecordedGameState, GameMemory.PermanentStorage, GameMemory.TotalSize);
    };

    local_func auto
    RecordInput(Game_Input* Input, Win32::Dbg::Game_Replay_State* GameReplayState) -> void
    {
        GameReplayState->RecordedInputs[GameReplayState->InputCount] = *Input;
        ++GameReplayState->InputCount;
        ++GameReplayState->TotalInputStructsRecorded;
    };

    local_func auto
    InitInputPlayBack(Win32::Dbg::Game_Replay_State *GameReplayState) -> void
    {
        GameReplayState->InputPlayBack = true;
        GameReplayState->InputCount = 0;
        //Set game state back to when it was first recorded for proper looping playback
        memcpy(GameMemory.PermanentStorage, GameReplayState->OriginalRecordedGameState, GameMemory.TotalSize);
    }

    local_func auto
    EndInputPlayBack(Game_Input* Input, Win32::Dbg::Game_Replay_State* GameReplayState) -> void
    {
        GameReplayState->InputPlayBack = false;
        for (ui32 ControllerIndex{0}; ControllerIndex < ArrayCount(Input->Controllers); ++ControllerIndex)
        {
            for (ui32 ButtonIndex{0}; ButtonIndex < ArrayCount(Input->Controllers[ControllerIndex].Buttons); ++ButtonIndex)
            {
                Input->Controllers[ControllerIndex].Buttons[ButtonIndex].Pressed = false;
            }
        }
    };

    local_func auto
    PlayBackInput(Game_Input *Input, Win32::Dbg::Game_Replay_State *GameReplayState) -> void
    {
        if (GameReplayState->InputCount < GameReplayState->TotalInputStructsRecorded)
        {
            *Input = GameReplayState->RecordedInputs[GameReplayState->InputCount];
            ++GameReplayState->InputCount;
        }
        else
        {
            GameReplayState->InputCount = 0;
            memcpy(GameMemory.PermanentStorage, GameReplayState->OriginalRecordedGameState, GameMemory.TotalSize);
        }
    }
}   

namespace Win32
{
    local_func auto
    ProcessKeyboardMessage(Button_State* NewState, b32 IsDown) -> void
    {

        if (NewState->Pressed != IsDown)
        {
            NewState->Pressed = IsDown;
            ++NewState->NumTransitionsPerFrame;
        }
    }

    local_func auto 
    ProcessPendingMessages(Game_Input* Input, Win32::Dbg::Game_Replay_State* GameReplayState) -> void
    {
        MSG Message;
        while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
        {
            switch(Message.message)
            {
                case WM_QUIT:
                {
                    GameRunning = false;
                } break;
            
                case WM_SYSKEYDOWN:
                case WM_SYSKEYUP:
                case WM_KEYDOWN:
                case WM_KEYUP:
                {
                    ui32 VKCode = (ui32)Message.wParam;

                    Game_Controller* Keyboard = &Input->Controllers[0];

                    //Since we are comparing IsDown and WasDown below, we need to use == and != to convert these bit tests to 
                    //actual 0 or 1 values.
                    b32 WasDown = ((Message.lParam & (1 << 30)) != 0);
                    b32 IsDown = ((Message.lParam & (1 << 31)) == 0);

                    if (WasDown != IsDown) //Filter out key repeats as my current input scheme is already able to react to held down inputs properly
                    {
                        if (VKCode == 'W')
                        {
                            Win32::ProcessKeyboardMessage(&Keyboard->MoveUp, IsDown);
                        }
                        else if (VKCode == 'S')
                        {
                            Win32::ProcessKeyboardMessage(&Keyboard->MoveDown, IsDown);
                        }
                        else if (VKCode == 'A')
                        {
                            Win32::ProcessKeyboardMessage(&Keyboard->MoveLeft, IsDown);
                        }
                        else if (VKCode == 'D')
                        {
                            Win32::ProcessKeyboardMessage(&Keyboard->MoveRight, IsDown);
                        }
                        else if (VKCode == 'Q')
                        {
                            Win32::ProcessKeyboardMessage(&Keyboard->ActionUp, IsDown);
                        }
                        else if (VKCode == 'Z')
                        {
                            Win32::ProcessKeyboardMessage(&Keyboard->ActionDown, IsDown);
                        }
                        else if (VKCode == 'C')
                        {
                            Win32::ProcessKeyboardMessage(&Keyboard->ActionRight, IsDown);
                        }
                        else if (VKCode == 'X')
                        {
                            Win32::ProcessKeyboardMessage(&Keyboard->ActionLeft, IsDown);
                        }
                        else if (VKCode == 'R')
                        {
                            if (IsDown)
                            {
                                if (!GameReplayState->InputRecording)
                                {
                                    InitInputRecording(GameReplayState);
                                }
                                else
                                {
                                    GameReplayState->InputRecording = false;
                                }
                            }
                        }
                        else if(VKCode == 'L')
                        {
                            if (IsDown)
                            {
                                if (!GameReplayState->InputPlayBack)
                                {
                                    Win32::Dbg::InitInputPlayBack(GameReplayState);
                                }
                                else
                                {
                                    Win32::Dbg::EndInputPlayBack(Input, GameReplayState);
                                }
                            }
                        }
                        else if (VKCode == VK_UP)
                        {
                        }
                        else if (VKCode == VK_LEFT)
                        {
                        }
                        else if (VKCode == VK_DOWN)
                        {
                        }
                        else if (VKCode == VK_RIGHT)
                        {
                        }
                        else if (VKCode == VK_ESCAPE)
                        {
                        }
                        else if (VKCode == VK_SPACE)
                        {
                        }
                    }
                }

                default:
                {
                    TranslateMessage(&Message);
                    DispatchMessageA(&Message);
                } break;
            }
        }
    };

    LRESULT CALLBACK
    ProgramWindowCallback(HWND WindowHandle, UINT Message, WPARAM wParam, LPARAM lParam)
    {
        LRESULT Result{0};

        HDC WindowContext = GetDC(WindowHandle);

        switch(Message)
        {
            case WM_CREATE:
            {
                if(WindowContext)
                {
                    { //Init OpenGL
                        //Pixel format you would like to have if possible
                        PIXELFORMATDESCRIPTOR DesiredPixelFormat{};
                        //Pixel format that windows actually gives you based on your desired pixel format and what the system
                        //acutally has available (32 bit color capabilites? etc.)
                        PIXELFORMATDESCRIPTOR SuggestedPixelFormat{};

                        DesiredPixelFormat.nSize = sizeof(DesiredPixelFormat);
                        DesiredPixelFormat.nVersion = 1;
                        DesiredPixelFormat.iPixelType = PFD_TYPE_RGBA;
                        DesiredPixelFormat.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
                        DesiredPixelFormat.cColorBits = 32;
                        DesiredPixelFormat.cAlphaBits = 8;
                        DesiredPixelFormat.iLayerType = PFD_MAIN_PLANE;

                        int SuggestedPixelFormatIndex = ChoosePixelFormat(WindowContext, &DesiredPixelFormat);
                        DescribePixelFormat(WindowContext, SuggestedPixelFormatIndex, sizeof(SuggestedPixelFormat), &SuggestedPixelFormat);

                        if (SetPixelFormat(WindowContext, SuggestedPixelFormatIndex, &SuggestedPixelFormat))
                        {
                            HGLRC OpenGLRenderingContext = wglCreateContext(WindowContext);

                            if (OpenGLRenderingContext)
                            {
                                if (wglMakeCurrent(WindowContext, OpenGLRenderingContext))
                                {
                                    glViewport(0, 0, WindowWidth, WindowHeight);

                                    //Success! We have a current openGL context. Now setup glew
                                    if(glewInit() == GLEW_OK)
                                    {
                                        if(WGLEW_EXT_swap_control)
                                        {
                                            //Turn Vsync on/off
                                            wglSwapIntervalEXT(0);
                                        }
                                        else
                                        {
                                            Win32::Dbg::LogErr("Failed to find opengl swap interval extention on computer!");
                                        }
                                    }
                                    else
                                    {
                                        ReleaseDC(WindowHandle, WindowContext);
                                        InvalidCodePath;
                                    }
                                }
                                else
                                {
                                    ReleaseDC(WindowHandle, WindowContext);
                                    Win32::Dbg::LogErr("Unable to make opengl context the current context!");
                                    InvalidCodePath;
                                }
                            }
                            else
                            {
                                ReleaseDC(WindowHandle, WindowContext);
                                Win32::Dbg::LogErr("Unable to create an opengl rendering context!");
                                InvalidCodePath;
                            }
                        }
                        else
                        {        
                            ReleaseDC(WindowHandle, WindowContext);
                            Win32::Dbg::LogErr("Unable to set the pixel format for potential opengl window!");
                            InvalidCodePath;
                        }

                        glClearColor(1.0f, 0.0f, 1.0f, 0.0f);
                    }//Init OpenGL 
                }
                else
                {
                    Win32::Dbg::LogErr("Unable to get window context from window!");
                    InvalidCodePath;
                }

            }break;

            case WM_DESTROY:
            {
                GameRunning = false;
            }break;

            case WM_CLOSE:
            {
                GameRunning = false;
            }break;

            case WM_ACTIVATEAPP:
            {
            }break;

            default:
            {
                Result = DefWindowProc(WindowHandle, Message, wParam, lParam);
            }break;
        }

        return Result;
    };

    local_func auto
    ProcessXInputDigitalButton(DWORD XInputButtonState, Button_State* GameButtonState, DWORD XInputButtonBit)
    {
        if(GameButtonState->Pressed != ((XInputButtonState & XInputButtonBit) == XInputButtonBit))
        {
            GameButtonState->Pressed = ((XInputButtonState & XInputButtonBit) == XInputButtonBit);
            ++GameButtonState->NumTransitionsPerFrame;
        }
    }

    local_func auto
    NormalizeAnalogStickValue(SHORT Value, SHORT DeadZoneThreshold) -> f32
    {
        f32 Result = 0;

        if (Value < -DeadZoneThreshold)
        {
            Result = (f32)((Value + DeadZoneThreshold) / (32768.0f - DeadZoneThreshold));
        }
        else if (Value > DeadZoneThreshold)
        {
            Result = (f32)((Value - DeadZoneThreshold) / (32767.0f - DeadZoneThreshold));
        }

        return (Result);
    }
}

namespace GL
{
    local_func auto
    Init() -> void
    {
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0.0, (f32)WindowWidth, 0.0, (f32)WindowHeight, -1.0, 1.0);
    }

    local_func auto
    LoadTexture(Image ImageToSendToGPU) -> Texture
    {
        Texture ResultingTexture{};
        ResultingTexture.Dimensions.Width = ImageToSendToGPU.Dimensions.Width;
        ResultingTexture.Dimensions.Height = ImageToSendToGPU.Dimensions.Height;

        ui8* ImageData = ImageToSendToGPU.Data;

        glEnable(GL_TEXTURE_2D);
        glGenTextures(1, &ResultingTexture.ID);
        glBindTexture(GL_TEXTURE_2D, ResultingTexture.ID);

        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_RGBA, ResultingTexture.Dimensions.Width, ResultingTexture.Dimensions.Height, 
            0, GL_RGBA, GL_UNSIGNED_BYTE, ImageData);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

        //Enable alpha channel for transparency
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBindTexture(GL_TEXTURE_2D, 0);

        return ResultingTexture; 
    }

    local_func auto
    DrawBackground(ui32 TextureID, v2f CameraViewDimensions, v2f MinUV, v2f MaxUV) -> void
    {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, TextureID);

        glBegin(GL_QUADS);
        glTexCoord2f(MinUV.x, MinUV.y);
        glVertex2f(0.0f, 0.0f);

        glTexCoord2f(MaxUV.x, MinUV.y);
        glVertex2f(CameraViewDimensions.x, 0.0f);

        glTexCoord2f(MaxUV.x, MaxUV.y);
        glVertex2f(CameraViewDimensions.x, CameraViewDimensions.y);

        glTexCoord2f(MinUV.x, MaxUV.y);
        glVertex2f(0.0f, CameraViewDimensions.y);

        glEnd();
        glFlush();

        glDisable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    local_func auto
    DrawRect(v2f MinPoint, v2f MaxPoint) -> void
    {
        glBegin(GL_QUADS);

        glVertex2f(MinPoint.x, MinPoint.y);
        glVertex2f(MaxPoint.x, MinPoint.y);
        glVertex2f(MaxPoint.x, MaxPoint.y);
        glVertex2f(MinPoint.x, MaxPoint.y);

        glEnd();
        glFlush();
    }

    local_func auto
    DrawTexture(ui32 TextureID, Drawable_Rect Destination, v2f* UVs)
    {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, TextureID);

        glBegin(GL_QUADS);
        glTexCoord2f(UVs[0].x, UVs[0].y);
        glVertex2f(Destination.BottomLeft.x, Destination.BottomLeft.y);

        glTexCoord2f(UVs[1].x, UVs[1].y);
        glVertex2f(Destination.BottomRight.x, Destination.BottomRight.y);

        glTexCoord2f(UVs[2].x, UVs[2].y);
        glVertex2f(Destination.TopRight.x, Destination.TopRight.y);

        glTexCoord2f(UVs[3].x, UVs[3].y);
        glVertex2f(Destination.TopLeft.x, Destination.TopLeft.y);

        glEnd();
        glFlush();

        glDisable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    local_func auto
    ClearScreen() -> void
    {
        glClear(GL_COLOR_BUFFER_BIT);
    }
}

int CALLBACK WinMain(HINSTANCE CurrentProgramInstance, HINSTANCE PrevInstance, LPSTR CommandLine, int ShowCode)
{
    Win32::Dbg::UseConsole();

    //Set scheduler granularity to help ensure we are able to put thread to sleep by the amount of time specified and no longer
    UINT DesiredSchedulerGranularityMS = 1;
    BGZ_ASSERT(timeBeginPeriod(DesiredSchedulerGranularityMS) == TIMERR_NOERROR)
    
    WNDCLASS WindowProperties{};
    WindowProperties.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW; //TODO: Check if OWNDC/HREDRAW/VEDRAW matter
    WindowProperties.lpfnWndProc = Win32::ProgramWindowCallback;
    WindowProperties.hInstance = CurrentProgramInstance;
    WindowProperties.lpszClassName = "MemoWindowClass";

    if(RegisterClass(&WindowProperties))
    {
        HWND Window = CreateWindowEx(0, WindowProperties.lpszClassName, "Memo", WS_OVERLAPPEDWINDOW|WS_VISIBLE, 
                                     CW_USEDEFAULT, CW_USEDEFAULT, WindowWidth, WindowHeight, 0, 0, CurrentProgramInstance, 0);

        HDC WindowContext = GetDC(Window);

        if(Window && WindowContext)
        {

#if DEVELOPMENT_BUILD
            void *BaseAddress{(void *)Terabytes(2)};
#else
            void *BaseAddress{(void *)0};
#endif

            Game_Input Input{};
            Game_Sound_Output_Buffer SoundBuffer{};
            Game_Render_Cmds RenderCmds{};
            Platform_Services PlatformServices{};
            Win32::Dbg::Game_Replay_State GameReplayState{};
            Win32::Game_Code GameCode{Win32::Dbg::LoadGameCodeDLL("build/gamecode.dll")};
            BGZ_ASSERT(GameCode.DLLHandle);

            {//Init Game Memory
                GameMemory.SizeOfPermanentStorage = Megabytes(64);
                GameMemory.SizeOfTemporaryStorage = Gigabytes(1);
                GameMemory.TotalSize = GameMemory.SizeOfPermanentStorage + GameMemory.SizeOfTemporaryStorage;
                GameMemory.PermanentStorage = VirtualAlloc(BaseAddress, GameMemory.TotalSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);//TODO: Add large page support?
                GameMemory.TemporaryStorage = ((ui8 *)GameMemory.PermanentStorage + GameMemory.SizeOfPermanentStorage);
            }

            {//Init input recording and replay services
                GameReplayState.RecordedInputs = (Game_Input *)VirtualAlloc(0, (sizeof(Game_Input) * Win32::Dbg::MaxAllowableRecordedInputs), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
                GameReplayState.OriginalRecordedGameState = VirtualAlloc(0, GameMemory.TotalSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            }

            { //Init game services
                PlatformServices.WriteEntireFile = &Win32::Dbg::WriteEntireFile;
                PlatformServices.ReadEntireFile = &Win32::Dbg::ReadEntireFile;
                PlatformServices.ReadFileOfLength = &Win32::Dbg::ReadFileOfLength;
                PlatformServices.FreeFileMemory = &Win32::Dbg::FreeFileMemory;
                PlatformServices.LoadRGBAImage = &Win32::Dbg::LoadRGBAImage;
                PlatformServices.PlatMalloc = &Win32::Dbg::Malloc;
                PlatformServices.PlatCalloc = &Win32::Dbg::Calloc;
                PlatformServices.PlatFree = &Win32::Dbg::Free;

                RenderCmds.DrawRect = &GL::DrawRect;
                RenderCmds.ClearScreen = &GL::ClearScreen;
                RenderCmds.DrawBackground = &GL::DrawBackground;
                RenderCmds.LoadTexture = &GL::LoadTexture;
                RenderCmds.DrawTexture = &GL::DrawTexture;
                RenderCmds.Init = &GL::Init;
            }

            ui32 MonitorRefreshRate = bgz::MonitorRefreshHz();
            int GameRefreshRate{};
            f32 TargetSecondsPerFrame{};

            switch(MonitorRefreshRate)
            {
                case 30: 
                {
                    GameRefreshRate = MonitorRefreshRate;
                    TargetSecondsPerFrame = 1.0f/(f32)GameRefreshRate;
                }break;
                case 60: 
                {
                    GameRefreshRate = MonitorRefreshRate;
                    TargetSecondsPerFrame = 1.0f/(f32)GameRefreshRate;
                }break;
                case 120: 
                {
                    GameRefreshRate = MonitorRefreshRate/2;
                    TargetSecondsPerFrame = 1.0f/(f32)GameRefreshRate;
                }break;
                default:
                InvalidCodePath //Unkown monitor refresh rate
            };

            GameRunning = true;

            bgz::Timer FramePerformanceTimer{};
            FramePerformanceTimer.Init();

            while (GameRunning)
            {
                //Hot reloading
                FILETIME NewGameCodeDLLWriteTime = Win32::Dbg::GetFileTime("build/gamecode.dll");
                if(CompareFileTime(&NewGameCodeDLLWriteTime, &GameCode.PreviousDLLWriteTime) != 0)
                {
                    Win32::Dbg::FreeGameCodeDLL(&GameCode, &PlatformServices);
                    GameCode = Win32::Dbg::LoadGameCodeDLL("build/gamecode.dll");
                }

                //TODO: Should we poll more frequently?
                auto [UpdatedInput, UpdatedReplayState] = [](Game_Input Input, Win32::Dbg::Game_Replay_State GameReplayState) -> auto
                {
                    struct Input_Result {Game_Input NewInput; Win32::Dbg::Game_Replay_State NewGameReplayState;};
                    Input_Result Result{};

                    for (ui32 ControllerIndex = 0; ControllerIndex < ArrayCount(Input.Controllers); ++ControllerIndex)
                    {
                        ClearTransitionCounts(&Input.Controllers[ControllerIndex]);
                    }

                    //Poll Keyboard Input
                    Win32::ProcessPendingMessages(&Input, &GameReplayState);

                    {//Poll GamePad Input(s)
                        for (DWORD ControllerIndex = 0; ControllerIndex < ArrayCount(Input.Controllers); ++ControllerIndex)
                        {
                            Game_Controller *MyGamePad = &Input.Controllers[ControllerIndex + 1]; //Since index 0 is reserved for keyboard

                            XINPUT_STATE ControllerState;
                            if (XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
                            {
                                if (ControllerIndex == ArrayCount(Input.Controllers)) //Since index 0 is reserved for keyboard
                                    break;

                                //This controller is plugged in
                                XINPUT_GAMEPAD *XGamePad = &ControllerState.Gamepad;
                                MyGamePad->IsConnected = true;

                                MyGamePad->LThumbStick.x = Win32::NormalizeAnalogStickValue(XGamePad->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
                                MyGamePad->LThumbStick.y = Win32::NormalizeAnalogStickValue(XGamePad->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);

                                if((MyGamePad->LThumbStick.x != 0.0f) || (MyGamePad->LThumbStick.y != 0.0f))
                                {
                                    MyGamePad->IsAnalog = true;
                                }

                                if (XGamePad->wButtons & XINPUT_GAMEPAD_DPAD_UP)
                                {
                                    MyGamePad->LThumbStick.y = 1.0f;
                                    MyGamePad->IsAnalog = false;
                                }

                                if (XGamePad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN)
                                {
                                    MyGamePad->LThumbStick.y = -1.0f;
                                    MyGamePad->IsAnalog = false;
                                }

                                if (XGamePad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT)
                                {
                                    MyGamePad->LThumbStick.x = -1.0f;
                                    MyGamePad->IsAnalog = false;
                                }

                                if (XGamePad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)
                                {
                                    MyGamePad->LThumbStick.x = 1.0f;
                                    MyGamePad->IsAnalog = false;
                                }
     
                                Win32::ProcessXInputDigitalButton(XGamePad->wButtons, &MyGamePad->MoveUp, XINPUT_GAMEPAD_DPAD_UP);
                                Win32::ProcessXInputDigitalButton(XGamePad->wButtons, &MyGamePad->MoveDown, XINPUT_GAMEPAD_DPAD_DOWN);
                                Win32::ProcessXInputDigitalButton(XGamePad->wButtons, &MyGamePad->MoveLeft, XINPUT_GAMEPAD_DPAD_LEFT);
                                Win32::ProcessXInputDigitalButton(XGamePad->wButtons, &MyGamePad->MoveRight, XINPUT_GAMEPAD_DPAD_RIGHT);
                                Win32::ProcessXInputDigitalButton(XGamePad->wButtons, &MyGamePad->LeftShoulder, XINPUT_GAMEPAD_LEFT_SHOULDER);
                                Win32::ProcessXInputDigitalButton(XGamePad->wButtons, &MyGamePad->RightShoulder, XINPUT_GAMEPAD_RIGHT_SHOULDER);
                                Win32::ProcessXInputDigitalButton(XGamePad->wButtons, &MyGamePad->ActionUp, XINPUT_GAMEPAD_Y);
                                Win32::ProcessXInputDigitalButton(XGamePad->wButtons, &MyGamePad->ActionDown, XINPUT_GAMEPAD_A);
                                Win32::ProcessXInputDigitalButton(XGamePad->wButtons, &MyGamePad->ActionLeft, XINPUT_GAMEPAD_X);
                                Win32::ProcessXInputDigitalButton(XGamePad->wButtons, &MyGamePad->ActionRight, XINPUT_GAMEPAD_B);
                                Win32::ProcessXInputDigitalButton(XGamePad->wButtons, &MyGamePad->Start, XINPUT_GAMEPAD_START);
                                Win32::ProcessXInputDigitalButton(XGamePad->wButtons, &MyGamePad->Back, XINPUT_GAMEPAD_BACK);
                            }
                            else
                            {
                                //Controller not available
                                MyGamePad->IsConnected = false;
                            }
                        }
                    };

                    Result.NewInput = Input;
                    Result.NewGameReplayState = GameReplayState;

                    return Result;
                }(Input, GameReplayState);

                if(UpdatedReplayState.InputRecording)
                {
                    Win32::Dbg::RecordInput(&UpdatedInput, &UpdatedReplayState);
                }

                if(UpdatedReplayState.InputPlayBack)
                {
                    Win32::Dbg::PlayBackInput(&UpdatedInput, &UpdatedReplayState);
                }

                GameCode.UpdateFunc(&GameMemory, &PlatformServices, RenderCmds, &SoundBuffer, &UpdatedInput);

                Input = UpdatedInput;
                GameReplayState = UpdatedReplayState; 

                f32 SecondsElapsedForWork = FramePerformanceTimer.SecondsElapsed();

                if(SecondsElapsedForWork < TargetSecondsPerFrame)
                {
                    DWORD MSToSleep = (DWORD)(1000.0f * (TargetSecondsPerFrame - SecondsElapsedForWork));
                    Sleep(MSToSleep);
                }
                else
                {
                    BGZ_CONSOLE("Missed our frame rate!!!\n");
                }

                SwapBuffers(WindowContext);

                f32 FrameTimeInMS = FramePerformanceTimer.MilliSecondsElapsed();
                FramePerformanceTimer.UpdateTimeCount();

                //BGZ_CONSOLE("ms per frame: %f\n", FrameTimeInMS);
            };

            wglMakeCurrent(NULL, NULL);
            ReleaseDC(Window, WindowContext);
        }
        else
        {
            Win32::Dbg::LogErr("Window could not be created!");
            InvalidCodePath;
        }
    }
    else
    {
        Win32::Dbg::LogErr("Window properties could not be registered!");
        InvalidCodePath;
    }

    return 0;
}
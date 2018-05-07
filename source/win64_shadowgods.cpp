/*
    ToDo List:

    - Load XInput through LoadLibrary() (To help avoid likely "Xinput.dll not found on certain windows platforms")
    - Have it so I handle a game controller being disconnected at any point while the game is running
*/

#if (DEVELOPMENT_BUILD)
    #define BGZ_LOGGING_ON true
#else
    #define BGZ_LOGGING_ON false
#endif

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <xinput.h>
#include <io.h> 
#include <fcntl.h> 
#include <gl/gl.h>
#include <boagz/error_handling.h>

#include "types.h"
#include "math.h"
#include "utilities.h"
#include "win64_shadowgods.h"
#include "shared.h"

global_variable bool GameRunning{};

namespace Win32
{
    local_func auto 
    LogErr(const char* ErrMessage) -> void
    {
        BGZ_CONSOLE("Windows error code: %d\n", GetLastError());
        BGZ_CONSOLE(ErrMessage);
    };

    local_func auto
    ProcessKeyboardMessage(Button_State* NewState, bool32 IsDown) -> void
    {
        if (NewState->Pressed != IsDown)
        {
            NewState->Pressed = IsDown;
            ++NewState->NumTransitionsPerFrame;
        }
    }

    local_func auto 
    ProcessPendingMessages(Game_Controller* Keyboard) -> void
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
                    uint32 VKCode = (uint32)Message.wParam;

                    //lParam is basically a bit field and part of that bit field has bits specifying 
                    //if the certain VKcode is down now and if it was down previously. Just filtering
                    //those out more explicitly here
                    bool32 WasDown = ((Message.lParam & (1 << 30)) != 0);
                    bool32 IsDown = ((Message.lParam & (1 << 31)) == 0);

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
                        }
                        else if (VKCode == 'E')
                        {
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
                                    //Success!
                                }
                                else
                                {
                                    ReleaseDC(WindowHandle, WindowContext);
                                    Win32::LogErr("Unable to make opengl context the current context!");
                                    InvalidCodePath;
                                }
                            }
                            else
                            {
                                ReleaseDC(WindowHandle, WindowContext);
                                Win32::LogErr("Unable to create an opengl rendering context!");
                                InvalidCodePath;
                            }
                        }
                        else
                        {        
                            ReleaseDC(WindowHandle, WindowContext);
                            Win32::LogErr("Unable to set the pixel format for potential opengl window!");
                            InvalidCodePath;
                        }

                        glClearColor(1.0f, 0.0f, 1.0f, 0.0f);
                    }//Init OpenGL 
                }
                else
                {
                    Win32::LogErr("Unable to get window context from window!");
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
                OutputDebugStringA("WM_ACTIVATEAPP\n");
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
    NormalizeAnalogStickValue(SHORT Value, SHORT DeadZoneThreshold) -> float32
    {
        float32 Result = 0;

        if (Value < -DeadZoneThreshold)
        {
            Result = (float32)((Value + DeadZoneThreshold) / (32768.0f - DeadZoneThreshold));
        }
        else if (Value > DeadZoneThreshold)
        {
            Result = (float32)((Value - DeadZoneThreshold) / (32767.0f - DeadZoneThreshold));
        }

        return (Result);
    }

    namespace Dbg
    {
        local_func auto
        FreeFileMemory(void* FileMemory)
        {
            if(FileMemory)
            {
                VirtualFree(FileMemory, 0, MEM_RELEASE);
            }
        };

        local_func auto
        WriteEntireFile(const char* FileName, void* Memory, uint32 MemorySize) -> bool 
        {
            bool32 Result = false;

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
                    Win32::LogErr("Unable to write to file!");
                    InvalidCodePath;
                }

                CloseHandle(FileHandle);
            }
            else
            {
                Win32::LogErr("Unable to create file handle!");
                InvalidCodePath;
            }

            return (Result);
        };

        local_func auto
        ReadEntireFile(const char *FileName) -> Read_File_Result
        {
            Read_File_Result Result{};

            HANDLE FileHandle = CreateFile(FileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

            if(FileHandle != INVALID_HANDLE_VALUE)
            {
                LARGE_INTEGER FileSize{};
                if(GetFileSizeEx(FileHandle, &FileSize))
                {
                    uint32 FileSize32 = SafeTruncateUInt64(FileSize.QuadPart);
                    Result.FileContents = VirtualAlloc(0, FileSize32, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

                    if(Result.FileContents)
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
                        Win32::LogErr("Unable to allocate memory for read file!");
                        InvalidCodePath;
                    }
                }
                else
                {
                    Win32::LogErr("Unable to get size of the file!");
                    InvalidCodePath;
                }
            }
            else
            {
                Win32::LogErr("Unable to get file handle!");
                InvalidCodePath;
            }

            return Result;
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
            uint32 Counter{};
            uint32 MaxAllowedLoops{5000};

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
        FreeGameCodeDLL(Game_Code *GameCode) -> void
        {
            if (GameCode->DLLHandle != INVALID_HANDLE_VALUE)
            {
                FreeLibrary(GameCode->DLLHandle);
                GameCode->DLLHandle = 0;
                GameCode->UpdateFunc = nullptr;
            };
        };
    }
}

int CALLBACK WinMain(HINSTANCE CurrentProgramInstance, HINSTANCE PrevInstance, LPSTR CommandLine, int ShowCode)
{
    Win32::Dbg::UseConsole();

    WNDCLASS WindowProperties{};

    //TODO: Check if OWNDC/HREDRAW/VEDRAW matter
    WindowProperties.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
    WindowProperties.lpfnWndProc = Win32::ProgramWindowCallback;
    WindowProperties.hInstance = CurrentProgramInstance;
    WindowProperties.lpszClassName = "MemoWindowClass";

    if(RegisterClass(&WindowProperties))
    {
        int WindowWidth{CW_USEDEFAULT};
        int WindowHeight{CW_USEDEFAULT};

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

            Game_Memory GameMemory{};
            GameMemory.PermanentStorageSize = Megabytes(64);
            GameMemory.TemporaryStorageSize = Gigabytes(1);
            uint64 TotalStorageSize = GameMemory.PermanentStorageSize + GameMemory.TemporaryStorageSize;
            GameMemory.PermanentStorage = VirtualAlloc(BaseAddress, TotalStorageSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            GameMemory.TemporaryStorage = ((uint8 *)GameMemory.PermanentStorage + GameMemory.TemporaryStorageSize);

            Game_Code GameCode{Win32::Dbg::LoadGameCodeDLL("build/gamecode.dll")};

            Game_Sound_Output_Buffer SoundBuffer{};
            Game_Render_Cmd_Buffer RenderCmdBuffer{};
            Platform_Services PlatformServices{};
            Game_Input OldInput{};

            {//Init game services
                PlatformServices.WriteEntireFile = &Win32::Dbg::WriteEntireFile;
                PlatformServices.ReadEntireFile = &Win32::Dbg::ReadEntireFile;
                PlatformServices.FreeFileMemory = &Win32::Dbg::FreeFileMemory;
            }

            GameRunning = true;
            while (GameRunning)
            {
                //Hot reloading
                FILETIME NewGameCodeDLLWriteTime = Win32::Dbg::GetFileTime("build/gamecode.dll");
                if(CompareFileTime(&NewGameCodeDLLWriteTime, &GameCode.PreviousDLLWriteTime) != 0)
                {
                    Win32::Dbg::FreeGameCodeDLL(&GameCode);
                    GameCode = Win32::Dbg::LoadGameCodeDLL("build/gamecode.dll");
                }

                Game_Input UpdatedInput = [](Game_Input Input) -> Game_Input
                {
                    for (uint ControllerIndex = 0; ControllerIndex < Input.MaxControllerCount; ++ControllerIndex)
                    {
                        ClearTransitionCounts(&Input.Controllers[ControllerIndex]);
                    }

                    Game_Controller* Keyboard = &Input.Controllers[0];

                    Win32::ProcessPendingMessages(Keyboard);

                    //TODO: Should we poll this more frequently?
                    for (DWORD ControllerIndex = 0; ControllerIndex < Input.MaxControllerCount; ++ControllerIndex)
                    {
                        Game_Controller *MyGamePad = &Input.Controllers[ControllerIndex + 1]; //Since index 0 is reserved for keyboard

                        XINPUT_STATE ControllerState;
                        if (XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
                        {
                            if (ControllerIndex == Input.MaxControllerCount) //Since index 0 is reserved for keyboard
                                break;

                            //This controller is plugged in
                            XINPUT_GAMEPAD *XGamePad = &ControllerState.Gamepad;
                            MyGamePad->IsConnected = true;

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

                            MyGamePad->LThumbStick.X = 0.0f;
                            MyGamePad->LThumbStick.Y = 0.0f;

                            MyGamePad->LThumbStick.X = Win32::NormalizeAnalogStickValue(XGamePad->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
                            MyGamePad->LThumbStick.Y = Win32::NormalizeAnalogStickValue(XGamePad->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
                        }
                        else
                        {
                            //Controller not available
                            MyGamePad->IsConnected = false;
                        }
                    }

                    return Input;
                }(OldInput);

                GameCode.UpdateFunc(&GameMemory, PlatformServices, &RenderCmdBuffer, &SoundBuffer, &UpdatedInput);

                OldInput = UpdatedInput;

                glViewport(0, 0, WindowWidth, WindowHeight);
                glClear(GL_COLOR_BUFFER_BIT);
                SwapBuffers(WindowContext);
            };

            wglMakeCurrent(NULL, NULL);
            ReleaseDC(Window, WindowContext);
        }
        else
        {
            Win32::LogErr("Window could not be created!");
            InvalidCodePath;
        }
    }
    else
    {
        Win32::LogErr("Window properties could not be registered!");
        InvalidCodePath;
    }

    return 0;
}
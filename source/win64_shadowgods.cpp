#if (DEVELOPMENT_BUILD)
    #define BGZ_LOGGING_ON true
#else
    #define BGZ_LOGGING_ON false
#endif

#include <Windows.h>
#include <gl/gl.h>
#include <boagz/error_handling.h>

#include "types.h"
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
    ProcessPendingMessages() -> void
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

                    if(VKCode == 'W')
                    {
                    }
                    else if(VKCode == 'A')
                    {
                    }
                    else if(VKCode == 'S')
                    {
                    }
                    else if(VKCode == 'D')
                    {
                    }
                    else if(VKCode == 'Q')
                    {
                    }
                    else if(VKCode == 'E')
                    {
                    }
                    else if(VKCode == VK_UP)
                    {
                    }
                    else if(VKCode == VK_LEFT)
                    {
                    }
                    else if(VKCode == VK_DOWN)
                    {
                    }
                    else if(VKCode == VK_RIGHT)
                    {
                    }
                    else if(VKCode == VK_ESCAPE)
                    {
                    }
                    else if(VKCode == VK_SPACE)
                    {
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

    namespace Dbg
    {
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
    //Needed to setup console for windows app
    Bgz::UseDefaultOSConsole();

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
            Game_Input Input{};

            GameRunning = true;
            while (GameRunning)
            {
                Win32::ProcessPendingMessages();

                GameCode.UpdateFunc(&GameMemory, PlatformServices, &RenderCmdBuffer, &SoundBuffer, &Input);

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
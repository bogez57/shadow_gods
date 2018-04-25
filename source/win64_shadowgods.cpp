#if (DEVELOPMENT_BUILD)
    #define BGZ_LOGGING_ON true
#else
    #define BGZ_LOGGING_ON false
#endif

#include <Windows.h>
#include <gl/gl.h>
#include <boagz/error_handling.h>

#include "types.h"

namespace Win64
{
    local_func void
    ProcessPendingMessages()
    {
        MSG Message;
        while(GetMessageA(&Message, 0, 0, 0))
        {
            switch(Message.message)
            {
                case WM_QUIT:
                {
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

        switch(Message)
        {
            case WM_SIZE:
            {
                OutputDebugStringA("WM_SIZE\n");
            }break;

            case WM_DESTROY:
            {
                OutputDebugStringA("WM_DESTROY\n");
            }break;

            case WM_CLOSE:
            {
                OutputDebugStringA("WM_CLOSE\n");
            }break;

            case WM_ACTIVATEAPP:
            {
                OutputDebugStringA("WM_ACTIVATEAPP\n");
            }break;

            case WM_PAINT:
            {
                PAINTSTRUCT Paint;
                RECT ClientRect;
                GetClientRect(WindowHandle, &ClientRect);

                HDC DeviceContext = BeginPaint(WindowHandle, &Paint);

                LONG Width = ClientRect.right - ClientRect.left;
                LONG Height = ClientRect.bottom - ClientRect.top;

                glViewport(0, 0, Width, Height);
                glClearColor(1.0f, 0.0f, 1.0f, 0.0f);
                glClear(GL_COLOR_BUFFER_BIT);
                SwapBuffers(DeviceContext);

                EndPaint(WindowHandle, &Paint);
            }break;

            default:
            {
                Result = DefWindowProc(WindowHandle, Message, wParam, lParam);
            }break;
        }

        return Result;
    };
}

int CALLBACK WinMain(HINSTANCE CurrentProgramInstance, HINSTANCE PrevInstance, LPSTR CommandLine, int ShowCode)
{
    //Needed to setup console for windows app
    Bgz::UseDefaultOSConsole();

    WNDCLASS WindowProperties{};

    //TODO: Check if OWNDC/HREDRAW/VEDRAW matter
    WindowProperties.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
    WindowProperties.lpfnWndProc = Win64::ProgramWindowCallback;
    WindowProperties.hInstance = CurrentProgramInstance;
    WindowProperties.lpszClassName = "MemoWindowClass";

    if(RegisterClass(&WindowProperties))
    {
        HWND Window = CreateWindowEx(0, WindowProperties.lpszClassName, "Memo", WS_OVERLAPPEDWINDOW|WS_VISIBLE, 
                                     CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, CurrentProgramInstance, 0);

        if(Window)
        {
            HDC WindowDeviceContext = GetDC(Window);

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

                int SuggestedPixelFormatIndex = ChoosePixelFormat(WindowDeviceContext, &DesiredPixelFormat);
                DescribePixelFormat(WindowDeviceContext, SuggestedPixelFormatIndex, sizeof(SuggestedPixelFormat), &SuggestedPixelFormat);

                if (SetPixelFormat(WindowDeviceContext, SuggestedPixelFormatIndex, &SuggestedPixelFormat))
                {
                    HGLRC OpenGLRenderingContext = wglCreateContext(WindowDeviceContext);

                    if (OpenGLRenderingContext)
                    {
                        if (wglMakeCurrent(WindowDeviceContext, OpenGLRenderingContext))
                        {
                            //Success!
                        }
                        else
                        {
                            BGZ_CONSOLE("Windows error code: %d", GetLastError());
                            InvalidCodePath;
                        }
                    }
                    else
                    {
                        BGZ_CONSOLE("Windows error code: %d", GetLastError());
                        InvalidCodePath;
                    }
                }
                else
                {
                    BGZ_CONSOLE("Windows error code: %d", GetLastError());
                    InvalidCodePath;
                }
            }//Init OpenGL 

            Win64::ProcessPendingMessages();
        }
    }
    else
    {
        BGZ_CONSOLE("Windows error code: %d", GetLastError());
        InvalidCodePath;
    }

    return 0;
}
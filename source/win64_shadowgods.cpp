/*

    ToDo List:

    - Load XInput through LoadLibrary() (To help avoid likely "Xinput.dll not found on certain windows platforms")
    - Make current frame timing more accurate/in-sync with monitor refresh rate
    - Pass deltatime to game
    - Fix dll reloading which currently free's and reload's game code twice upon building. I believe this is because
    when comparing file times between old and new dll write times the game tends to update fast enough that the times
    are still the same upon the second run through of the loop since the last run through.
    - Make an array class
*/

#if (DEVELOPMENT_BUILD)

#define BGZ_ERRHANDLING_ON true
#define BGZ_LOGGING_ON true
#else

#define BGZ_ERRHANDLING_ON false
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
#include <stb/stb_image.h>

#define ATOMIC_TYPES_IMPL
#include "atomic_types.h"
#include "array.h"
#include "memory_handling.h"

#include "my_math.h"
#include "utilities.h"
#include "renderer_stuff.h"
#include "win64_shadowgods.h"
#include "shared.h"
#include "opengl.h"
#include "software_rendering.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#define PLATFORM_RENDERER_STUFF_IMPL
#include "renderer_stuff.h"
#define MEMORY_HANDLING_IMPL
#include "memory_handling.h"
#define BGZ_MAX_CONTEXTS 10000
#include <boagz/error_context.cpp>

global_variable ui32 globalWindowWidth { 1280 };
global_variable ui32 globalWindowHeight { 720 };
global_variable Win32::Offscreen_Buffer globalBackBuffer;
global_variable Application_Memory gameMemory;
global_variable bool GameRunning {};

namespace Win32::Dbg
{
    local_func auto
        LogErr(const char* ErrMessage) -> void
    {
        BGZ_CONSOLE("Windows error code: %d\n", GetLastError());
        BGZ_CONSOLE(ErrMessage);
    };
    
    local_func auto
        Malloc(sizet Size) -> void*
    {
        void* Result {};
        
        Result = malloc(Size);
        
        return Result;
    };
    
    local_func auto
        Calloc(sizet Count, sizet Size) -> void*
    {
        void* Result {};
        
        Result = calloc(Count, Size);
        
        return Result;
    };
    
    local_func auto
        Realloc(void* ptr, sizet size) -> void*
    {
        void* result {};
        
        result = realloc(ptr, size);
        
        return result;
    };
    
    local_func auto
        Free(void* PtrToFree) -> void
    {
        free(PtrToFree);
    };
    
    local_func auto
        FreeFileMemory(void* FileMemory)
    {
        if (FileMemory)
        {
            VirtualFree(FileMemory, 0, MEM_RELEASE);
        }
    };
    
    local_func auto
        WriteEntireFile(const char* FileName, void* memory, ui32 MemorySize) -> bool
    {
        b32 Result = false;
        
        HANDLE FileHandle = CreateFile(FileName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
        if (FileHandle != INVALID_HANDLE_VALUE)
        {
            DWORD BytesWritten;
            if (WriteFile(FileHandle, memory, MemorySize, &BytesWritten, 0))
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
        ReadEntireFile(i32&& length, const char* FilePath) -> char*
    {
        char* data;
        FILE* file;
        errno_t err;
        
        if ((err = fopen_s(&file, FilePath, "rb")) != 0)
        {
            InvalidCodePath;
        };
        
        fseek(file, 0, SEEK_END);
        length = (i32)ftell(file);
        fseek(file, 0, SEEK_SET);
        
        data = (char*)malloc(length);
        fread(data, 1, length, file);
        
        fclose(file);
        
        return data;
    };
    
    local_func auto
        LoadBGRAImage(const char* ImagePath, i32&& width, i32&& height) -> ui8*
    {
        stbi_set_flip_vertically_on_load(true); //So first byte stbi_load() returns is bottom left instead of top-left of image (which is stb's default)
        
        i32 numOfLoadedChannels {};
        i32 desiredChannels { 4 }; //Since I still draw assuming 4 byte pixels I need 4 channels
        
        //Returns RGBA
        unsigned char* imageData = stbi_load(ImagePath, &width, &height, &numOfLoadedChannels, desiredChannels);
        BGZ_ASSERT(imageData, "Invalid image data!");
        
        i32 totalPixelCountOfImg = width * height;
        ui32* imagePixel = (ui32*)imageData;
        
        //Swap R and B channels of image
        for (int i = 0; i < totalPixelCountOfImg; ++i)
        {
            auto color = UnPackPixelValues(*imagePixel, RGBA);
            
            //Pre-multiplied alpha
            f32 alphaBlend = color.a / 255.0f;
            color.rgb *= alphaBlend;
            
            ui32 newSwappedPixelColor = (((ui8)color.a << 24) | ((ui8)color.r << 16) | ((ui8)color.g << 8) | ((ui8)color.b << 0));
            
            *imagePixel++ = newSwappedPixelColor;
        }
        
        return (ui8*)imageData;
    }
    
    local_func auto
        UseConsole() -> void
    {
        //Create a console for this application
        AllocConsole();
        
        // Get STDOUT handle
        HANDLE ConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
        int SystemOutput = _open_osfhandle(intptr_t(ConsoleOutput), _O_TEXT);
        FILE* COutputHandle = _fdopen(SystemOutput, "w");
        
        // Get STDERR handle
        HANDLE ConsoleError = GetStdHandle(STD_ERROR_HANDLE);
        int SystemError = _open_osfhandle(intptr_t(ConsoleError), _O_TEXT);
        FILE* CErrorHandle = _fdopen(SystemError, "w");
        
        // Get STDIN handle
        HANDLE ConsoleInput = GetStdHandle(STD_INPUT_HANDLE);
        int SystemInput = _open_osfhandle(intptr_t(ConsoleInput), _O_TEXT);
        FILE* CInputHandle = _fdopen(SystemInput, "r");
        
        // Redirect the CRT standard input, output, and error handles to the console
        freopen_s(&CInputHandle, "CONIN$", "r", stdin);
        freopen_s(&COutputHandle, "CONOUT$", "w", stdout);
        freopen_s(&CErrorHandle, "CONOUT$", "w", stderr);
    };
    
    inline auto
        GetFileTime(const char* FileName) -> FILETIME
    {
        FILETIME TimeFileWasLastWrittenTo {};
        WIN32_FILE_ATTRIBUTE_DATA FileData {};
        
        if (GetFileAttributesEx(FileName, GetFileExInfoStandard, &FileData))
        {
            TimeFileWasLastWrittenTo = FileData.ftLastWriteTime;
        }
        
        return TimeFileWasLastWrittenTo;
    }
    
    local_func auto
        LoadGameCodeDLL(const char* GameCodeDLL) -> Game_Code
    {
        Game_Code GameCode {};
        const char* GameCodeTempDLL = "w:/shadow_gods/build/game_temp.dll";
        
        GameCode.PreviousDLLWriteTime = GetFileTime(GameCodeDLL);
        
        //Copy app code dll file to a temp file and load that temp file so that original app code dll can be written to while exe
        //is running. This is For live editing purposes. Code is currently being looped because the modified source dll that gets compiled
        //apparently isn't unlocked by Windows in time for it to be copied upon the first few CopyFile() function calls.
        bool CopyFileFuncNotWorking { true };
        ui32 Counter {};
        ui32 MaxAllowedLoops { 5000 };
        
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
        FreeGameCodeDLL(Game_Code&& GameCode, Platform_Services&& platformServices) -> void
    {
        if (GameCode.DLLHandle != INVALID_HANDLE_VALUE)
        {
            FreeLibrary(GameCode.DLLHandle);
            GameCode.DLLHandle = 0;
            GameCode.UpdateFunc = nullptr;
            platformServices.DLLJustReloaded = true;
        };
    };
    
    local_func auto
        InitInputRecording(Win32::Dbg::Game_Replay_State&& GameReplayState) -> void
    {
        GameReplayState.InputRecording = true;
        GameReplayState.TotalInputStructsRecorded = 0;
        GameReplayState.InputCount = 0;
        memcpy(GameReplayState.OriginalRecordedGameState, gameMemory.permanentStorage, gameMemory.totalSize);
    };
    
    local_func auto
        RecordInput(const Game_Input* Input, Win32::Dbg::Game_Replay_State&& GameReplayState) -> void
    {
        GameReplayState.RecordedInputs[GameReplayState.InputCount] = *Input;
        ++GameReplayState.InputCount;
        ++GameReplayState.TotalInputStructsRecorded;
    };
    
    local_func auto
        InitInputPlayBack(Win32::Dbg::Game_Replay_State&& GameReplayState) -> void
    {
        GameReplayState.InputPlayBack = true;
        GameReplayState.InputCount = 0;
        //Set game state back to when it was first recorded for proper looping playback
        memcpy(gameMemory.permanentStorage, GameReplayState.OriginalRecordedGameState, gameMemory.totalSize);
    }
    
    local_func auto
        EndInputPlayBack(Game_Input&& Input, Win32::Dbg::Game_Replay_State&& GameReplayState) -> void
    {
        GameReplayState.InputPlayBack = false;
        for (ui32 ControllerIndex { 0 }; ControllerIndex < ArrayCount(Input.Controllers); ++ControllerIndex)
        {
            for (ui32 ButtonIndex { 0 }; ButtonIndex < ArrayCount(Input.Controllers[ControllerIndex].Buttons); ++ButtonIndex)
            {
                Input.Controllers[ControllerIndex].Buttons[ButtonIndex].Pressed = false;
            }
        }
    };
    
    local_func auto
        PlayBackInput(Game_Input&& Input, Win32::Dbg::Game_Replay_State&& GameReplayState) -> void
    {
        if (GameReplayState.InputCount < GameReplayState.TotalInputStructsRecorded)
        {
            Input = GameReplayState.RecordedInputs[GameReplayState.InputCount];
            ++GameReplayState.InputCount;
        }
        else
        {
            GameReplayState.InputCount = 0;
            memcpy(gameMemory.permanentStorage, GameReplayState.OriginalRecordedGameState, gameMemory.totalSize);
        }
    }
} // namespace Win32::Dbg

namespace Win32
{
    local_func Window_Dimension
        GetWindowDimension(HWND window)
    {
        Window_Dimension Result;
        
        RECT ClientRect;
        GetClientRect(window, &ClientRect);
        
        Result.width = ClientRect.right - ClientRect.left;
        Result.height = ClientRect.bottom - ClientRect.top;
        
        return (Result);
    };
    
    local_func void
        ResizeDIBSection(Win32::Offscreen_Buffer&& buffer, int width, int height)
    {
        // TODO: Bulletproof this.
        // Maybe don't free first, free after, then free first if that fails.
        
        if (buffer.memory)
        {
            VirtualFree(buffer.memory, 0, MEM_RELEASE);
        }
        
        buffer.width = width;
        buffer.height = height;
        
        buffer.bytesPerPixel = 4;
        
        // When the biHeight field is negative, this is the clue to
        // Windows to treat this bitmap as top-down, not bottom-up, meaning that
        // the first three bytes of the image are the color for the top left pixel
        // in the bitmap, not the bottom left!
        buffer.Info.bmiHeader.biSize = sizeof(buffer.Info.bmiHeader);
        buffer.Info.bmiHeader.biWidth = buffer.width;
        buffer.Info.bmiHeader.biHeight = buffer.height;
        buffer.Info.bmiHeader.biPlanes = 1;
        buffer.Info.bmiHeader.biBitCount = 32;
        buffer.Info.bmiHeader.biCompression = BI_RGB;
        
        // Thank you to Chris Hecker of Spy Party fame
        // for clarifying the deal with StretchDIBits and BitBlt!
        // No more DC for us.
        int BitmapMemorySize = (buffer.width * buffer.height) * buffer.bytesPerPixel;
        buffer.memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        buffer.pitch = width * buffer.bytesPerPixel;
        
        // TODO: Probably clear this to black
    }
    
    local_func void
        DisplayBufferInWindow(Rendering_Info&& renderingInfo, HDC deviceContext, int windowWidth, int windowHeight, Platform_Services platformServices)
    {
        b renderThroughHardware { false };
        if (renderThroughHardware)
        {
        }
        else
        {
            RenderViaSoftware($(renderingInfo), globalBackBuffer.memory, v2i { globalBackBuffer.width, globalBackBuffer.height }, globalBackBuffer.pitch, &platformServices);
            
            //Performs screen clear so resizing window doesn't screw up the image displayed
            PatBlt(deviceContext, 0, 0, windowWidth, 0, BLACKNESS);
            PatBlt(deviceContext, 0, globalBackBuffer.height, windowWidth, windowHeight, BLACKNESS);
            PatBlt(deviceContext, 0, 0, 0, windowHeight, BLACKNESS);
            PatBlt(deviceContext, globalBackBuffer.width, 0, windowWidth, windowHeight, BLACKNESS);
            
            { //Switched around coordinates and things here so I can treat drawing in game as bottom-up instead of top down
                v2i displayRect_BottomLeftCoords { 0, 0 };
                v2i displayRect_Dimensions {};
                displayRect_Dimensions.width = globalBackBuffer.width;
                displayRect_Dimensions.height = globalBackBuffer.height;
                
                //Copy game's rendered back buffer to whatever display area size you want
                StretchDIBits(deviceContext,
                              displayRect_BottomLeftCoords.x, displayRect_BottomLeftCoords.y, displayRect_Dimensions.width, displayRect_Dimensions.height, //Dest - Area to draw to within window's window
                              0, 0, globalBackBuffer.width, globalBackBuffer.height, //Source - The dimensions/coords of the back buffer the game rendered to
                              globalBackBuffer.memory,
                              &globalBackBuffer.Info,
                              DIB_RGB_COLORS, SRCCOPY);
            };
        };
        
        //Clear out command buffer
        renderingInfo.cmdBuffer.usedAmount = 0;
    };
    
    local_func auto
        ProcessKeyboardMessage(Button_State&& NewState, b32 IsDown) -> void
    {
        if (NewState.Pressed != IsDown)
        {
            NewState.Pressed = IsDown;
            ++NewState.NumTransitionsPerFrame;
        }
    }
    
    local_func auto
        ProcessPendingMessages(Game_Input&& Input, Win32::Dbg::Game_Replay_State&& GameReplayState) -> void
    {
        MSG Message;
        while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
        {
            switch (Message.message)
            {
                case WM_QUIT: {
                    GameRunning = false;
                }
                break;
                
                case WM_SYSKEYDOWN:
                case WM_SYSKEYUP:
                case WM_KEYDOWN:
                case WM_KEYUP: {
                    ui32 VKCode = (ui32)Message.wParam;
                    
                    Game_Controller* Keyboard = &Input.Controllers[0];
                    
                    //Since we are comparing IsDown and WasDown below to filter out key repeats, we need to use == and !=
                    //to convert these bit tests to actual 0 or 1 values.
                    b32 WasDown = ((Message.lParam & (1 << 30)) != 0);
                    b32 IsDown = ((Message.lParam & (1 << 31)) == 0);
                    
                    if (WasDown != IsDown) //Filter out key repeats as my current input scheme is already able to react to held down inputs properly
                    {
                        if (VKCode == 'W')
                        {
                            Win32::ProcessKeyboardMessage($(Keyboard->MoveUp), IsDown);
                        }
                        else if (VKCode == 'S')
                        {
                            Win32::ProcessKeyboardMessage($(Keyboard->MoveDown), IsDown);
                        }
                        else if (VKCode == 'A')
                        {
                            Win32::ProcessKeyboardMessage($(Keyboard->MoveLeft), IsDown);
                        }
                        else if (VKCode == 'D')
                        {
                            Win32::ProcessKeyboardMessage($(Keyboard->MoveRight), IsDown);
                        }
                        else if (VKCode == 'X')
                        {
                            Win32::ProcessKeyboardMessage($(Keyboard->ActionUp), IsDown);
                        }
                        else if (VKCode == 'Z')
                        {
                            Win32::ProcessKeyboardMessage($(Keyboard->ActionDown), IsDown);
                        }
                        else if (VKCode == 'I')
                        {
                            Win32::ProcessKeyboardMessage($(Keyboard->ActionRight), IsDown);
                        }
                        else if (VKCode == 'U')
                        {
                            Win32::ProcessKeyboardMessage($(Keyboard->ActionLeft), IsDown);
                        }
                        else if (VKCode == 'R')
                        {
                            if (IsDown)
                            {
                                if (!GameReplayState.InputRecording)
                                {
                                    InitInputRecording($(GameReplayState));
                                }
                                else
                                {
                                    GameReplayState.InputRecording = false;
                                }
                            }
                        }
                        else if (VKCode == 'L')
                        {
                            if (IsDown)
                            {
                                if (!GameReplayState.InputPlayBack)
                                {
                                    Win32::Dbg::InitInputPlayBack($(GameReplayState));
                                }
                                else
                                {
                                    Win32::Dbg::EndInputPlayBack($(Input), $(GameReplayState));
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
                
                default: {
                    TranslateMessage(&Message);
                    DispatchMessageA(&Message);
                }
                break;
            }
        }
    };
    
    LRESULT CALLBACK
        ProgramWindowCallback(HWND WindowHandle, UINT Message, WPARAM wParam, LPARAM lParam)
    {
        LRESULT Result { 0 };
        
        //For hardware rendering
        HDC WindowContext = GetDC(WindowHandle);
        
        switch (Message)
        {
            case WM_CREATE: {
                if (WindowContext)
                {
                    { //Init OpenGL
                        //Pixel format you would like to have if possible
                        PIXELFORMATDESCRIPTOR DesiredPixelFormat {};
                        //Pixel format that windows actually gives you based on your desired pixel format and what the system
                        //acutally has available (32 bit color capabilites? etc.)
                        PIXELFORMATDESCRIPTOR SuggestedPixelFormat {};
                        
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
                                    glViewport(0, 0, globalWindowWidth, globalWindowHeight);
                                    
                                    //Success! We have a current openGL context. Now setup glew
                                    if (glewInit() == GLEW_OK)
                                    {
                                        if (WGLEW_EXT_swap_control)
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
                    } //Init OpenGL
                }
                else
                {
                    Win32::Dbg::LogErr("Unable to get window context from window!");
                    InvalidCodePath;
                }
            }
            break;
            
            case WM_PAINT: {
                //To understand why you need a display buffer call here as well as game loop, see this post:
                //https://hero.handmade.network/forums/code-discussion/t/825-wm_paint_question_beginner
                PAINTSTRUCT Paint;
                HDC deviceContext = BeginPaint(WindowHandle, &Paint);
#if 0
                Win32::Window_Dimension windowDimension = GetWindowDimension(WindowHandle);
                Win32::DisplayBufferInWindow(deviceContext, windowDimension.width, windowDimension.height);
#endif
                EndPaint(WindowHandle, &Paint);
            }
            break;
            
            case WM_DESTROY: {
                GameRunning = false;
            }
            break;
            
            case WM_CLOSE: {
                GameRunning = false;
            }
            break;
            
            case WM_ACTIVATEAPP: {
            }
            break;
            
            default: {
                Result = DefWindowProc(WindowHandle, Message, wParam, lParam);
            }
            break;
        }
        
        return Result;
    };
    
    local_func auto
        ProcessXInputDigitalButton(DWORD XInputButtonState, Button_State&& GameButtonState, DWORD XInputButtonBit)
    {
        if (GameButtonState.Pressed != ((XInputButtonState & XInputButtonBit) == XInputButtonBit))
        {
            GameButtonState.Pressed = ((XInputButtonState & XInputButtonBit) == XInputButtonBit);
            ++GameButtonState.NumTransitionsPerFrame;
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
} // namespace Win32

struct Work_Queue_Entry
{
    platform_work_queue_callback* callback;
    void* data;
};

struct Work_Queue
{
    HANDLE semaphoreHandle;
    i32 volatile entryCompletionGoal;
    i32 volatile entryCompletionCount;
    i32 volatile nextEntryToWrite;
    i32 volatile nextEntryToRead;
    Work_Queue_Entry entries[256];
};

global_variable Work_Queue globalWorkQueue;

struct Thread_Info
{
    i32 logicalThreadIndex;
};

void AddToWorkQueue(platform_work_queue_callback* callback, void* data)
{
    i32 newNextEntryToWrite = (globalWorkQueue.nextEntryToWrite + 1) % ArrayCount(globalWorkQueue.entries);
    Assert(newNextEntryToWrite != globalWorkQueue.nextEntryToRead);
    
    Work_Queue_Entry entry { callback, data };
    globalWorkQueue.entries[globalWorkQueue.nextEntryToWrite] = entry;
    ++globalWorkQueue.entryCompletionGoal;
    
    _WriteBarrier();
    _mm_sfence();
    
    ++globalWorkQueue.nextEntryToWrite = newNextEntryToWrite;
    ReleaseSemaphore(globalWorkQueue.semaphoreHandle, 1, 0);
};

b DoWork();
void FinishAllWork()
{
    while (globalWorkQueue.entryCompletionGoal != globalWorkQueue.entryCompletionCount)
    {
        DoWork();
    };
    
    globalWorkQueue.entryCompletionCount = 0;
    globalWorkQueue.entryCompletionGoal = 0;
};

b DoWork()
{
    b isThereStillWork {};
    
    i32 originalNextEntryToRead = globalWorkQueue.nextEntryToRead;
    i32 newNextEntryToRead = (originalNextEntryToRead + 1) % ArrayCount(globalWorkQueue.entries);
    if (originalNextEntryToRead != globalWorkQueue.nextEntryToWrite)
    {
        i32 entryIndex = _InterlockedCompareExchange((LONG volatile*)&globalWorkQueue.nextEntryToRead, newNextEntryToRead, originalNextEntryToRead);
        _ReadBarrier();
        
        if (entryIndex == originalNextEntryToRead)
        {
            Work_Queue_Entry entry = globalWorkQueue.entries[entryIndex];
            entry.callback(entry.data);
            _InterlockedIncrement((LONG volatile*)&globalWorkQueue.entryCompletionCount);
        };
        
        isThereStillWork = true;
    }
    else
    {
        isThereStillWork = false;
    }
    
    return isThereStillWork;
};

DWORD WINAPI
ThreadProc(LPVOID param)
{
    Thread_Info* info = (Thread_Info*)param;
    
    while (1)
    {
        if (DoWork())
        {
            //Keep doing work
        }
        else
        {
            WaitForSingleObjectEx(globalWorkQueue.semaphoreHandle, INFINITE, FALSE);
        }
    };
    
    return 0;
};

int CALLBACK WinMain(HINSTANCE CurrentProgramInstance, HINSTANCE PrevInstance, LPSTR CommandLine, int ShowCode)
{
    Win32::Dbg::UseConsole();
    
    Thread_Info threadInfo[8] = {};
    i32 threadCount = ArrayCount(threadInfo);
    
    i32 initialThreadCount = 0;
    globalWorkQueue.semaphoreHandle = CreateSemaphoreExA(0, initialThreadCount, threadCount, 0, 0, SEMAPHORE_ALL_ACCESS);
    
    for (i32 threadIndex {}; threadIndex < ArrayCount(threadInfo); ++threadIndex)
    {
        
        Thread_Info* info = threadInfo + threadIndex;
        info->logicalThreadIndex = threadIndex;
        
        DWORD threadID;
        HANDLE myThread = CreateThread(0, 0, ThreadProc, info, 0, &threadID);
    };
    
    //Set scheduler granularity to help ensure we are able to put thread to sleep by the amount of time specified and no longer
    UINT DesiredSchedulerGranularityMS = 1;
    BGZ_ASSERT(timeBeginPeriod(DesiredSchedulerGranularityMS) == TIMERR_NOERROR, "Error when trying to set windows granularity!");
    
    Win32::ResizeDIBSection($(globalBackBuffer), globalWindowWidth, globalWindowHeight);
    
    WNDCLASS WindowProperties {};
    WindowProperties.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW; //TODO: Check if OWNDC/HREDRAW/VEDRAW matter
    WindowProperties.lpfnWndProc = Win32::ProgramWindowCallback;
    WindowProperties.hInstance = CurrentProgramInstance;
    WindowProperties.lpszClassName = "MemoWindowClass";
    
    if (RegisterClass(&WindowProperties))
    {
        //Since CreateWindowEx function expects the TOTAL window size (including pixels for title bar, borders etc.)
        //we will specify the rect size we actually want the game to run in and then us window's AdjustWindowRectEx
        //to properly adjust rect coordinates so no clipping of game window occurs.
        DWORD windowStyles = WS_VISIBLE | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
        RECT rect = { 0, 0, (LONG)globalWindowWidth, (LONG)globalWindowHeight };
        BOOL success = AdjustWindowRectEx(&rect, windowStyles, false, 0);
        if (NOT success)
            InvalidCodePath;
        
        HWND window = CreateWindowEx(0, WindowProperties.lpszClassName, "Shadow Gods", WS_VISIBLE | WS_OVERLAPPEDWINDOW,
                                     CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top, 0, 0, CurrentProgramInstance, 0);
        
        HDC WindowContext = GetDC(window);
        
        if (window && WindowContext)
        {
            
#if DEVELOPMENT_BUILD
            void* baseAddress { (void*)Terabytes(2) };
#else
            void* baseAddress { (void*)0 };
#endif
            
            Game_Input Input {};
            Game_Sound_Output_Buffer SoundBuffer {};
            Rendering_Info renderingInfo {};
            Platform_Services platformServices {};
            Win32::Dbg::Game_Replay_State GameReplayState {};
            Win32::Game_Code GameCode { Win32::Dbg::LoadGameCodeDLL("w:/shadow_gods/build/gamecode.dll") };
            BGZ_ASSERT(GameCode.DLLHandle, "Invalide DLL Handle!");
            
            InitApplicationMemory(&gameMemory, Gigabytes(1), Megabytes(64), VirtualAlloc(baseAddress, Gigabytes(1), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE)); //TODO: Add large page support?)
            
            CreatePartitionFromMemoryBlock($(gameMemory), Megabytes(100), "frame");
            CreatePartitionFromMemoryBlock($(gameMemory), Megabytes(100), "level");
            
            { //Init render command buffer and other render stuff
                void* renderCommandBaseAddress = (void*)(((ui8*)baseAddress) + gameMemory.totalSize + 1);
                renderingInfo.cmdBuffer.baseAddress = (ui8*)VirtualAlloc(renderCommandBaseAddress, Megabytes(5), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
                renderingInfo.cmdBuffer.size = Megabytes(5);
                renderingInfo.cmdBuffer.entryCount = 0;
                renderingInfo.cmdBuffer.usedAmount = 0;
                
                renderingInfo._pixelsPerMeter = globalBackBuffer.height * .10f;
            }
            
            { //Init input recording and replay services
                GameReplayState.RecordedInputs = (Game_Input*)VirtualAlloc(0, (sizeof(Game_Input) * Win32::Dbg::MaxAllowableRecordedInputs), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
                GameReplayState.OriginalRecordedGameState = VirtualAlloc(0, gameMemory.totalSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            }
            
            { //Init game services
                platformServices.WriteEntireFile = &Win32::Dbg::WriteEntireFile;
                platformServices.ReadEntireFile = &Win32::Dbg::ReadEntireFile;
                platformServices.FreeFileMemory = &Win32::Dbg::FreeFileMemory;
                platformServices.LoadBGRAImage = &Win32::Dbg::LoadBGRAImage;
                platformServices.Malloc = &Win32::Dbg::Malloc;
                platformServices.Calloc = &Win32::Dbg::Calloc;
                platformServices.Realloc = &Win32::Dbg::Realloc;
                platformServices.Free = &Win32::Dbg::Free;
                platformServices.AddWorkQueueEntry = &AddToWorkQueue;
                platformServices.FinishAllWork = &FinishAllWork;
            }
            
            ui32 MonitorRefreshRate = bgz::MonitorRefreshHz();
            int GameRefreshRate {};
            f32 TargetSecondsPerFrame {};
            
            switch (MonitorRefreshRate)
            {
                case 30: {
                    GameRefreshRate = MonitorRefreshRate;
                    TargetSecondsPerFrame = 1.0f / (f32)GameRefreshRate;
                }
                break;
                
                case 40: {
                    GameRefreshRate = MonitorRefreshRate;
                    TargetSecondsPerFrame = 1.0f / (f32)GameRefreshRate;
                }
                break;
                
                case 60: {
                    GameRefreshRate = MonitorRefreshRate;
                    TargetSecondsPerFrame = 1.0f / (f32)GameRefreshRate;
                }
                break;
                case 120: {
                    GameRefreshRate = MonitorRefreshRate / 2;
                    TargetSecondsPerFrame = 1.0f / (f32)GameRefreshRate;
                }
                break;
                default:
                InvalidCodePath; //Unkown monitor refresh rate
            };
            
            GameRunning = true;
            
            platformServices.targetFrameTimeInSecs = TargetSecondsPerFrame;
            
            bgz::Timer FramePerformanceTimer {};
            FramePerformanceTimer.Init();
            
            auto UpdateInput = [](Game_Input&& Input, Win32::Dbg::Game_Replay_State&& GameReplayState) -> void {
                for (ui32 ControllerIndex = 0; ControllerIndex < ArrayCount(Input.Controllers); ++ControllerIndex)
                {
                    ClearTransitionCounts(&Input.Controllers[ControllerIndex]);
                }
                
                //Poll Keyboard Input
                Win32::ProcessPendingMessages($(Input), $(GameReplayState));
                
                { //Poll GamePad Input(s)
                    for (DWORD ControllerIndex = 0; ControllerIndex < ArrayCount(Input.Controllers); ++ControllerIndex)
                    {
                        Game_Controller* MyGamePad = &Input.Controllers[ControllerIndex + 1]; //Since index 0 is reserved for keyboard
                        
                        XINPUT_STATE ControllerState;
                        if (XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
                        {
                            if (ControllerIndex == ArrayCount(Input.Controllers)) //Since index 0 is reserved for keyboard
                                break;
                            
                            //This controller is plugged in
                            XINPUT_GAMEPAD* XGamePad = &ControllerState.Gamepad;
                            MyGamePad->IsConnected = true;
                            
                            MyGamePad->LThumbStick.x = Win32::NormalizeAnalogStickValue(XGamePad->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
                            MyGamePad->LThumbStick.y = Win32::NormalizeAnalogStickValue(XGamePad->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
                            
                            if ((MyGamePad->LThumbStick.x != 0.0f) || (MyGamePad->LThumbStick.y != 0.0f))
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
                            
                            Win32::ProcessXInputDigitalButton(XGamePad->wButtons, $(MyGamePad->MoveUp), XINPUT_GAMEPAD_DPAD_UP);
                            Win32::ProcessXInputDigitalButton(XGamePad->wButtons, $(MyGamePad->MoveDown), XINPUT_GAMEPAD_DPAD_DOWN);
                            Win32::ProcessXInputDigitalButton(XGamePad->wButtons, $(MyGamePad->MoveLeft), XINPUT_GAMEPAD_DPAD_LEFT);
                            Win32::ProcessXInputDigitalButton(XGamePad->wButtons, $(MyGamePad->MoveRight), XINPUT_GAMEPAD_DPAD_RIGHT);
                            Win32::ProcessXInputDigitalButton(XGamePad->wButtons, $(MyGamePad->LeftShoulder), XINPUT_GAMEPAD_LEFT_SHOULDER);
                            Win32::ProcessXInputDigitalButton(XGamePad->wButtons, $(MyGamePad->RightShoulder), XINPUT_GAMEPAD_RIGHT_SHOULDER);
                            Win32::ProcessXInputDigitalButton(XGamePad->wButtons, $(MyGamePad->ActionUp), XINPUT_GAMEPAD_Y);
                            Win32::ProcessXInputDigitalButton(XGamePad->wButtons, $(MyGamePad->ActionDown), XINPUT_GAMEPAD_A);
                            Win32::ProcessXInputDigitalButton(XGamePad->wButtons, $(MyGamePad->ActionLeft), XINPUT_GAMEPAD_X);
                            Win32::ProcessXInputDigitalButton(XGamePad->wButtons, $(MyGamePad->ActionRight), XINPUT_GAMEPAD_B);
                            Win32::ProcessXInputDigitalButton(XGamePad->wButtons, $(MyGamePad->Start), XINPUT_GAMEPAD_START);
                            Win32::ProcessXInputDigitalButton(XGamePad->wButtons, $(MyGamePad->Back), XINPUT_GAMEPAD_BACK);
                        }
                        else
                        {
                            //Controller not available
                            MyGamePad->IsConnected = false;
                        }
                    }
                };
            };
            
            while (GameRunning)
            {
                Win32::Window_Dimension windowDimension = Win32::GetWindowDimension(window);
                HDC deviceContext = GetDC(window);
                Win32::ResizeDIBSection($(globalBackBuffer), windowDimension.width, windowDimension.height);
                renderingInfo._pixelsPerMeter = globalBackBuffer.height * .10f;
                
                //Hot reloading
                FILETIME NewGameCodeDLLWriteTime = Win32::Dbg::GetFileTime("w:/shadow_gods/build/gamecode.dll");
                if (CompareFileTime(&NewGameCodeDLLWriteTime, &GameCode.PreviousDLLWriteTime) != 0)
                {
                    Win32::Dbg::FreeGameCodeDLL($(GameCode), $(platformServices));
                    GameCode = Win32::Dbg::LoadGameCodeDLL("w:/shadow_gods/build/gamecode.dll");
                }
                
#if 1
                //helps to prevent overly large detlatimes from getting passed when using debugger and breakpoints
                if (platformServices.prevFrameTimeInSecs > 1.0f / 30.0f)
                    platformServices.prevFrameTimeInSecs = 1.0f / 30.0f;
#endif
                
                //TODO: Should we poll more frequently?
                UpdateInput($(Input), $(GameReplayState));
                
                if (GameReplayState.InputRecording)
                {
                    Win32::Dbg::RecordInput(&Input, $(GameReplayState));
                }
                
                if (GameReplayState.InputPlayBack)
                {
                    Win32::Dbg::PlayBackInput($(Input), $(GameReplayState));
                }
                
                GameCode.UpdateFunc(&gameMemory, &platformServices, &renderingInfo, &SoundBuffer, &Input);
                
                Input = Input;
                GameReplayState = GameReplayState;
                
                f32 SecondsElapsedForWork = FramePerformanceTimer.SecondsElapsed();
                
                if (SecondsElapsedForWork < TargetSecondsPerFrame)
                {
                    DWORD MSToSleep = (DWORD)(1000.0f * (TargetSecondsPerFrame - SecondsElapsedForWork));
                    Sleep(MSToSleep);
                }
                else
                {
                    //BGZ_CONSOLE("Missed our frame rate!!!\n");
                }
                
                Win32::DisplayBufferInWindow($(renderingInfo), deviceContext, windowDimension.width, windowDimension.height, platformServices);
                ReleaseDC(window, deviceContext);
                
                f32 frameTimeInMS = FramePerformanceTimer.MilliSecondsElapsed();
                BGZ_CONSOLE("Frame time: %f\n", frameTimeInMS);
                
                platformServices.prevFrameTimeInSecs = FramePerformanceTimer.SecondsElapsed();
                platformServices.realLifeTimeInSecs += platformServices.prevFrameTimeInSecs;
                
                FramePerformanceTimer.UpdateTimeCount();
            };
            
            //Hardware Rendering shutdown procedure
            wglMakeCurrent(NULL, NULL);
            ReleaseDC(window, WindowContext);
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
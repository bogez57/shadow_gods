#if (DEVELOPMENT_BUILD)
    #define BGZ_LOGGING_ON true
    #define BGZ_ERRHANDLING_ON true
#else
    #define BGZ_LOGGING_ON false
    #define BGZ_ERRHANDLING_ON false
#endif

#include <boagz/error_handling.h>
#include <boagz/error_context.cpp>

#include "gamecode.h"
#include "shared.h"
#include "math.h"

global_variable Platform_Services GlobalPlatformServices;
global_variable Game_Render_Cmds GlobalRenderCmds;
global_variable Game_State* GlobalGameState;
global_variable f32 ViewportWidth;
global_variable f32 ViewportHeight;

local_func auto 
InitMemoryChunk(Memory_Chunk* MemoryChunkToInit, ui64 SizeToReserve, ui64* StartingAddress) -> void
{
    MemoryChunkToInit->BaseAddress = StartingAddress;
    MemoryChunkToInit->Size = SizeToReserve;
    MemoryChunkToInit->UsedMemory = 0;
};

#define PushStruct(MemoryChunk, Type) (Type*)PushStruct_(MemoryChunk, sizeof(Type));
auto
PushStruct_(Memory_Chunk* MemoryChunk, ui64 Size) -> void*
{
    BGZ_ASSERT((MemoryChunk->UsedMemory + Size) <= MemoryChunk->Size);
    void* Result = MemoryChunk->BaseAddress + MemoryChunk->UsedMemory;
    MemoryChunk->UsedMemory += Size;

    return Result;
};

#include "spine2d.h"

extern "C" void
GameUpdate(Game_Memory* GameMemory, Platform_Services PlatformServices, Game_Render_Cmds RenderCmds, 
                    Game_Sound_Output_Buffer* SoundOutput, const Game_Input* GameInput)
{
    Game_State* GameState = (Game_State*)GameMemory->PermanentStorage;
    GlobalGameState = GameState;
    GlobalPlatformServices = PlatformServices;
    GlobalRenderCmds = RenderCmds;

    Camera* GameCamera = &GameState->GameCamera;
    Player* Fighter1 = &GameState->Fighter1;
    Level* GameLevel = &GameState->GameLevel;
    spSkeleton* MySkeleton = GameState->Skeleton;

    const Game_Controller* Keyboard = &GameInput->Controllers[0];
    const Game_Controller* GamePad = &GameInput->Controllers[1];

    if(!GameMemory->IsInitialized)
    {
        InitMemoryChunk(&GameState->Textures, Kilobytes(1), (ui64*)GameMemory->TemporaryStorage);

        GameState->Atlas = spAtlas_createFromFile("data/spineboy.atlas", 0);
        if (GameState->Atlas)
        {
            GameState->SkelBin = spSkeletonJson_create(GameState->Atlas);
            if(GameState->SkelBin)
            {
                GameState->SkelData = spSkeletonJson_readSkeletonDataFile(GameState->SkelBin, "data/spineboy-ess.json");
                if (GameState->SkelData)
                {
                    MySkeleton = spSkeleton_create(GameState->SkelData);
                    if (!MySkeleton)
                    {
                        InvalidCodePath;
                    };
                }
                else
                {
                    InvalidCodePath;
                };
            }
            else
            {
                InvalidCodePath;
            };
        }
        else
        {
            InvalidCodePath;
        };

        GameMemory->IsInitialized = true;
        ViewportWidth = 1280.0f;
        ViewportHeight = 720.0f;

        {//Init Game State
            GameLevel->DisplayImage.Data = PlatformServices.LoadRGBAImage(
                                                            "4k.jpg", 
                                                            &GameLevel->DisplayImage.Dimensions.Width,
                                                            &GameLevel->DisplayImage.Dimensions.Height);

            //TODO: Move out to renderer
            GameLevel->CurrentTexture = RenderCmds.LoadTexture(GameLevel->DisplayImage);

            GameLevel->Dimensions.Width = (f32)GameLevel->DisplayImage.Dimensions.Width;
            GameLevel->Dimensions.Height = (f32)GameLevel->DisplayImage.Dimensions.Height;
            GameLevel->CenterPoint = {(f32)GameLevel->Dimensions.Width / 2, (f32)GameLevel->Dimensions.Height / 2};

            Fighter1->WorldPos.x = GameLevel->CenterPoint.x - 100.0f;
            Fighter1->WorldPos.y = GameLevel->CenterPoint.y - 300.0f;

            GameCamera->ViewWidth = ViewportWidth;
            GameCamera->ViewHeight = ViewportHeight;
            GameCamera->LookAt = GameLevel->CenterPoint;
            GameCamera->ViewCenter = {GameCamera->ViewWidth/2.0f, GameCamera->ViewHeight/2.0f};
            GameCamera->DilatePoint = GameCamera->ViewCenter - v2f{0.0f, 200.0f};
            GameCamera->ZoomFactor = 1.0f;
        };
    }

    MySkeleton = spSkeleton_create(GameState->SkelData);

    if (Keyboard->MoveUp.Pressed)
    {
    }

    if(Keyboard->MoveDown.Pressed)
    {
    }

    if(Keyboard->MoveRight.Pressed)
    {
    }

    if(Keyboard->MoveLeft.Pressed)
    {
    }

    if(Keyboard->ActionUp.Pressed)
    {
    }

    if(Keyboard->ActionDown.Pressed)
    {
    }

    if(Keyboard->ActionRight.Pressed)
    {
    }

    if(Keyboard->ActionLeft.Pressed)
    {
    }

    spSkeleton_updateWorldTransform(MySkeleton);

    {//Render
        RenderCmds.Init();

        RenderCmds.ClearScreen();

        {//Draw Level Background
            Coordinate_System BackgroundWorldSpace{};
            Coordinate_System BackgroundCameraSpace{};
            Drawable_Rect BackgroundCanvas{};

            BackgroundWorldSpace.Origin = {0.0f, 0.0f};

            {//Transform to Camera Space
                v2f TranslationToCameraSpace = GameCamera->ViewCenter - GameCamera->LookAt;
                BackgroundCameraSpace.Origin = BackgroundWorldSpace.Origin + TranslationToCameraSpace;
            };

            BackgroundCanvas = ProduceRectFromBottomLeftPoint(
                                        BackgroundCameraSpace.Origin, 
                                        (f32)GameLevel->Dimensions.Width, 
                                        (f32)GameLevel->Dimensions.Height);

            BackgroundCanvas = DilateAboutArbitraryPoint(GameCamera->DilatePoint, GameCamera->ZoomFactor, BackgroundCanvas);

            RenderCmds.DrawTexture(GameLevel->CurrentTexture.ID, BackgroundCanvas, v2f{0.0f, 0.0f}, v2f{1.0f, 1.0f});
        };

        float verts[8]= {0};
        Texture* texture;
        if(MySkeleton->slots[0]->attachment->type == SP_ATTACHMENT_REGION)
        {
            spRegionAttachment* regionAttachment = (spRegionAttachment*)MySkeleton->slots[0]->attachment;
            texture = (Texture*)((spAtlasRegion*)regionAttachment->rendererObject)->page->rendererObject;
            spRegionAttachment_computeWorldVertices(regionAttachment, MySkeleton->slots[0]->bone, verts, 0, 2);
        };

        Drawable_Rect SpineImage{
                        v2f{verts[0], verts[1]}, 
                        v2f{verts[2], verts[3]}, 
                        v2f{verts[4], verts[5]}, 
                        v2f{verts[6], verts[7]}};

        RenderCmds.DrawTexture(texture->ID, SpineImage, v2f{0.0f, 0.0f}, v2f{1.0f, 1.0f});

        {//Draw Players
            Drawable_Rect CameraSpacePositions{};

            //Draw Images to limbs
            for(i32 LimbIndex{0}; LimbIndex < ArrayCount(Fighter1->Body.Limbs); ++LimbIndex)
            {
                {
                    //Transform to Camera Space
                    v2f TranslationToCameraSpace = GameCamera->ViewCenter - GameCamera->LookAt;
                };
            };
        };
    };
};
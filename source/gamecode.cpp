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
LinkMemoryChunks(Memory_Chunk* ParentChunk, Memory_Chunk* ChildChunk) -> void
{
    ParentChunk->PreviousChunkInMemory = nullptr;
    ChildChunk->PreviousChunkInMemory = ParentChunk;
};

local_func auto 
InitMemoryChunk(Memory_Chunk* MemoryChunkToInit, ui64 SizeToReserve, Game_Memory* GameMemory) -> void
{
    //If First memory chunk of game memory 
    if(!MemoryChunkToInit->PreviousChunkInMemory)
    {
        MemoryChunkToInit->BaseAddress = (ui64*)GameMemory->TemporaryStorage;
        MemoryChunkToInit->EndAddress = MemoryChunkToInit->BaseAddress + SizeToReserve;
        MemoryChunkToInit->UsedAddress = MemoryChunkToInit->BaseAddress;
        MemoryChunkToInit->Size = SizeToReserve;
        MemoryChunkToInit->UsedAmount = 0;
    }
    else
    {
        MemoryChunkToInit->BaseAddress = MemoryChunkToInit->PreviousChunkInMemory->EndAddress;
        MemoryChunkToInit->EndAddress = MemoryChunkToInit->BaseAddress + SizeToReserve;
        MemoryChunkToInit->UsedAddress = MemoryChunkToInit->BaseAddress;
        MemoryChunkToInit->Size = SizeToReserve;
        MemoryChunkToInit->UsedAmount = 0;
    };
};

#define PushType(MemoryChunk, Type, Count) (Type*)_PushType(MemoryChunk, sizeof(Type), Count)
auto
_PushType(Memory_Chunk* MemoryChunk, ui64 Size, sizet Count) -> void*
{
    BGZ_ASSERT((MemoryChunk->UsedAmount + Size) <= MemoryChunk->Size);
    void* Result = MemoryChunk->UsedAddress + MemoryChunk->UsedAmount;
    MemoryChunk->UsedAmount += (Size * Count);

    return Result;
};

//TODO: Right now this only works if nothing new was pushed onto the memory chunk between the previous allocation 
//and this allocation. If there was then this will override it
#define RePushType(MemoryChunk, Ptr, Type, Count) (Type*)_RePushType(MemoryChunk, Ptr, sizeof(Type), Count)
auto
_RePushType(Memory_Chunk* MemoryChunk, void* NewPtr, ui64 Size, sizet Count) -> void*
{
    BGZ_ASSERT(1 == 0)//TODO: Remove eventually - Only here so I know when this is called since I don't think it will work with spine yet
    BGZ_ASSERT((MemoryChunk->UsedAmount + Size) <= MemoryChunk->Size);
    BGZ_ASSERT(NewPtr > MemoryChunk->BaseAddress && NewPtr < MemoryChunk->EndAddress);

    void* Result{nullptr};

    MemoryChunk->UsedAmount -= (MemoryChunk->UsedAddress - (ui64*)NewPtr);
    MemoryChunk->UsedAddress = (ui64*)NewPtr;
    Result = MemoryChunk->UsedAddress;
    MemoryChunk->UsedAmount += (Size + Count);

    return Result;
};

#define Free(MemoryChunk, VALUE) _Free(MemoryChunk, (void*)VALUE)
auto
_Free(Memory_Chunk* MemoryChunk, void* ptrToValue) -> void
{
};

auto
SpineFree() -> void
{
    //This function is used in spine's extension.h file. Since I'm not quite sure yet how I want to deallocate things from a memory 
    //chunk I'm just going to let spine allocate things and never free them for now. 
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
    spSkeleton* MySkeleton = GameState->MySkeleton;

    const Game_Controller* Keyboard = &GameInput->Controllers[0];
    const Game_Controller* GamePad = &GameInput->Controllers[1];

    if(!GameMemory->IsInitialized)
    {
        //These functions current need to be called in this order
        LinkMemoryChunks(&GameState->Spine, &GameState->Game);
        InitMemoryChunk(&GameState->Spine, Megabytes(10), GameMemory);
        InitMemoryChunk(&GameState->Game, Megabytes(10), GameMemory);

        GameState->Atlas = spAtlas_createFromFile("data/spineboy.atlas", 0);
        if (GameState->Atlas)
        {
            GameState->SkelJson = spSkeletonJson_create(GameState->Atlas);
            if(GameState->SkelJson )
            {
                GameState->SkelData = spSkeletonJson_readSkeletonDataFile(GameState->SkelJson, "data/spineboy-ess.json");
                if (GameState->SkelData)
                {
                    spSkeleton *temp{};

                    GameState->MySkeleton = spSkeleton_create(GameState->SkelData);
                    if (GameState->MySkeleton)
                    {
                    }
                    else
                    {
                        InvalidCodePath;
                    }
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

    if (Keyboard->MoveUp.Pressed)
    {
        GameState->MySkeleton->y += 100.0f;
    }

    if(Keyboard->MoveDown.Pressed)
    {
        spBone* upperArm = spSkeleton_findBone(GameState->MySkeleton, "front-upper-arm");
        upperArm->rotation += 30.0f;
    }

    if(Keyboard->MoveRight.Pressed)
    {
        MySkeleton->x += 100.0f;
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

    spSkeleton_updateWorldTransform(GameState->MySkeleton);

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
        };

        for(int SlotIndex{0}; SlotIndex < GameState->MySkeleton->slotsCount; ++SlotIndex)
        {
            float verts[8] = {0};
            Texture *texture;
            spRegionAttachment *regionAttachment;

            //If no current active attachment for slot then continue to next slot
            if(!GameState->MySkeleton->slots[SlotIndex]->attachment) continue;

            if (GameState->MySkeleton->slots[SlotIndex]->attachment->type == SP_ATTACHMENT_REGION)
            {
                regionAttachment = (spRegionAttachment *)GameState->MySkeleton->slots[SlotIndex]->attachment;
                texture = (Texture *)((spAtlasRegion *)regionAttachment->rendererObject)->page->rendererObject;
                spRegionAttachment_computeWorldVertices(regionAttachment, GameState->MySkeleton->slots[SlotIndex]->bone, verts, 0, 2);
            };

            Drawable_Rect SpineImage{
                v2f{verts[0], verts[1]},
                v2f{verts[2], verts[3]},
                v2f{verts[4], verts[5]},
                v2f{verts[6], verts[7]}};

            v2f UVArray[4] = {
                v2f{regionAttachment->uvs[0], regionAttachment->uvs[1]},
                v2f{regionAttachment->uvs[2], regionAttachment->uvs[3]},
                v2f{regionAttachment->uvs[4], regionAttachment->uvs[5]},
                v2f{regionAttachment->uvs[6], regionAttachment->uvs[7]}};

            //Need to try sending down uvs from region attachment to hopefully show correct region of image. Need to modify
            //current DrawTexture func first though to accept proper uvs
            RenderCmds.DrawTexture(texture->ID, SpineImage, UVArray);
        };

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
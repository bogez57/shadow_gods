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

extern "C" void
GameUpdate(Game_Memory* GameMemory, Platform_Services PlatformServices, Game_Render_Cmds RenderCmds, 
                    Game_Sound_Output_Buffer* SoundOutput, const Game_Input* GameInput)
{
    Game_State* GameState = (Game_State*)GameMemory->PermanentStorage;

    Camera* GameCamera = &GameState->GameCamera;
    Player* Fighter1 = &GameState->Fighter1;
    Level* GameLevel = &GameState->GameLevel;

    const Game_Controller* Keyboard = &GameInput->Controllers[0];
    const Game_Controller* GamePad = &GameInput->Controllers[1];

    if(!GameMemory->IsInitialized)
    {
        GameMemory->IsInitialized = true;
        ViewportWidth = 1280.0f;
        ViewportHeight = 720.0f;

        {//Init Game State
            GameLevel->DisplayImage.Data = PlatformServices.LoadRGBAImage(
                                                            "4k.jpg", 
                                                            &GameLevel->DisplayImage.Dimensions.Width,
                                                            &GameLevel->DisplayImage.Dimensions.Height);

            Fighter1->Body.Torso.DisplayImage.Data = PlatformServices.LoadRGBAImage(
                                                     "images/Torso.JPEG", 
                                                     &Fighter1->Body.Torso.DisplayImage.Dimensions.Width,
                                                     &Fighter1->Body.Torso.DisplayImage.Dimensions.Height);
            
            Fighter1->Body.Head.DisplayImage.Data = PlatformServices.LoadRGBAImage(
                                                     "images/Head.JPEG", 
                                                     &Fighter1->Body.Head.DisplayImage.Dimensions.Width,
                                                     &Fighter1->Body.Head.DisplayImage.Dimensions.Height);

            Fighter1->Body.LeftThigh.DisplayImage.Data = PlatformServices.LoadRGBAImage(
                                                     "images/Left Thigh.JPEG", 
                                                     &Fighter1->Body.LeftThigh.DisplayImage.Dimensions.Width,
                                                     &Fighter1->Body.LeftThigh.DisplayImage.Dimensions.Height);
            Fighter1->Body.RightThigh.DisplayImage.Data = PlatformServices.LoadRGBAImage(
                                                     "images/Right Thigh.JPEG", 
                                                     &Fighter1->Body.RightThigh.DisplayImage.Dimensions.Width,
                                                     &Fighter1->Body.RightThigh.DisplayImage.Dimensions.Height);
            Fighter1->Body.RightShoulder.DisplayImage.Data = PlatformServices.LoadRGBAImage(
                                                     "images/Right Shoulder.jpg", 
                                                     &Fighter1->Body.RightShoulder.DisplayImage.Dimensions.Width,
                                                     &Fighter1->Body.RightShoulder.DisplayImage.Dimensions.Height);
            Fighter1->Body.LeftShoulder.DisplayImage.Data = PlatformServices.LoadRGBAImage(
                                                     "images/Left Shoulder.jpg", 
                                                     &Fighter1->Body.LeftShoulder.DisplayImage.Dimensions.Width,
                                                     &Fighter1->Body.LeftShoulder.DisplayImage.Dimensions.Height);
            Fighter1->Body.RightArm.DisplayImage.Data = PlatformServices.LoadRGBAImage(
                                                     "images/Right Arm.jpg", 
                                                     &Fighter1->Body.RightArm.DisplayImage.Dimensions.Width,
                                                     &Fighter1->Body.RightArm.DisplayImage.Dimensions.Height);
            Fighter1->Body.LeftArm.DisplayImage.Data = PlatformServices.LoadRGBAImage(
                                                     "images/Left Arm.jpg", 
                                                     &Fighter1->Body.LeftArm.DisplayImage.Dimensions.Width,
                                                     &Fighter1->Body.LeftArm.DisplayImage.Dimensions.Height);
            Fighter1->Body.Neck.DisplayImage.Data = PlatformServices.LoadRGBAImage(
                                                     "images/Neck.jpg", 
                                                     &Fighter1->Body.Neck.DisplayImage.Dimensions.Width,
                                                     &Fighter1->Body.Neck.DisplayImage.Dimensions.Height);

            //TODO: Move out to renderer
            GameLevel->CurrentTexture = RenderCmds.LoadTexture(GameLevel->DisplayImage);
            Fighter1->Body.Head.CurrentTexture = RenderCmds.LoadTexture(Fighter1->Body.Head.DisplayImage);
            Fighter1->Body.Neck.CurrentTexture = RenderCmds.LoadTexture(Fighter1->Body.Neck.DisplayImage);
            Fighter1->Body.Torso.CurrentTexture = RenderCmds.LoadTexture(Fighter1->Body.Torso.DisplayImage);
            Fighter1->Body.LeftThigh.CurrentTexture = RenderCmds.LoadTexture(Fighter1->Body.LeftThigh.DisplayImage);
            Fighter1->Body.RightThigh.CurrentTexture = RenderCmds.LoadTexture(Fighter1->Body.LeftThigh.DisplayImage);
            Fighter1->Body.RightShoulder.CurrentTexture = RenderCmds.LoadTexture(Fighter1->Body.RightShoulder.DisplayImage);
            Fighter1->Body.LeftShoulder.CurrentTexture = RenderCmds.LoadTexture(Fighter1->Body.LeftShoulder.DisplayImage);
            Fighter1->Body.RightArm.CurrentTexture = RenderCmds.LoadTexture(Fighter1->Body.RightArm.DisplayImage);
            Fighter1->Body.LeftArm.CurrentTexture = RenderCmds.LoadTexture(Fighter1->Body.LeftArm.DisplayImage);

            GameLevel->Dimensions.Width = (f32)GameLevel->DisplayImage.Dimensions.Width;
            GameLevel->Dimensions.Height = (f32)GameLevel->DisplayImage.Dimensions.Height;
            GameLevel->CenterPoint = {(f32)GameLevel->Dimensions.Width / 2, (f32)GameLevel->Dimensions.Height / 2};

            Fighter1->WorldPos.x = GameLevel->CenterPoint.x - 100.0f;
            Fighter1->WorldPos.y = GameLevel->CenterPoint.y - 300.0f;

            for(i32 LimbIndex{0}; LimbIndex < ArrayCount(Fighter1->Body.Limbs); ++LimbIndex) 
            {
                Fighter1->Body.Limbs[LimbIndex].Dimensions.Width = (f32)Fighter1->Body.Limbs[LimbIndex].DisplayImage.Dimensions.Width;
                Fighter1->Body.Limbs[LimbIndex].Dimensions.Height = (f32)Fighter1->Body.Limbs[LimbIndex].DisplayImage.Dimensions.Height;
                Fighter1->Body.Limbs[LimbIndex].Transform.Scale = 1.0f;
                Fighter1->Body.Limbs[LimbIndex].Transform.Rotation = 0.0f;
            };

            Fighter1->Body.RightThigh.Transform.Rotation = -180.0f;
            Fighter1->Body.LeftThigh.Transform.Rotation = 180.0f;
            Fighter1->Body.RightShoulder.Transform.Rotation = -50.0f;
            Fighter1->Body.LeftShoulder.Transform.Rotation = 50.0f;
            Fighter1->Body.RightArm.Transform.Rotation = 180.0f;
            Fighter1->Body.LeftArm.Transform.Rotation = 180.0f;

            Fighter1->Body.Transform.Rotation = 0.0f;
            Fighter1->Body.Transform.Scale = 1.0f;

            Fighter1->Body.Root.Offset = {0.0f, 0.0f};
            Fighter1->Body.Root.Transform.Rotation = 0.0f;
            Fighter1->Body.Root.Transform.Scale = 1.0f;

            GameCamera->ViewWidth = ViewportWidth;
            GameCamera->ViewHeight = ViewportHeight;
            GameCamera->LookAt = GameLevel->CenterPoint;
            GameCamera->ViewCenter = {GameCamera->ViewWidth/2.0f, GameCamera->ViewHeight/2.0f};
            GameCamera->DilatePoint = GameCamera->ViewCenter - v2f{0.0f, 200.0f};
            GameCamera->ZoomFactor = 1.0f;
        };
    }

    Fighter1->Body.Torso.Offset = {0.0f, 0.0f};
    Fighter1->Body.Head.Offset = {0.0f, Fighter1->Body.Neck.Dimensions.Height};
    Fighter1->Body.LeftThigh.Offset = {-50.0f, 0.0f};
    Fighter1->Body.RightThigh.Offset = {50.0f, 0.0f};
    Fighter1->Body.RightShoulder.Offset = {100.0f, 30.0f};
    Fighter1->Body.LeftShoulder.Offset = {-100.0f, 30.0f};
    Fighter1->Body.RightArm.Offset = {50.0f, 50.0f};
    Fighter1->Body.LeftArm.Offset = {-50.0f, 50.0f};
    Fighter1->Body.Neck.Offset = {0.0f, Fighter1->Body.Torso.Dimensions.Height};

    {//Set limb relationships
        Fighter1->Body.Root.Parent = nullptr;
        Fighter1->Body.Torso.Parent = &Fighter1->Body.Root;
        Fighter1->Body.Head.Parent = &Fighter1->Body.Neck;
        Fighter1->Body.Neck.Parent = &Fighter1->Body.Torso;
        Fighter1->Body.LeftThigh.Parent = &Fighter1->Body.Torso;
        Fighter1->Body.RightThigh.Parent = &Fighter1->Body.Torso;
        Fighter1->Body.RightShoulder.Parent = &Fighter1->Body.Neck;
        Fighter1->Body.LeftShoulder.Parent = &Fighter1->Body.Neck;
        Fighter1->Body.RightArm.Parent = &Fighter1->Body.RightShoulder;
        Fighter1->Body.LeftArm.Parent = &Fighter1->Body.LeftShoulder;
    };

    if (Keyboard->MoveUp.Pressed)
    {
        Fighter1->Body.RightShoulder.Transform.Rotation += .3f;
    }

    if(Keyboard->MoveDown.Pressed)
    {
        Fighter1->Body.Neck.Transform.Rotation -= .2f;
    }

    if(Keyboard->MoveRight.Pressed)
    {
        Fighter1->Body.Torso.Transform.Rotation += .2f;
    }

    if(Keyboard->MoveLeft.Pressed)
    {
        Fighter1->Body.Torso.Transform.Rotation -= .2f;
    }

    if(Keyboard->ActionUp.Pressed)
    {
        GameCamera->ZoomFactor += .01f;
    }

    if(Keyboard->ActionDown.Pressed)
    {
        GameCamera->ZoomFactor -= .01f;
    }

    if(Keyboard->ActionRight.Pressed)
    {
    }

    if(Keyboard->ActionLeft.Pressed)
    {
    }

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

        {//Draw Players
            Coordinate_System FighterWorldSpace{};
            Coordinate_System FighterCameraSpace{};
            Drawable_Rect FighterSpacePositions{};

            FighterWorldSpace.Origin = Fighter1->WorldPos;

            //Update limb position vectors to local fighter space
            for(i32 LimbIndex{0}; LimbIndex < ArrayCount(Fighter1->Body.Limbs); ++LimbIndex)
            {
                Limb* FighterLimb = &Fighter1->Body.Limbs[LimbIndex];

                //Limb isn't root as root has no parent
                if(FighterLimb->Parent)
                {
                    FighterLimb->Offset = LinearRotation(FighterLimb->Parent->Transform.Rotation, FighterLimb->Offset);

                    //Translate Limb Origin Point based off parent position
                    FighterLimb->Position = FighterLimb->Parent->Position + FighterLimb->Offset;
                };
            };

            //Draw Images to limbs
            for(i32 LimbIndex{0}; LimbIndex < ArrayCount(Fighter1->Body.Limbs); ++LimbIndex)
            {
                Limb* FighterLimb = &Fighter1->Body.Limbs[LimbIndex];

                v2f LimbSpaceOrigin{0.0f, 0.0f};
                Drawable_Rect LimbSpacePositions = ProduceRectFromBottomMidPoint(
                                                        LimbSpaceOrigin,
                                                        (f32)FighterLimb->Dimensions.Width,
                                                        (f32)FighterLimb->Dimensions.Height);

                //Transform image position vectors to fighter space
                for (ui32 CornerIndex{0}; CornerIndex < ArrayCount(LimbSpacePositions.Corners); ++CornerIndex)
                {
                    FighterSpacePositions.Corners[CornerIndex] = LinearRotation(FighterLimb->Transform.Rotation, LimbSpacePositions.Corners[CornerIndex]);

                    //Translate drawable positions
                    FighterSpacePositions.Corners[CornerIndex] += FighterLimb->Position;
                };

                //Transform image position vectors to world and camera space
                Drawable_Rect WorldSpacePositions{}; Drawable_Rect CameraSpacePositions{}; 
                for(ui32 CornerIndex{0}; CornerIndex < ArrayCount(FighterSpacePositions.Corners); ++CornerIndex)
                {
                    WorldSpacePositions.Corners[CornerIndex] = LinearRotation(Fighter1->Body.Transform.Rotation, FighterSpacePositions.Corners[CornerIndex]);

                    //Translate
                    WorldSpacePositions.Corners[CornerIndex] += FighterWorldSpace.Origin;

                    //Transform to Camera Space
                    v2f TranslationToCameraSpace = GameCamera->ViewCenter - GameCamera->LookAt;
                    CameraSpacePositions.Corners[CornerIndex] = WorldSpacePositions.Corners[CornerIndex] + TranslationToCameraSpace;
                };

                RenderCmds.DrawTexture(Fighter1->Body.Limbs[LimbIndex].CurrentTexture.ID, CameraSpacePositions, v2f{0.0f, 0.0f}, v2f{1.0f, 1.0f});
            };
        };
    };
};
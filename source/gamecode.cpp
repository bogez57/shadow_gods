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
}

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
                                                     "test/Torso.JPEG", 
                                                     &Fighter1->Body.Torso.DisplayImage.Dimensions.Width,
                                                     &Fighter1->Body.Torso.DisplayImage.Dimensions.Height);
            
            Fighter1->Body.Head.DisplayImage.Data = PlatformServices.LoadRGBAImage(
                                                     "test/Head.JPEG", 
                                                     &Fighter1->Body.Head.DisplayImage.Dimensions.Width,
                                                     &Fighter1->Body.Head.DisplayImage.Dimensions.Height);

           /*Fighter1->Body.LeftThigh.DisplayImage.Data = PlatformServices.LoadRGBAImage(
                                                     "test/Left Thigh.JPEG", 
                                                     &Fighter1->Body.LeftThigh.DisplayImage.Dimensions.Width,
                                                     &Fighter1->Body.LeftThigh.DisplayImage.Dimensions.Height);
            Fighter1->Body.RightThigh.DisplayImage.Data = PlatformServices.LoadRGBAImage(
                                                     "test/Right Thigh.JPEG", 
                                                     &Fighter1->Body.RightThigh.DisplayImage.Dimensions.Width,
                                                     &Fighter1->Body.RightThigh.DisplayImage.Dimensions.Height);
                                                     */

            //TODO: Move out to renderer
            GameLevel->CurrentTexture = RenderCmds.LoadTexture(GameLevel->DisplayImage);
            Fighter1->Body.Head.CurrentTexture = RenderCmds.LoadTexture(Fighter1->Body.Head.DisplayImage);
            Fighter1->Body.Torso.CurrentTexture = RenderCmds.LoadTexture(Fighter1->Body.Torso.DisplayImage);

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

            Fighter1->Body.Transform.Rotation = 0.0f;
            Fighter1->Body.Transform.Scale = 1.0f;

            GameCamera->ViewWidth = ViewportWidth;
            GameCamera->ViewHeight = ViewportHeight;
            GameCamera->LookAt = GameLevel->CenterPoint;
            GameCamera->ViewCenter = {GameCamera->ViewWidth/2.0f, GameCamera->ViewHeight/2.0f};
            GameCamera->DilatePoint = GameCamera->ViewCenter - v2f{0.0f, 200.0f};
            GameCamera->ZoomFactor = 1.0f;
        };
    }

    Fighter1->Body.Torso.Offset = {0.0f, 0.0f};
    Fighter1->Body.Head.Offset = {0.0f, Fighter1->Body.Torso.Dimensions.Height};

    {//Set limb relationships
        Fighter1->Body.Torso.Parent = nullptr;
        Fighter1->Body.Torso.Child = &Fighter1->Body.Head;
        Fighter1->Body.Head.Parent = &Fighter1->Body.Torso;
    };

    if (Keyboard->MoveUp.Pressed)
    {
        Fighter1->Body.Head.Transform.Rotation += .2f;
    }

    if(Keyboard->MoveDown.Pressed)
    {
        Fighter1->Body.Head.Transform.Rotation -= .2f;
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

            //Transform to Fighter space
            for(i32 LimbIndex{0}; LimbIndex < ArrayCount(Fighter1->Body.Limbs); ++LimbIndex)
            {
                v2f LimbSpaceOrigin{0.0f, 0.0f};
                v2f FighterSpaceOrigin{0.0f, 0.0f};
                Limb* FighterLimb = &Fighter1->Body.Limbs[LimbIndex];

                Drawable_Rect LimbSpacePositions = ProduceRectFromBottomMidPoint(
                                                        LimbSpaceOrigin,
                                                        (f32)FighterLimb->Dimensions.Width,
                                                        (f32)FighterLimb->Dimensions.Height);

                if(!FighterLimb->Parent)
                {
                    for (ui32 CornerIndex{0}; CornerIndex < ArrayCount(LimbSpacePositions.Corners); ++CornerIndex)
                    {
                        {//Rotate and translate into correct Fighter space positions
                            { //Rotate
                                f32 RotatedAngleInRadians = FighterLimb->Transform.Rotation * (PI / 180.0f);
                                v2f TempFighterCornerPos1 = LimbSpacePositions.Corners[CornerIndex].x * v2f{Cos(RotatedAngleInRadians), Sin(RotatedAngleInRadians)};
                                v2f TempFighterCornerPos2 = LimbSpacePositions.Corners[CornerIndex].y * v2f{-Sin(RotatedAngleInRadians), Cos(RotatedAngleInRadians)};
                                FighterSpacePositions.Corners[CornerIndex] = TempFighterCornerPos1 + TempFighterCornerPos2;
                            };

                            { //Translate
                                FighterSpacePositions.Corners[CornerIndex] += FighterSpaceOrigin;
                                FighterLimb->Position = FighterSpaceOrigin;
                            };
                        };
                    };
                }
                else
                {
                    //Rotate Limb origin point based off parent
                    f32 RotatedAngleInRadians = FighterLimb->Parent->Transform.Rotation * (PI / 180.0f);
                    v2f TempFighterCornerPos1 = FighterLimb->Offset.x * v2f{Cos(RotatedAngleInRadians), Sin(RotatedAngleInRadians)};
                    v2f TempFighterCornerPos2 = FighterLimb->Offset.y * v2f{-Sin(RotatedAngleInRadians), Cos(RotatedAngleInRadians)};
                    FighterLimb->Offset = TempFighterCornerPos1 + TempFighterCornerPos2;

                    //Translate Limb Origin Point based off parent
                    FighterLimb->Position = FighterLimb->Parent->Position + FighterLimb->Offset;

                    for (ui32 CornerIndex{0}; CornerIndex < ArrayCount(LimbSpacePositions.Corners); ++CornerIndex)
                    {
                        {//Rotate and translate into correct Fighter space positions
                            {//Rotate drawable positions
                                RotatedAngleInRadians = FighterLimb->Transform.Rotation * (PI / 180.0f);
                                TempFighterCornerPos1 = LimbSpacePositions.Corners[CornerIndex].x * v2f{Cos(RotatedAngleInRadians), Sin(RotatedAngleInRadians)};
                                TempFighterCornerPos2 = LimbSpacePositions.Corners[CornerIndex].y * v2f{-Sin(RotatedAngleInRadians), Cos(RotatedAngleInRadians)};
                                FighterSpacePositions.Corners[CornerIndex] = TempFighterCornerPos1 + TempFighterCornerPos2;
                            };

                            {//Translate translate drawable positions
                                FighterSpacePositions.Corners[CornerIndex] += FighterLimb->Position;
                            };
                        };
                    };                  
                };

                Drawable_Rect WorldSpacePositions{};
                Drawable_Rect CameraSpacePositions{};
                v2f TranslationToCameraSpace{};

                //Transform to world and then camera space
                for(ui32 CornerIndex{0}; CornerIndex < ArrayCount(FighterSpacePositions.Corners); ++CornerIndex)
                {
                    { //Rotate and translate fighter vectors into World space
                        {//Rotate
                            f32 RotatedAngleInRadians = Fighter1->Body.Transform.Rotation * (PI / 180.0f);
                            v2f TempFighterCornerPos1 = FighterSpacePositions.Corners[CornerIndex].x * v2f{Cos(RotatedAngleInRadians), Sin(RotatedAngleInRadians)};
                            v2f TempFighterCornerPos2 = FighterSpacePositions.Corners[CornerIndex].y * v2f{-Sin(RotatedAngleInRadians), Cos(RotatedAngleInRadians)};
                            WorldSpacePositions.Corners[CornerIndex] = TempFighterCornerPos1 + TempFighterCornerPos2;
                        };

                        { //Translate
                            WorldSpacePositions.Corners[CornerIndex] += FighterWorldSpace.Origin;
                        };
                    };

                    { //Transform to Camera Space
                        TranslationToCameraSpace = GameCamera->ViewCenter - GameCamera->LookAt;
                        CameraSpacePositions.Corners[CornerIndex] = WorldSpacePositions.Corners[CornerIndex] + TranslationToCameraSpace;
                    };
                };

                RenderCmds.DrawTexture(Fighter1->Body.Limbs[LimbIndex].CurrentTexture.ID, CameraSpacePositions, v2f{0.0f, 0.0f}, v2f{1.0f, 1.0f});
            };
        };
    };
};
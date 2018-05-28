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

global_variable float32 ViewportWidth;
global_variable float32 ViewportHeight;

extern "C" void
GameUpdate(Game_Memory* GameMemory, Platform_Services PlatformServices, Game_Render_Cmds RenderCmds, 
                    Game_Sound_Output_Buffer* SoundOutput, const Game_Input* GameInput)
{
    Game_State* GameState = (Game_State*)GameMemory->PermanentStorage;

    const Game_Controller* Keyboard = &GameInput->Controllers[0];
    const Game_Controller* GamePad = &GameInput->Controllers[1];

    if(!GameMemory->IsInitialized)
    {
        GameMemory->IsInitialized = true;
        ViewportWidth = 1280.0f;
        ViewportHeight = 720.0f;

        RenderCmds.Init();

        {//Init Game State
            GameState->GameLevel.BackgroundTexture.ImageData = PlatformServices.LoadRGBAImage(
                                                                                "4k.jpg", 
                                                                                &GameState->GameLevel.BackgroundTexture.Width,
                                                                                &GameState->GameLevel.BackgroundTexture.Height);
            GameState->Fighter.CurrentTexture.ImageData = PlatformServices.LoadRGBAImage(
                                                                                "Fighter.jpg", 
                                                                                &GameState->Fighter.CurrentTexture.Width,
                                                                                &GameState->Fighter.CurrentTexture.Height);

            GameState->GameLevel.BackgroundTexture.ID = RenderCmds.LoadTexture(GameState->GameLevel.BackgroundTexture);
            GameState->Fighter.CurrentTexture.ID = RenderCmds.LoadTexture(GameState->Fighter.CurrentTexture);

            GameState->GameLevel.Width = (float32)GameState->GameLevel.BackgroundTexture.Width;
            GameState->GameLevel.Height = (float32)GameState->GameLevel.BackgroundTexture.Height;
            GameState->GameLevel.CenterPoint = {GameState->GameLevel.Width / 2, GameState->GameLevel.Height / 2};

            GameState->GameCamera.LevelPos = {GameState->GameLevel.Width / 2, GameState->GameLevel.Height / 2};
            GameState->GameCamera.ViewWidth = ViewportWidth;
            GameState->GameCamera.ViewHeight = ViewportHeight;

            GameState->Fighter.LevelPos = {GameState->GameLevel.Width * .45f, GameState->GameLevel.Height * .45f};
            GameState->Fighter.Width = 100.0f;
            GameState->Fighter.Height = 200.0f;
        };
    }

    Camera* GameCamera = &GameState->GameCamera;
    Player* Fighter = &GameState->Fighter;
    Level* GameLevel = &GameState->GameLevel;
    Texture* BackgroundTexture = &GameState->GameLevel.BackgroundTexture;

    GameCamera->ZoomFactor = {0.0f};

    RenderCmds.ClearScreen();

    if(Keyboard->MoveUp.Pressed)
    {
        Fighter->LevelPos.y += 1.0f;
    }

    if(Keyboard->MoveDown.Pressed)
    {
        Fighter->LevelPos.y -= 1.0f;
    }

    if(Keyboard->MoveRight.Pressed)
    {
        Fighter->LevelPos.x += 5.0f;
    }

    if(Keyboard->MoveLeft.Pressed)
    {
        Fighter->LevelPos.x -= 5.0f;
    }

    if(Keyboard->ActionUp.Pressed)
    {
        GameCamera->LevelPos.y += 2.0f;
    }

    if(Keyboard->ActionDown.Pressed)
    {
        GameCamera->LevelPos.y -= 2.0f;
    }

    if(Keyboard->ActionRight.Pressed)
    {
        GameCamera->LevelPos.x += 2.0f;
    }

    if(Keyboard->ActionLeft.Pressed)
    {
        GameCamera->LevelPos.x -= 2.0f;
    }

    {//Render

        {//Draw Level Background
            Rect LevelSpaceCoords = ProduceRectFromCenterPoint(GameLevel->CenterPoint, GameLevel->Width, GameLevel->Height);
            //Draw entire level. Even though points are being generated offscreen (as they are in level space and not 
            //converted to camera/view space), this might make later calculations eaiser to reason about without causing too 
            //much of a performance hit since I'm only drawing 4 points off screen.
            RenderCmds.DrawRect(LevelSpaceCoords.MinPoint, LevelSpaceCoords.MaxPoint);


            Rect BackgroundPoritionToDisplay = ProduceRectFromCenterPoint(GameCamera->LevelPos, 
                                                                          GameCamera->ViewWidth, 
                                                                          GameCamera->ViewHeight);

            vec2 MinUV{BackgroundPoritionToDisplay.MinPoint.x / GameLevel->Width, 
                       BackgroundPoritionToDisplay.MinPoint.y / GameLevel->Height};
            vec2 MaxUV{BackgroundPoritionToDisplay.MaxPoint.x / GameLevel->Width, 
                       BackgroundPoritionToDisplay.MaxPoint.y / GameLevel->Height};

            RenderCmds.DrawTexture(BackgroundTexture->ID, LevelSpaceCoords, MinUV, MaxUV);
        };

        {//Draw Player
            vec2 FighterRelativeDistanceFromCamera{};
            vec2 FighterViewSpacePosition{};

            {//Convert Player world position to camera space position
                FighterRelativeDistanceFromCamera = {GameCamera->LevelPos- Fighter->LevelPos};

                FighterViewSpacePosition = {AbsoluteVal(FighterRelativeDistanceFromCamera.x - (GameCamera->ViewWidth / 2)),
                                            AbsoluteVal(FighterRelativeDistanceFromCamera.y - (GameCamera->ViewHeight / 2))};
            };

            Rect FighterViewSpacePos = ProduceRectFromCenterPoint(FighterViewSpacePosition, Fighter->Width, Fighter->Height);

            RenderCmds.DrawRect(FighterViewSpacePos.MinPoint, FighterViewSpacePos.MaxPoint);

            vec2 MinFighterTextureUV{0.0f, 0.0f};
            vec2 MaxFighterTextureUV{1.0f, 1.0f};
            RenderCmds.DrawTexture(Fighter->CurrentTexture.ID, FighterViewSpacePos, MinFighterTextureUV, MaxFighterTextureUV);
        };
    };
}

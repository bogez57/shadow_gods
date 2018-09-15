/*
    TODO List:

    - convert game units from pixels to meters
*/

#if (DEVELOPMENT_BUILD)
    #define BGZ_LOGGING_ON true
    #define BGZ_ERRHANDLING_ON true
#else
    #define BGZ_LOGGING_ON false
    #define BGZ_ERRHANDLING_ON false
#endif

#define BGZ_MAX_CONTEXTS 10000
#include <boagz/error_handling.h>

#include "gamecode.h"
#include "shared.h"
#include "math.h"
#include "memory_handling.h"

global_variable Platform_Services* GlobalPlatformServices;
global_variable Game_Render_Cmds GlobalRenderCmds;
global_variable Game_State* GlobalGameState;
global_variable f32 ViewportWidth;
global_variable f32 ViewportHeight;

#include "memory_handling.cpp"

//Third Party
#include "spine.cpp"
#include <boagz/error_context.cpp>

local_func auto
MyListener(spAnimationState* state, spEventType type, spTrackEntry* entry, spEvent* event) -> void
{
   switch (type) 
   {
   case SP_ANIMATION_START:
       printf("Animation %s started on track %i\n", entry->animation->name, entry->trackIndex);
       break;
   case SP_ANIMATION_INTERRUPT:
       printf("Animation %s interrupted on track %i\n", entry->animation->name, entry->trackIndex);
       break;
   case SP_ANIMATION_END:
       printf("Animation %s ended on track %i\n", entry->animation->name, entry->trackIndex);
       break;
   case SP_ANIMATION_COMPLETE:
       printf("Animation %s completed on track %i\n", entry->animation->name, entry->trackIndex);
       break;
   case SP_ANIMATION_DISPOSE:
       printf("Track entry for animation %s disposed on track %i\n", entry->animation->name, entry->trackIndex);
       break;
   case SP_ANIMATION_EVENT:
       printf("User defined event for animation %s on track %i\n", entry->animation->name, entry->trackIndex);
       break;
   default:
       printf("Unknown event type: %i", type);
   }
}

local_func auto
ReloadCorrectSpineFunctionPtrs(spSkeletonData* skelData, spAnimationState* animationState) -> void
{
    animationState->listener = MyListener;

    for (i32 animationIndex{0}; animationIndex < skelData->animationsCount; ++animationIndex)
    {
        for (i32 timelineIndex{0}; timelineIndex < skelData->animations[animationIndex]->timelinesCount; ++timelineIndex)
        {
            spTimeline *Timeline = skelData->animations[animationIndex]->timelines[timelineIndex];

            switch (Timeline->type)
            {
            case SP_TIMELINE_ROTATE:
            {
                VTABLE(spTimeline, Timeline)->dispose = _spBaseTimeline_dispose;
                VTABLE(spTimeline, Timeline)->apply = _spRotateTimeline_apply;
                VTABLE(spTimeline, Timeline)->getPropertyId = _spRotateTimeline_getPropertyId;
            }
            break;

            case SP_TIMELINE_SCALE:
            {
                VTABLE(spTimeline, Timeline)->dispose = _spBaseTimeline_dispose;
                VTABLE(spTimeline, Timeline)->apply = _spScaleTimeline_apply;
                VTABLE(spTimeline, Timeline)->getPropertyId = _spScaleTimeline_getPropertyId;
            }
            break;

            case SP_TIMELINE_SHEAR:
            {
                VTABLE(spTimeline, Timeline)->dispose = _spBaseTimeline_dispose;
                VTABLE(spTimeline, Timeline)->apply = _spShearTimeline_apply;
                VTABLE(spTimeline, Timeline)->getPropertyId = _spShearTimeline_getPropertyId;
            }
            break;

            case SP_TIMELINE_TRANSLATE:
            {
                VTABLE(spTimeline, Timeline)->dispose = _spBaseTimeline_dispose;
                VTABLE(spTimeline, Timeline)->apply = _spTranslateTimeline_apply;
                VTABLE(spTimeline, Timeline)->getPropertyId = _spTranslateTimeline_getPropertyId;
            }
            break;

            case SP_TIMELINE_EVENT:
            {
                VTABLE(spTimeline, Timeline)->dispose = _spEventTimeline_dispose;
                VTABLE(spTimeline, Timeline)->apply = _spEventTimeline_apply;
                VTABLE(spTimeline, Timeline)->getPropertyId = _spEventTimeline_getPropertyId;
            }
            break;

            case SP_TIMELINE_ATTACHMENT:
            {
                VTABLE(spTimeline, Timeline)->dispose = _spAttachmentTimeline_dispose;
                VTABLE(spTimeline, Timeline)->apply = _spAttachmentTimeline_apply;
                VTABLE(spTimeline, Timeline)->getPropertyId = _spAttachmentTimeline_getPropertyId;
            }
            break;

            case SP_TIMELINE_COLOR:
            {
                VTABLE(spTimeline, Timeline)->dispose = _spBaseTimeline_dispose;
                VTABLE(spTimeline, Timeline)->apply = _spColorTimeline_apply;
                VTABLE(spTimeline, Timeline)->getPropertyId = _spColorTimeline_getPropertyId;
            }
            break;

            case SP_TIMELINE_DEFORM:
            {
                VTABLE(spTimeline, Timeline)->dispose = _spDeformTimeline_dispose;
                VTABLE(spTimeline, Timeline)->apply = _spDeformTimeline_apply;
                VTABLE(spTimeline, Timeline)->getPropertyId = _spDeformTimeline_getPropertyId;
            }
            break;

            case SP_TIMELINE_IKCONSTRAINT:
            {
                VTABLE(spTimeline, Timeline)->dispose = _spBaseTimeline_dispose;
                VTABLE(spTimeline, Timeline)->apply = _spIkConstraintTimeline_apply;
                VTABLE(spTimeline, Timeline)->getPropertyId = _spIkConstraintTimeline_getPropertyId;
            }
            break;

            case SP_TIMELINE_PATHCONSTRAINTMIX:
            {
                VTABLE(spTimeline, Timeline)->dispose = _spBaseTimeline_dispose;
                VTABLE(spTimeline, Timeline)->apply = _spPathConstraintMixTimeline_apply;
                VTABLE(spTimeline, Timeline)->getPropertyId = _spPathConstraintMixTimeline_getPropertyId;
            }
            break;

            case SP_TIMELINE_PATHCONSTRAINTPOSITION:
            {
                VTABLE(spTimeline, Timeline)->dispose = _spBaseTimeline_dispose;
                VTABLE(spTimeline, Timeline)->apply = _spPathConstraintPositionTimeline_apply;
                VTABLE(spTimeline, Timeline)->getPropertyId = _spPathConstraintPositionTimeline_getPropertyId;
            }
            break;

            case SP_TIMELINE_PATHCONSTRAINTSPACING:
            {
                VTABLE(spTimeline, Timeline)->dispose = _spBaseTimeline_dispose;
                VTABLE(spTimeline, Timeline)->apply = _spPathConstraintSpacingTimeline_apply;
                VTABLE(spTimeline, Timeline)->getPropertyId = _spPathConstraintSpacingTimeline_getPropertyId;
            }
            break;

            case SP_TIMELINE_TRANSFORMCONSTRAINT:
            {
                VTABLE(spTimeline, Timeline)->dispose = _spBaseTimeline_dispose;
                VTABLE(spTimeline, Timeline)->apply = _spTransformConstraintTimeline_apply;
                VTABLE(spTimeline, Timeline)->getPropertyId = _spTransformConstraintTimeline_getPropertyId;
            }
            break;

            case SP_TIMELINE_TWOCOLOR:
            {
                VTABLE(spTimeline, Timeline)->dispose = _spBaseTimeline_dispose;
                VTABLE(spTimeline, Timeline)->apply = _spTwoColorTimeline_apply;
                VTABLE(spTimeline, Timeline)->getPropertyId = _spTwoColorTimeline_getPropertyId;
            }
            break;
            };
        };
    };
};

auto
OnKeyPress(Button_State KeyState, Game_State* gameState, void(*Action)(Game_State*)) -> void
{
    if(KeyState.Pressed && KeyState.NumTransitionsPerFrame)
    {
        Action(gameState);
    };
};

inline auto
OnKeyHold(Button_State KeyState, Game_State* gameState, void(*Action)(Game_State*)) -> void
{
    if(KeyState.Pressed && (KeyState.NumTransitionsPerFrame == 0))
    {
        Action(gameState);
    };
};

inline auto
OnKeyComboPress(Button_State KeyState1, Button_State KeyState2, Game_State* gameState, void(*Action)(Game_State*)) -> void
{
    if(KeyState1.Pressed && KeyState2.Pressed && (KeyState1.NumTransitionsPerFrame || KeyState2.NumTransitionsPerFrame))
    {
        Action(gameState);
    };
};

inline auto
OnKeyComboRepeat(Button_State KeyState1, Button_State KeyState2, Game_State* gameState, void(*Action)(Game_State*)) -> void
{
    if(KeyState1.Pressed && KeyState2.Pressed && (KeyState1.NumTransitionsPerFrame || KeyState2.NumTransitionsPerFrame))
    {
        Action(gameState);
    };
};


inline auto
OnKeyRelease(Button_State KeyState, Game_State* gameState, void(*Action)(Game_State*)) -> void
{
    if(!KeyState.Pressed && KeyState.NumTransitionsPerFrame)
    {
        Action(gameState);
    };
};

extern "C" void
GameUpdate(Game_Memory* GameMemory, Platform_Services* PlatformServices, Game_Render_Cmds RenderCmds, 
                    Game_Sound_Output_Buffer* SoundOutput, Game_Input* GameInput)
{
    BGZ_ERRCTXT1("When entering GameUpdate");

    Game_State* GameState = (Game_State*)GameMemory->PermanentStorage;
    GlobalGameState = GameState;
    GlobalPlatformServices = PlatformServices;
    GlobalRenderCmds = RenderCmds;

    Camera* GameCamera = &GameState->GameCamera;
    Level* GameLevel = &GameState->GameLevel;

    const Game_Controller* Keyboard = &GameInput->Controllers[0];
    const Game_Controller* GamePad = &GameInput->Controllers[1];

    if(!GameMemory->IsInitialized)
    {
        BGZ_ERRCTXT1("When Initializing GameMemory and GameState");

        GameMemory->IsInitialized = true;
        ViewportWidth = 1280.0f;
        ViewportHeight = 720.0f;

        //Split game memory into more specific memory regions
        GameState->DynamAllocator.MemRegions[SPINEDATA] = CreateRegionFromGameMem(GameMemory, Megabytes(10));
        InitDynamAllocatorRegion(&GameState->DynamAllocator, SPINEDATA);

        { //Init spine stuff
            GameState->Atlas = spAtlas_createFromFile("data/spineboy.atlas", 0);
            GameState->SkelJson = spSkeletonJson_create(GameState->Atlas);
            GameState->SkelData = spSkeletonJson_readSkeletonDataFile(GameState->SkelJson, "data/spineboy-ess.json");
            GameState->MySkeleton = spSkeleton_create(GameState->SkelData);
            GameState->AnimationStateData = spAnimationStateData_create(GameState->SkelData);

            spAnimationStateData_setMixByName(GameState->AnimationStateData, "idle", "walk", 0.2f);
            spAnimationStateData_setMixByName(GameState->AnimationStateData, "walk", "idle", 0.2f);
            spAnimationStateData_setMixByName(GameState->AnimationStateData, "walk", "run", 0.2f);
            spAnimationStateData_setMixByName(GameState->AnimationStateData, "run", "idle", 0.2f);

            GameState->AnimationState = spAnimationState_create(GameState->AnimationStateData);

            spAnimationState_setAnimationByName(GameState->AnimationState, 0, "idle", 1);

            GameState->AnimationState->listener = MyListener;
        };

        GameLevel->DisplayImage.Data = PlatformServices->LoadRGBAImage(
                                             "data/4k.jpg",
                                             &GameLevel->DisplayImage.Dimensions.Width,
                                             &GameLevel->DisplayImage.Dimensions.Height);

        //TODO: Move out to renderer
        GameLevel->CurrentTexture = RenderCmds.LoadTexture(GameLevel->DisplayImage);

        //For dll reloading/live code editing purposes
        GameState->SpineFuncPtrTest = _spAttachmentTimeline_apply;
        GameState->EmptyAnim = SP_EMPTY_ANIMATION;

        GameLevel->Dimensions.Width = (f32)GameLevel->DisplayImage.Dimensions.Width;
        GameLevel->Dimensions.Height = (f32)GameLevel->DisplayImage.Dimensions.Height;
        GameLevel->CenterPoint = {(f32)GameLevel->Dimensions.Width / 2, (f32)GameLevel->Dimensions.Height / 2};

        GameCamera->ViewWidth = ViewportWidth;
        GameCamera->ViewHeight = ViewportHeight;
        GameCamera->LookAt = {(ViewportWidth/2.0f), (ViewportHeight/2.0f)};
        GameCamera->ViewCenter = {GameCamera->ViewWidth / 2.0f, GameCamera->ViewHeight / 2.0f};
        GameCamera->DilatePoint = GameCamera->ViewCenter - v2f{0.0f, 200.0f};
        GameCamera->ZoomFactor = 1.0f;
    }

    //In order for live code reloading to work somewhat reliably I need to supply new function addresses
    //for all spine animation timelines upon dll reload. Also, for input playback, since the original game state is copied over to 
    //playback the input this also copies over old function ptr addresses. Currently correcting this by checking when ptr's don't match 
    //(by just picking a random spine func to check) and just reloading func ptr's. This will happen on every loop of playback
    if(GlobalPlatformServices->DLLJustReloaded || GameState->SpineFuncPtrTest != _spAttachmentTimeline_apply)
    {
        BGZ_CONSOLE("Dll reloaded!");
        GameState->SpineFuncPtrTest = _spAttachmentTimeline_apply;
        SP_EMPTY_ANIMATION = GameState->EmptyAnim;
        GlobalPlatformServices->DLLJustReloaded = false;
        ReloadCorrectSpineFunctionPtrs(GameState->SkelData, GameState->AnimationState);
    };

    spAnimationState_update(GameState->AnimationState, .016f);

    OnKeyPress(Keyboard->MoveUp, GameState, [](Game_State* gameState){
        gameState->MySkeleton->y += 10.0f;
    });

    OnKeyPress(Keyboard->MoveRight, GameState, [](Game_State* gameState){
        spAnimationState_setAnimationByName(gameState->AnimationState, 0, "walk", 1);
    });

    OnKeyHold(Keyboard->MoveRight, GameState, [](Game_State* gameState){
        gameState->MySkeleton->x += 3.0f;
    });

    OnKeyComboPress(Keyboard->MoveRight, Keyboard->ActionUp, GameState, [](Game_State* gameState){
        spAnimationState_setAnimationByName(gameState->AnimationState, 0, "run", 1);
    });

    OnKeyRelease(Keyboard->MoveRight, GameState, [](Game_State* gameState){
        spAnimationState_setAnimationByName(gameState->AnimationState, 0, "idle", 1);
    });

    OnKeyHold(Keyboard->MoveDown, GameState, [](Game_State* gameState){
        spAnimationState_addAnimationByName(gameState->AnimationState, 1, "shoot", 0, 0);
    });

    OnKeyRelease(Keyboard->MoveDown, GameState, [](Game_State* gameState){
        spAnimationState_setEmptyAnimation(gameState->AnimationState, 1, .1f);
    });

    spAnimationState_apply(GameState->AnimationState, GameState->MySkeleton);
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
            Texture *texture{};
            spRegionAttachment *regionAttachment{};

            //If no current active attachment for slot then continue to next slot
            if(!GameState->MySkeleton->slots[SlotIndex]->attachment) continue;

            if (GameState->MySkeleton->slots[SlotIndex]->attachment->type == SP_ATTACHMENT_REGION)
            {
                regionAttachment = (spRegionAttachment *)GameState->MySkeleton->slots[SlotIndex]->attachment;
                texture = (Texture *)((spAtlasRegion *)regionAttachment->rendererObject)->page->rendererObject;

                spRegionAttachment_computeWorldVertices(regionAttachment, GameState->MySkeleton->slots[SlotIndex]->bone, verts, 0, 2);

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

                {//Transform to Camera Space
                    v2f TranslationToCameraSpace = GameCamera->ViewCenter - GameCamera->LookAt;

                    for (ui32 VertIndex{0}; VertIndex < ArrayCount(SpineImage.Corners); ++VertIndex)
                    {
                        SpineImage.Corners[VertIndex] += TranslationToCameraSpace;
                    };
                };

                //Need to try sending down uvs from region attachment to hopefully show correct region of image. Need to modify
                //current DrawTexture func first though to accept proper uvs
                RenderCmds.DrawTexture(texture->ID, SpineImage, UVArray);
            };
        };
    };
};
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
ReloadCorrectSpineFunctionPtrs(spSkeletonData* skelData) -> void
{
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
    Player* Fighter1 = &GameState->Fighter1;
    Level* GameLevel = &GameState->GameLevel;
    spSkeleton* MySkeleton = GameState->MySkeleton;

    const Game_Controller* Keyboard = &GameInput->Controllers[0];
    const Game_Controller* GamePad = &GameInput->Controllers[1];

    if(!GameMemory->IsInitialized)
    {
        BGZ_ERRCTXT1("When Initializing GameMemory and GameState");

        GameState->DynamAllocator.MemRegions[SPINEDATA] = CreateRegionFromGameMem(GameMemory, Megabytes(10));
        InitDynamAllocator(&GameState->DynamAllocator, SPINEDATA);

        GameState->Atlas = spAtlas_createFromFile("data/spineboy.atlas", 0);
        GameState->SkelJson = spSkeletonJson_create(GameState->Atlas);
        GameState->SkelData = spSkeletonJson_readSkeletonDataFile(GameState->SkelJson, "data/spineboy-ess.json");
        GameState->MySkeleton = spSkeleton_create(GameState->SkelData);
        GameState->AnimationStateData = spAnimationStateData_create(GameState->SkelData);
        GameState->AnimationState = spAnimationState_create(GameState->AnimationStateData);

        spAnimationState_setAnimationByName(GameState->AnimationState, 0, "walk", 1);

        GameMemory->IsInitialized = true;
        ViewportWidth = 1280.0f;
        ViewportHeight = 720.0f;

        {//Init Game State
            GameLevel->DisplayImage.Data = PlatformServices->LoadRGBAImage(
                                                            "data/4k.jpg", 
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

    //In order for live code reloading to work somewhat reliably I need to supply new function addresses
    //for all spine animation timeline vtables. This is because upon every DLL reload functions can be mapped to 
    //new function addresses. This would mean old spine function pointer addresses would be invalid on DLL
    //reload. Also, for input playback, since the original game state is copied over to playback the input
    //this also copies over old function ptr addresses. Currently correcting this by checking when ptr's don't match and
    //just reloading func ptr's. This will happen on every loop of playback
    if(GlobalPlatformServices->DLLJustReloaded || 
           *VTABLE(spTimeline, GameState->SkelData->animations[0]->timelines[0])->apply != _spRotateTimeline_apply)
    {
        GlobalPlatformServices->DLLJustReloaded = false;
        ReloadCorrectSpineFunctionPtrs(GameState->SkelData);
   };

    spAnimationState_update(GameState->AnimationState, .007f);

    if (Keyboard->MoveUp.Pressed)
    {
        GameState->MySkeleton->y += 80.0f;
    }

    if(Keyboard->MoveDown.Pressed)
    {
        spBone* upperArm = spSkeleton_findBone(GameState->MySkeleton, "front-upper-arm");
        upperArm->rotation += 15.0f;
    }

    if(Keyboard->MoveRight.Pressed)
    {
        MySkeleton->x += 1.0f;
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
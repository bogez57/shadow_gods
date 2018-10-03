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

#define _CRT_SECURE_NO_WARNINGS // to surpress things like 'use printf_s func instead!'

#define BGZ_MAX_CONTEXTS 10000
#include <boagz/error_handling.h>

#include "gamecode.h"
#include "math.h"
#include "shared.h"
#include "memory_handling.h"

global_variable Platform_Services* globalPlatformServices;
global_variable Game_Render_Cmds globalRenderCmds;
global_variable Memory_Handler* globalMemHandler;
global_variable f32 deltaT;
global_variable f32 viewportWidth;
global_variable f32 viewportHeight;

#include "memory_handling.cpp"
#include "memory_allocators.cpp"
// Third Party
#include "spine.cpp"
#include <boagz/error_context.cpp>

local_func auto FlipImage(Image image) -> Image
{
    i32  widthInBytes = image.size.width * 4;
    ui8* p_topRowOfTexels = nullptr;
    ui8* p_bottomRowOfTexels = nullptr;
    ui8  temp = 0;
    i32  halfHeight = image.size.height / 2;

    for (i32 row = 0; row < halfHeight; ++row)
    {
        p_topRowOfTexels = image.Data + row * widthInBytes;
        p_bottomRowOfTexels = image.Data + (image.size.height - row - 1) * widthInBytes;

        for (i32 col = 0; col < widthInBytes; ++col)
        {
            temp = *p_topRowOfTexels;
            *p_topRowOfTexels = *p_bottomRowOfTexels;
            *p_bottomRowOfTexels = temp;
            p_topRowOfTexels++;
            p_bottomRowOfTexels++;
        };
    };

    return image;
};

local_func auto MyListener(spAnimationState* state, spEventType type, spTrackEntry* entry, spEvent* event) -> void
{
    switch (type)
    {
    case SP_ANIMATION_START:
    {
        printf("Animation %s started on track %i\n", entry->animation->name, entry->trackIndex);
        break;
    }
    case SP_ANIMATION_INTERRUPT:
    {
        printf("Animation %s interrupted on track %i\n", entry->animation->name, entry->trackIndex);
        break;
    }
    case SP_ANIMATION_END:
    {
        printf("Animation %s ended on track %i\n", entry->animation->name, entry->trackIndex);
        break;
    }
    case SP_ANIMATION_COMPLETE:
    {
        printf("Animation %s completed on track %i\n", entry->animation->name, entry->trackIndex);
        break;
    }
    case SP_ANIMATION_DISPOSE:
    {
        printf("Track entry for animation %s disposed on track %i\n",
            entry->animation->name,
            entry->trackIndex);
        break;
    }
    case SP_ANIMATION_EVENT:
    {
        printf("User defined event for animation %s on track %i\n",
            entry->animation->name,
            entry->trackIndex);
        break;
    }
    default:
        printf("Unknown event type: %i", type);
    }
}

local_func auto ReloadAllSpineTimelineFunctionPtrs(spSkeletonData skelData) -> spSkeletonData
{
    for (i32 animationIndex{ 0 }; animationIndex < skelData.animationsCount;
         ++animationIndex)
    {
        for (i32 timelineIndex{ 0 };
             timelineIndex < skelData.animations[animationIndex]->timelinesCount;
             ++timelineIndex)
        {
            spTimeline* Timeline = skelData.animations[animationIndex]->timelines[timelineIndex];

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

            case SP_TIMELINE_DRAWORDER:
            {
                // Adding this currently to satisfy
                // clang compiler warning. Might need to
                // implement in future
            }
            break;
            };
        };
    };

    return skelData;
};

auto UpdateMainCollisionBoxOnFighter(Fighter fighter) -> Fighter
{
    f32 halfFighterWidth = (fighter.skeleton->data->width / 2.0f) * AbsoluteVal(fighter.skeleton->scaleX);
    f32 fighterHeight = fighter.skeleton->data->height * AbsoluteVal(fighter.skeleton->scaleY);
    fighter.collisionBox.minCorner = { fighter.worldPos.x - halfFighterWidth,
        fighter.worldPos.y };
    fighter.collisionBox.maxCorner = { fighter.worldPos.x + halfFighterWidth,
        fighter.worldPos.y + fighterHeight };

    return fighter;
};

auto CheckForFighterCollisions(AABB fighter1Box, AABB fighter2Box) -> b
{
    // Exit returning NO intersection between bounding boxes
    if (fighter1Box.maxCorner.x < fighter2Box.minCorner.x || fighter1Box.minCorner.x > fighter2Box.maxCorner.x)
    {
        return false;
    }

    // Exit returning NO intersection between bounding boxes
    if (fighter1Box.maxCorner.y < fighter2Box.minCorner.y || fighter1Box.minCorner.y > fighter2Box.maxCorner.y)
    {
        return false;
    }

    // Else intersection and thus collision has occured!
    return true;
};

auto OnKeyPress(Button_State KeyState, Stage_Data* stage, void (*Action)(Stage_Data*)) -> void
{
    if (KeyState.Pressed && KeyState.NumTransitionsPerFrame)
    {
        Action(stage);
    };
};

inline auto OnKeyHold(Button_State KeyState, Stage_Data* stage, void (*Action)(Stage_Data*)) -> void
{
    if (KeyState.Pressed && (KeyState.NumTransitionsPerFrame == 0))
    {
        Action(stage);
    };
};

inline auto OnKeyComboPress(Button_State KeyState1, Button_State KeyState2, Stage_Data* stage, void (*Action)(Stage_Data*)) -> void
{
    if (KeyState1.Pressed && KeyState2.Pressed && (KeyState1.NumTransitionsPerFrame || KeyState2.NumTransitionsPerFrame))
    {
        Action(stage);
    };
};

inline auto OnKeyComboRepeat(Button_State KeyState1, Button_State KeyState2, Stage_Data* stage, void (*Action)(Stage_Data*)) -> void
{
    if (KeyState1.Pressed && KeyState2.Pressed && (KeyState1.NumTransitionsPerFrame || KeyState2.NumTransitionsPerFrame))
    {
        Action(stage);
    };
};

inline auto OnKeyRelease(Button_State KeyState, Stage_Data* stage, void (*Action)(Stage_Data*)) -> void
{
    if (!KeyState.Pressed && KeyState.NumTransitionsPerFrame)
    {
        Action(stage);
    };
};

extern "C" void GameUpdate(Game_Memory* gameMemory, Platform_Services* platformServices, Game_Render_Cmds renderCmds, Game_Sound_Output_Buffer* soundOutput, Game_Input* gameInput)
{
    BGZ_ERRCTXT1("When entering GameUpdate");

    Game_State* gameState = (Game_State*)gameMemory->PermanentStorage;
    globalMemHandler = &gameState->memHandler;
    globalPlatformServices = platformServices;
    globalRenderCmds = renderCmds;

    deltaT = platformServices->prevFrameTimeInSecs;
    Stage_Data* stage = &gameState->stage;
    Fighter*    player = &stage->player;
    Fighter*    ai = &stage->ai;

    const Game_Controller* keyboard = &gameInput->Controllers[0];
    const Game_Controller* gamePad = &gameInput->Controllers[1];

    if (!gameMemory->IsInitialized)
    {
        BGZ_ERRCTXT1("When Initializing game memory and game state");

        gameMemory->IsInitialized = true;
        viewportWidth = 1280.0f;
        viewportHeight = 720.0f;

        // Make sure everything is initialized
        gameState->stage = {};
        gameState->memHandler = {};
        gameState->emptyAnim = {};

        // Split game memory into more specific memory regions
        {
            auto [updatedGameMemory, dynamicMemRegion] = CreateRegionFromGameMem(*gameMemory, Megabytes(10));
            *gameMemory = updatedGameMemory;
            gameState->memHandler.memRegions[DYNAMIC] = dynamicMemRegion;

            gameState->memHandler.dynamAllocator = CreateAndInitDynamAllocator();
        };

        stage->info.displayImage.Data = platformServices->LoadRGBAImage("data/4k.jpg", &stage->info.displayImage.size.width, &stage->info.displayImage.size.height);

        // Since opengl will read-in image upside down
        stage->info.displayImage = FlipImage(stage->info.displayImage);
        stage->info.currentTexture = renderCmds.LoadTexture(stage->info.displayImage); // TODO: Move out to renderer

        // For dll reloading/live code editing purposes
        gameState->SpineFuncPtrTest = _spAttachmentTimeline_apply;
        gameState->emptyAnim = SP_EMPTY_ANIMATION;

        stage->info.size.width = (f32)stage->info.displayImage.size.width;
        stage->info.size.height = (f32)stage->info.displayImage.size.height;
        stage->info.centerPoint = { (f32)stage->info.size.width / 2, (f32)stage->info.size.height / 2 };

        { // Set stage camera
            stage->camera.viewWidth = viewportWidth;
            stage->camera.viewHeight = viewportHeight;
            stage->camera.lookAt = { stage->info.centerPoint.x, stage->info.centerPoint.y - 600.0f };
            stage->camera.viewCenter = { stage->camera.viewWidth / 2.0f, stage->camera.viewHeight / 2.0f };
            stage->camera.dilatePoint = stage->camera.viewCenter - v2f{ 0.0f, 200.0f };
            stage->camera.zoomFactor = 1.0f;
        };

        { // Init spine stuff
            BGZ_ERRCTXT1("When Initializing Spine stuff");

            spAtlas*        atlas = spAtlas_createFromFile("data/hero.atlas", 0);
            spSkeletonJson* skelJson = spSkeletonJson_create(atlas);
            stage->commonSkeletonData = spSkeletonJson_readSkeletonDataFile(skelJson, "data/hero-ess.json");
            stage->commonAnimationData = spAnimationStateData_create(stage->commonSkeletonData);
            spSkeletonJson_dispose(skelJson);

            spAnimationStateData_setMixByName(stage->commonAnimationData, "idle", "walk", 0.2f);
            spAnimationStateData_setMixByName(stage->commonAnimationData, "walk", "idle", 0.2f);
            spAnimationStateData_setMixByName(stage->commonAnimationData, "walk", "run", 0.2f);
            spAnimationStateData_setMixByName(stage->commonAnimationData, "run", "idle", 0.2f);
            spAnimationStateData_setMixByName(stage->commonAnimationData, "attack", "idle", 0.2f);

            { // Setup fighters
                player->skeleton = spSkeleton_create(stage->commonSkeletonData);
                player->animationState = spAnimationState_create(stage->commonAnimationData);
                spAnimationState_setAnimationByName(player->animationState, 0, "idle", 1);
                player->animationState->listener = MyListener;
                player->worldPos = { (stage->info.size.width / 2.0f) - 300.0f, (stage->info.size.height / 2.0f) - 900.0f };
                player->skeleton->scaleX = 0.6f;
                player->skeleton->scaleY = 0.6f;
                *player = UpdateMainCollisionBoxOnFighter(*player);

                ai->skeleton = spSkeleton_create(stage->commonSkeletonData);
                ai->animationState = spAnimationState_create(stage->commonAnimationData);
                spAnimationState_setAnimationByName(ai->animationState, 0, "idle", 1);
                ai->worldPos = { (stage->info.size.width / 2.0f) + 300.0f, (stage->info.size.height / 2.0f) - 900.0f };
                ai->skeleton->scaleX = -0.6f; // Flip ai fighter to start
                ai->skeleton->scaleY = 0.6f;
                *ai = UpdateMainCollisionBoxOnFighter(*ai);
            };
        };
    };

    if (globalPlatformServices->DLLJustReloaded || gameState->SpineFuncPtrTest != _spAttachmentTimeline_apply)
    {
        BGZ_CONSOLE("Dll reloaded!");

        { // Perform necessary operations to keep spine working with
            // live code editing/input playback
            gameState->SpineFuncPtrTest = _spAttachmentTimeline_apply;
            SP_EMPTY_ANIMATION = gameState->emptyAnim;
            globalPlatformServices->DLLJustReloaded = false;
            *stage->commonSkeletonData = ReloadAllSpineTimelineFunctionPtrs(*stage->commonSkeletonData);
            player->animationState->listener = MyListener;
        };
    };

    spAnimationState_update(player->animationState, deltaT);
    spAnimationState_update(ai->animationState, deltaT);

    OnKeyPress(keyboard->MoveUp, stage, [](Stage_Data* stage) {
        spAnimationState_setAnimationByName(stage->player.animationState, 0, "attack", 0);
    });

    OnKeyRelease(keyboard->MoveUp, stage, [](Stage_Data* stage) {
        spAnimationState_setAnimationByName(stage->player.animationState, 0, "idle", 1);
    });

    OnKeyPress(keyboard->MoveRight, stage, [](Stage_Data* stage) {
        spAnimationState_setAnimationByName(stage->player.animationState, 0, "walk", 1);
    });

    OnKeyHold(keyboard->MoveRight, stage, [](Stage_Data* stage) { stage->player.worldPos.x += 1.0f; });

    OnKeyComboPress(keyboard->MoveRight, keyboard->ActionUp, stage, [](Stage_Data* stage) {
        spAnimationState_setAnimationByName(stage->player.animationState, 0, "run", 1);
    });

    OnKeyRelease(keyboard->MoveRight, stage, [](Stage_Data* stage) {
        spAnimationState_setAnimationByName(stage->player.animationState, 0, "idle", 1);
    });

    OnKeyHold(keyboard->MoveDown, stage, [](Stage_Data* stage) {
        // spAnimationState_addAnimationByName(stage->commonAnimationState,
        // 1, "shoot", 0, 0);
    });

    OnKeyRelease(keyboard->MoveDown, stage, [](Stage_Data* stage) {
        // spAnimationState_setEmptyAnimation(stage->commonAnimationState,
        // 1, .1f);
    });

    *player = UpdateMainCollisionBoxOnFighter(*player);
    *ai = UpdateMainCollisionBoxOnFighter(*ai);

    b collision = CheckForFighterCollisions(player->collisionBox, ai->collisionBox);
    if (collision)
    {
        BGZ_CONSOLE("Colllision occured!");
    };

    // Needed for spine to correctly update bones
    player->skeleton->x = player->worldPos.x;
    player->skeleton->y = player->worldPos.y;
    ai->skeleton->x = ai->worldPos.x;
    ai->skeleton->y = ai->worldPos.y;

    spAnimationState_apply(player->animationState, player->skeleton);
    spSkeleton_updateWorldTransform(player->skeleton);
    spAnimationState_apply(ai->animationState, ai->skeleton);
    spSkeleton_updateWorldTransform(ai->skeleton);

    { // Render
        renderCmds.Init();

        renderCmds.ClearScreen();

        { // Draw Level Background
            Coordinate_System backgroundWorldSpace{};
            Coordinate_System backgroundCameraSpace{};
            Drawable_Rect     backgroundCanvas{};

            backgroundWorldSpace.Origin = { 0.0f, 0.0f };

            { // Transform to Camera Space
                v2f translationToCameraSpace = stage->camera.viewCenter - stage->camera.lookAt;
                backgroundCameraSpace.Origin = backgroundWorldSpace.Origin + translationToCameraSpace;
            };

            backgroundCanvas = ProduceRectFromBottomLeftPoint(backgroundCameraSpace.Origin, (f32)stage->info.size.width, (f32)stage->info.size.height);

            backgroundCanvas = DilateAboutArbitraryPoint(stage->camera.dilatePoint, stage->camera.zoomFactor, backgroundCanvas);

            renderCmds.DrawBackground(stage->info.currentTexture.ID, backgroundCanvas, v2f{ 0.0f, 0.0f }, v2f{ 1.0f, 1.0f });
        };

        Fighter* fighters[2] = { player, ai };
        for (i32 FighterIndex{ 0 }; FighterIndex < ArrayCount(fighters); ++FighterIndex)
        {
            for (i32 SlotIndex{ 0 }; SlotIndex < fighters[FighterIndex]->skeleton->slotsCount; ++SlotIndex)
            {
                float               verts[8] = { 0 };
                Texture*            texture{};
                spRegionAttachment* regionAttachment{};
                spSkeleton*         skeleton = fighters[FighterIndex]->skeleton;

                // If no current active attachment for
                // slot then continue to next slot
                if (!skeleton->slots[SlotIndex]->attachment)
                    continue;

                if (skeleton->slots[SlotIndex]->attachment->type == SP_ATTACHMENT_REGION)
                {
                    regionAttachment = (spRegionAttachment*)skeleton->slots[SlotIndex]->attachment;
                    texture = (Texture*)((spAtlasRegion*)regionAttachment->rendererObject)->page->rendererObject;

                    spRegionAttachment_computeWorldVertices(regionAttachment, skeleton->slots[SlotIndex]->bone, verts, 0, 2);

                    Drawable_Rect spineImage{
                        v2f{ verts[0], verts[1] },
                        v2f{ verts[2], verts[3] },
                        v2f{ verts[4], verts[5] },
                        v2f{ verts[6], verts[7] }
                    };

                    v2f UVArray[4] = {
                        v2f{ regionAttachment->uvs[0], regionAttachment->uvs[1] },
                        v2f{ regionAttachment->uvs[2], regionAttachment->uvs[3] },
                        v2f{ regionAttachment->uvs[4], regionAttachment->uvs[5] },
                        v2f{ regionAttachment->uvs[6], regionAttachment->uvs[7] }
                    };

                    { // Transform to Camera Space
                        v2f translationToCameraSpace = stage->camera.viewCenter - stage->camera.lookAt;

                        for (ui32 VertIndex{ 0 }; VertIndex < ArrayCount(spineImage.Corners);
                             ++VertIndex)
                        {
                            spineImage.Corners[VertIndex] += translationToCameraSpace;
                        };
                    };

                    spineImage = DilateAboutArbitraryPoint(stage->camera.dilatePoint, stage->camera.zoomFactor, spineImage);

                    renderCmds.DrawTexture(texture->ID, spineImage, UVArray);
                };
            };
        };
    };
};
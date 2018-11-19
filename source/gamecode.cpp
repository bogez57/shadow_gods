/*
    TODO List:

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

#include "atomic_types.h"
#include "shared.h"
#include "memory_handling.h"
#include "dynamic_array.h"
#include "gamecode.h"
#include "math.h"
#include "utilities.h"

global_variable Platform_Services* globalPlatformServices;
global_variable Game_Render_Cmds globalRenderCmds;
global_variable Memory_Handler* globalMemHandler;
global_variable f32 deltaT;
global_variable f32 viewportWidth;
global_variable f32 viewportHeight;
global_variable Animation rightCrossAnim;
global_variable Animation leftJabAnim;
global_variable Animation rightUpperCutAnim;
global_variable Animation highKickAnim;

#include "memory_handling.cpp"
#include "memory_allocators.cpp"
#include "collisions.cpp"

// Third Party
#include "spine.cpp"
#include <boagz/error_context.cpp>

local_func auto FlipImage(Image image) -> Image
{
    i32 widthInBytes = image.size.width * 4;
    ui8* p_topRowOfTexels = nullptr;
    ui8* p_bottomRowOfTexels = nullptr;
    ui8 temp = 0;
    i32 halfHeight = image.size.height / 2;

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

//Set to animationState->listener
local_func auto SpineEventCallBack(spAnimationState* state, spEventType type, spTrackEntry* entry, spEvent* event) -> void
{
    switch (type)
    {
    case SP_ANIMATION_START:
    {
        break;
    }
    case SP_ANIMATION_INTERRUPT:
    {
        break;
    }
    case SP_ANIMATION_END:
    {
        break;
    }
    case SP_ANIMATION_COMPLETE:
    {
        break;
    }
    case SP_ANIMATION_DISPOSE:
    {
        break;
    }
    case SP_ANIMATION_EVENT:
    {
        break;
    }
    default:
        printf("Unknown event type: %i", type);
    }
}

local_func auto ReloadAllSpineTimelineFunctionPtrs(spSkeletonData skelData) -> spSkeletonData
{
    for (i32 animationIndex { 0 }; animationIndex < skelData.animationsCount;
         ++animationIndex)
    {
        for (i32 timelineIndex { 0 };
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

inline b KeyPressed(Button_State KeyState)
{
    if (KeyState.Pressed && KeyState.NumTransitionsPerFrame)
    {
        return true;
    };

    return false;
};

inline b KeyComboPressed(Button_State KeyState1, Button_State KeyState2)
{
    if (KeyState1.Pressed && KeyState2.Pressed && (KeyState1.NumTransitionsPerFrame || KeyState2.NumTransitionsPerFrame))
    {
        return true;
    };

    return false;
};

inline b KeyHeld(Button_State KeyState)
{
    if (KeyState.Pressed && (KeyState.NumTransitionsPerFrame == 0))
    {
        return true;
    };

    return false;
};

inline b KeyComboHeld(Button_State KeyState1, Button_State KeyState2)
{
    if (KeyState1.Pressed && KeyState2.Pressed && (KeyState1.NumTransitionsPerFrame == 0 && KeyState2.NumTransitionsPerFrame == 0))
    {
        return true;
    };

    return false;
};

inline b KeyReleased(Button_State KeyState)
{
    if (!KeyState.Pressed && KeyState.NumTransitionsPerFrame)
    {
        return true;
    };

    return false;
};

extern "C" void GameUpdate(Game_Memory* gameMemory, Platform_Services* platformServices, Game_Render_Cmds renderCmds, Game_Sound_Output_Buffer* soundOutput, Game_Input* gameInput)
{
    BGZ_ERRCTXT1("When entering GameUpdate");

    Game_State* gameState = (Game_State*)gameMemory->PermanentStorage;
    deltaT = platformServices->prevFrameTimeInSecs;

    globalMemHandler = &gameState->memHandler;
    globalPlatformServices = platformServices;
    globalRenderCmds = renderCmds;

    Stage_Data* stage = &gameState->stage;
    Fighter* player = &stage->player;
    Fighter* ai = &stage->ai;

    const Game_Controller* keyboard = &gameInput->Controllers[0];
    const Game_Controller* gamePad = &gameInput->Controllers[1];

    if (!gameMemory->IsInitialized)
    {
        BGZ_ERRCTXT1("When Initializing game memory and game state");

        gameMemory->IsInitialized = true;
        viewportWidth = 1280.0f;
        viewportHeight = 720.0f;

        { // Split game memory into more specific memory regions
            gameState->memHandler.memRegions[DYNAMIC] = CreateRegionFromGameMem_1(gameMemory, Megabytes(10));

            InitDynamAllocator_1(&gameState->memHandler.dynamAllocator);
        };

        stage->info.displayImage.Data = platformServices->LoadRGBAImage("data/4k.jpg", &stage->info.displayImage.size.width, &stage->info.displayImage.size.height);

        // Since opengl will read-in image upside down
        stage->info.displayImage = FlipImage(stage->info.displayImage);
        stage->info.currentTexture = renderCmds.LoadTexture(stage->info.displayImage); // TODO: Move out to renderer

        stage->info.size.width = (f32)stage->info.displayImage.size.width;
        stage->info.size.height = (f32)stage->info.displayImage.size.height;
        stage->info.centerPoint = { (f32)stage->info.size.width / 2, (f32)stage->info.size.height / 2 };

        { // Set stage camera
            stage->camera.viewWidth = viewportWidth;
            stage->camera.viewHeight = viewportHeight;
            stage->camera.lookAt = { stage->info.centerPoint.x, stage->info.centerPoint.y - 600.0f };
            stage->camera.viewCenter = { stage->camera.viewWidth / 2.0f, stage->camera.viewHeight / 2.0f };
            stage->camera.dilatePoint = stage->camera.viewCenter - v2f { 0.0f, 200.0f };
            stage->camera.zoomFactor = 1.0f;
        };

        { // Init spine stuff
            BGZ_ERRCTXT1("When Initializing Spine stuff");

            spAtlas* atlas = spAtlas_createFromFile("data/yellow_god.atlas", 0);
            spSkeletonJson* skelJson = spSkeletonJson_create(atlas);
            stage->commonSkeletonData = spSkeletonJson_readSkeletonDataFile(skelJson, "data/yellow_god.json");
            stage->commonAnimationData = spAnimationStateData_create(stage->commonSkeletonData);
            spSkeletonJson_dispose(skelJson);
        };

        { // Setup fighters
            InitFighter_1(player, v2f { (stage->info.size.width / 2.0f) - 300.0f, (stage->info.size.height / 2.0f) - 900.0f }, spSkeleton_create(stage->commonSkeletonData), spAnimationState_create(stage->commonAnimationData), v2f { .6f, .6f }, v2f { 100.0f, 300.0f });
            InitFighter_1(ai, v2f { (stage->info.size.width / 2.0f) + 300.0f, (stage->info.size.height / 2.0f) - 900.0f }, spSkeleton_create(stage->commonSkeletonData), spAnimationState_create(stage->commonAnimationData), v2f { -.6f, .6f }, v2f { 100.0f, 300.0f });

            spAnimationState_setAnimationByName(player->animationState, 0, "idle", 1);
            spAnimationState_setAnimationByName(ai->animationState, 0, "idle", 1);
        };

        { //Setup animation stuff
            rightCrossAnim.baseAnimation = spSkeletonData_findAnimation(stage->commonSkeletonData, "right_cross");
            leftJabAnim.baseAnimation = spSkeletonData_findAnimation(stage->commonSkeletonData, "left_jab");
            rightUpperCutAnim.baseAnimation = spSkeletonData_findAnimation(stage->commonSkeletonData, "right_uppercut");
            highKickAnim.baseAnimation = spSkeletonData_findAnimation(stage->commonSkeletonData, "high_kick");

            rightCrossAnim.hitBoxCenter = { 340.11f, 753.74f };
        };

        // For dll reloading/live code editing purposes
        gameState->SpineFuncPtrTest = _spAttachmentTimeline_apply;
        gameState->emptyAnim = SP_EMPTY_ANIMATION;
    };

    if (globalPlatformServices->DLLJustReloaded || gameState->SpineFuncPtrTest != _spAttachmentTimeline_apply)
    {
        BGZ_CONSOLE("Dll reloaded!");

        { //Perform necessary operations to keep spine working with live code editing/input playback
            gameState->SpineFuncPtrTest = _spAttachmentTimeline_apply;
            SP_EMPTY_ANIMATION = gameState->emptyAnim;
            globalPlatformServices->DLLJustReloaded = false;
            *stage->commonSkeletonData = ReloadAllSpineTimelineFunctionPtrs(*stage->commonSkeletonData);
            player->animationState->listener = SpineEventCallBack;
        };
    };

    spAnimationState_update(player->animationState, deltaT);
    spAnimationState_update(ai->animationState, deltaT);

    if (KeyComboPressed(keyboard->ActionLeft, keyboard->MoveRight))
    {
        spAnimationState_addAnimation(stage->player.animationState, 0, highKickAnim.baseAnimation, 0, 0.0f);
    }
    else if (KeyPressed(keyboard->ActionLeft))
    {
        local_persist ui32 currentActionComboMove {};
        local_persist spTrackEntry* currentAnimTrackEntry { CallocType(spTrackEntry, 1) };

        ui32 const comboMove1 { 0 };
        ui32 const comboMove2 { 1 };
        ui32 const comboMove3 { 2 };

        if (spTrackEntry_getAnimationTime(currentAnimTrackEntry) == currentAnimTrackEntry->animationEnd)
        {
            currentActionComboMove = comboMove1;
        }

        switch (currentActionComboMove)
        {
        case comboMove1:
        {
            currentAnimTrackEntry = spAnimationState_addAnimation(stage->player.animationState, 0, rightCrossAnim.baseAnimation, 0, 0.0f);
            currentActionComboMove++;
        }
        break;

        case comboMove2:
        {
            currentAnimTrackEntry = spAnimationState_addAnimation(stage->player.animationState, 0, leftJabAnim.baseAnimation, 0, 0.0f);
            currentActionComboMove++;
        }
        break;

        case comboMove3:
        {
            currentAnimTrackEntry = spAnimationState_addAnimation(stage->player.animationState, 0, rightUpperCutAnim.baseAnimation, 0, 0.0f);
            currentActionComboMove++;
        }
        break;
        }
    };

    if (KeyHeld(keyboard->MoveRight))
    {
        player->worldPos.x += 2.0f;
    }

    if (KeyHeld(keyboard->MoveLeft))
    {
        player->worldPos.x -= 2.0f;
    }

    ApplyAnimationStateToSkeleton_2(player->skeleton, player->animationState, player->worldPos);
    ApplyAnimationStateToSkeleton_2(ai->skeleton, ai->animationState, ai->worldPos);
    TransformSkeletonToWorldSpace_1(player->skeleton);
    TransformSkeletonToWorldSpace_1(ai->skeleton);

    { //Update hurtBox pos
        player->hurtBox = UpdateCollisionBoxBasedOnCenterPoint(player->hurtBox, player->worldPos);
        ai->hurtBox = UpdateCollisionBoxBasedOnCenterPoint(ai->hurtBox, ai->worldPos);
    };

    { //Create hitbox
        v2f hitBoxCenterWorldCoords = rightCrossAnim.hitBoxCenter + player->worldPos;
        hitBoxCenterWorldCoords.y -= 200.0f;
        player->hitBox.size = { 80.0f, 40.0f };
        player->hitBox = UpdateCollisionBoxBasedOnCenterPoint(player->hitBox, hitBoxCenterWorldCoords);
    };

    if (CheckForFighterCollisions_AxisAligned(player->hurtBox, ai->hurtBox))
    {
        BGZ_CONSOLE("Collision!\n");
    }

    { // Render
        renderCmds.Init();

        renderCmds.ClearScreen();

        { // Draw Level Background
            Coordinate_System backgroundWorldSpace {};
            Coordinate_System backgroundCameraSpace {};
            Drawable_Rect backgroundCanvas {};

            backgroundWorldSpace.Origin = { 0.0f, 0.0f };

            { // Transform to Camera Space
                v2f translationToCameraSpace = stage->camera.viewCenter - stage->camera.lookAt;
                backgroundCameraSpace.Origin = backgroundWorldSpace.Origin + translationToCameraSpace;
            };

            backgroundCanvas = ProduceRectFromBottomLeftPoint(backgroundCameraSpace.Origin, (f32)stage->info.size.width, (f32)stage->info.size.height);

            backgroundCanvas = DilateAboutArbitraryPoint(stage->camera.dilatePoint, stage->camera.zoomFactor, backgroundCanvas);

            renderCmds.DrawBackground(stage->info.currentTexture.ID, backgroundCanvas, v2f { 0.0f, 0.0f }, v2f { 1.0f, 1.0f });
        };

        { //Draw fighters
            Fighter* fighters[2] = { player, ai };
            for (i32 FighterIndex { 0 }; FighterIndex < ArrayCount(fighters); ++FighterIndex)
            {
                for (i32 SlotIndex { 0 }; SlotIndex < fighters[FighterIndex]->skeleton->slotsCount; ++SlotIndex)
                {
                    float verts[8] = { 0 };
                    Texture* texture {};
                    spRegionAttachment* regionAttachment {};
                    spSkeleton* skeleton = fighters[FighterIndex]->skeleton;

                    // If no current active attachment for slot then continue to next slot
                    if (!skeleton->slots[SlotIndex]->attachment)
                        continue;

                    if (skeleton->slots[SlotIndex]->attachment->type == SP_ATTACHMENT_REGION)
                    {
                        regionAttachment = (spRegionAttachment*)skeleton->slots[SlotIndex]->attachment;
                        texture = (Texture*)((spAtlasRegion*)regionAttachment->rendererObject)->page->rendererObject;

                        spRegionAttachment_computeWorldVertices(regionAttachment, skeleton->slots[SlotIndex]->bone, verts, 0, 2);

                        Drawable_Rect spineImage {
                            v2f { verts[0], verts[1] },
                            v2f { verts[2], verts[3] },
                            v2f { verts[4], verts[5] },
                            v2f { verts[6], verts[7] }
                        };

                        v2f UVArray[4] = {
                            v2f { regionAttachment->uvs[0], regionAttachment->uvs[1] },
                            v2f { regionAttachment->uvs[2], regionAttachment->uvs[3] },
                            v2f { regionAttachment->uvs[4], regionAttachment->uvs[5] },
                            v2f { regionAttachment->uvs[6], regionAttachment->uvs[7] }
                        };

                        { // Transform to Camera Space
                            v2f translationToCameraSpace = stage->camera.viewCenter - stage->camera.lookAt;

                            for (ui32 VertIndex { 0 }; VertIndex < ArrayCount(spineImage.Corners);
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
        }; //Draw Fighters

        { //Draw collision boxes
            v2f translationToCameraSpace = stage->camera.viewCenter - stage->camera.lookAt;

            player->hurtBox.bounds.minCorner += translationToCameraSpace;
            player->hurtBox.bounds.maxCorner += translationToCameraSpace;

            player->hitBox.bounds.minCorner += translationToCameraSpace;
            player->hitBox.bounds.maxCorner += translationToCameraSpace;

            ai->hurtBox.bounds.minCorner += translationToCameraSpace;
            ai->hurtBox.bounds.maxCorner += translationToCameraSpace;

            renderCmds.DrawRect(player->hitBox.bounds.minCorner, player->hitBox.bounds.maxCorner, v4f { 0.9f, 0.0f, 0.0f, 0.3f });
            renderCmds.DrawRect(player->hurtBox.bounds.minCorner, player->hurtBox.bounds.maxCorner, v4f { 0.0f, .9f, 0.0f, 0.3f });
            renderCmds.DrawRect(ai->hurtBox.bounds.minCorner, ai->hurtBox.bounds.maxCorner, v4f { 0.0f, .9f, 0.0f, 0.3f });
        };
    };
};
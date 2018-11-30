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
#include "linked_list.h"
#include "ring_buffer.h"
#include "gamecode.h"
#include "math.h"
#include "utilities.h"

global_variable Platform_Services* globalPlatformServices;
global_variable Game_Render_Cmds globalRenderCmds;
global_variable Memory_Handler* globalMemHandler;
global_variable f32 deltaT;
global_variable f32 deltaTFixed;
global_variable f32 viewportWidth;
global_variable f32 viewportHeight;

global_variable spAnimation* rightCrossAnim;
global_variable spAnimation* leftJabAnim;
global_variable spAnimation* rightUpperCutAnim;
global_variable spAnimation* highKickAnim;
global_variable spAnimation* lowKickAnim;
global_variable spAnimation* punchFlurryAnim;

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

void DoComboMoveWith(Fighter* fighter, spAnimation* comboAnim1, spAnimation* comboAnim2, spAnimation* comboAnim3)
{
    ui32 const comboMove1 { 0 };
    ui32 const comboMove2 { 1 };
    ui32 const comboMove3 { 2 };

    if (fighter->currentAnimTrackEntry)
    {
        if (spTrackEntry_getAnimationTime(fighter->currentAnimTrackEntry) == fighter->currentAnimTrackEntry->animationEnd)
        {
            fighter->currentActionComboMove = comboMove1;
        }
    };

    switch (fighter->currentActionComboMove)
    {
    case comboMove1:
    {
        fighter->currentAnimTrackEntry = spAnimationState_addAnimation(fighter->animationState, 0, comboAnim1, 0, 0.0f);
        fighter->trackEntries.PushBack(fighter->currentAnimTrackEntry);
        fighter->currentActionComboMove++;
    }
    break;

    case comboMove2:
    {
        fighter->currentAnimTrackEntry = spAnimationState_addAnimation(fighter->animationState, 0, comboAnim2, 0, 0.0f);
        fighter->trackEntries.PushBack(fighter->currentAnimTrackEntry);
        fighter->currentActionComboMove++;
    }
    break;

    case comboMove3:
    {
        fighter->currentAnimTrackEntry = spAnimationState_addAnimation(fighter->animationState, 0, comboAnim3, 0, 0.0f);
        fighter->trackEntries.PushBack(fighter->currentAnimTrackEntry);
        fighter->currentActionComboMove++;
    }
    break;
    }
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

        deltaTFixed = platformServices->targetFrameTimeInSecs;

        { // Split game memory into more specific memory regions
            gameState->memHandler.memRegions[DYNAMIC] = CreateRegionFromGameMem_1(gameMemory, Megabytes(10));
            InitDynamAllocator_1(&gameState->memHandler.dynamAllocator);
        };

        { // Init spine stuff
            BGZ_ERRCTXT1("When Initializing Spine stuff");

            spAtlas* atlas = spAtlas_createFromFile("data/yellow_god.atlas", 0);
            spSkeletonJson* skelJson = spSkeletonJson_create(atlas);
            stage->commonSkeletonData = spSkeletonJson_readSkeletonDataFile(skelJson, "data/yellow_god.json");
            stage->commonAnimationData = spAnimationStateData_create(stage->commonSkeletonData);
            spSkeletonJson_dispose(skelJson);
        };

        { //Init stage info
            stage->info.displayImage.Data = platformServices->LoadRGBAImage("data/4k.jpg", &stage->info.displayImage.size.width, &stage->info.displayImage.size.height);
            stage->info.displayImage = FlipImage(stage->info.displayImage); // Since opengl will read-in image upside down
            stage->info.currentTexture = renderCmds.LoadTexture(stage->info.displayImage); // TODO: Move out to renderer
            stage->info.size.width = (f32)stage->info.displayImage.size.width;
            stage->info.size.height = (f32)stage->info.displayImage.size.height;
            stage->info.centerPoint = { (f32)stage->info.size.width / 2, (f32)stage->info.size.height / 2 };
        };

        { // Set stage camera
            stage->camera.viewWidth = viewportWidth;
            stage->camera.viewHeight = viewportHeight;
            stage->camera.lookAt = { stage->info.centerPoint.x, stage->info.centerPoint.y - 600.0f };
            stage->camera.viewCenter = { stage->camera.viewWidth / 2.0f, stage->camera.viewHeight / 2.0f };
            stage->camera.dilatePoint = stage->camera.viewCenter - v2f { 0.0f, 200.0f };
            stage->camera.zoomFactor = 1.0f;
        };

        { // Setup fighters
            player->skeleton = spSkeleton_create(stage->commonSkeletonData);
            player->animationState = spAnimationState_create(stage->commonAnimationData);
            player->worldPos = { (stage->info.size.width / 2.0f) - 300.0f, (stage->info.size.height / 2.0f) - 900.0f };
            player->skeleton->x = player->worldPos.x;
            player->skeleton->y = player->worldPos.y;
            player->skeleton->scaleX = .6f;
            player->skeleton->scaleY = .6f;

            ai->skeleton = spSkeleton_create(stage->commonSkeletonData);
            ai->animationState = spAnimationState_create(stage->commonAnimationData);
            ai->worldPos = { (stage->info.size.width / 2.0f) + 300.0f, (stage->info.size.height / 2.0f) - 900.0f };
            ai->skeleton->x = ai->worldPos.x;
            ai->skeleton->y = ai->worldPos.y;
            ai->skeleton->scaleX = -.6f;
            ai->skeleton->scaleY = .6f;
        };

        { //Setup animation stuff
            spAnimationState_setAnimationByName(player->animationState, 0, "idle", 1);
            spAnimationState_setAnimationByName(ai->animationState, 0, "idle", 1);

            rightCrossAnim = spSkeletonData_findAnimation(stage->commonSkeletonData, "right_cross");
            leftJabAnim = spSkeletonData_findAnimation(stage->commonSkeletonData, "left_jab");
            rightUpperCutAnim = spSkeletonData_findAnimation(stage->commonSkeletonData, "right_uppercut");
            highKickAnim = spSkeletonData_findAnimation(stage->commonSkeletonData, "high_kick");
            lowKickAnim = spSkeletonData_findAnimation(stage->commonSkeletonData, "low_kick");
            punchFlurryAnim = spSkeletonData_findAnimation(stage->commonSkeletonData, "punch_flurry");

            { //Setup animation collision boxes which are unique to each animation
                rightCrossAnim->hitBoxCenterOffset = { 300.0f, 400.0f };
                rightCrossAnim->hitBoxSize = { 80.0f, 40.0f };
                rightCrossAnim->hitBoxDuration = .1f;

                leftJabAnim->hitBoxCenterOffset = { 200.0f, 400.0f };
                leftJabAnim->hitBoxSize = { 60.0f, 40.0f };
                leftJabAnim->hitBoxDuration = .1f;

                rightUpperCutAnim->hitBoxCenterOffset = { 100.0f, 400.0f };
                rightUpperCutAnim->hitBoxSize = { 30.0f, 100.0f };
                rightUpperCutAnim->hitBoxDuration = .1f;

                lowKickAnim->hitBoxCenterOffset = { 200.0f, 210.0f };
                lowKickAnim->hitBoxSize = { 30.0f, 130.0f };
                lowKickAnim->hitBoxDuration = .15f;

                punchFlurryAnim->hitBoxCenterOffset = { 200.0f, 400.0f };
                punchFlurryAnim->hitBoxSize = { 80.0f, 80.0f };
                punchFlurryAnim->hitBoxDuration = .15f;

                player->trackEntries.Init(10);
            };
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
            rightCrossAnim = spSkeletonData_findAnimation(stage->commonSkeletonData, "right_cross");
            leftJabAnim = spSkeletonData_findAnimation(stage->commonSkeletonData, "left_jab");
            rightUpperCutAnim = spSkeletonData_findAnimation(stage->commonSkeletonData, "right_uppercut");
            highKickAnim = spSkeletonData_findAnimation(stage->commonSkeletonData, "high_kick");
            lowKickAnim = spSkeletonData_findAnimation(stage->commonSkeletonData, "low_kick");
            punchFlurryAnim = spSkeletonData_findAnimation(stage->commonSkeletonData, "punch_flurry");
        };
    };

    spAnimationState_update(player->animationState, deltaT);
    spAnimationState_update(ai->animationState, deltaT);

    if (KeyComboPressed(keyboard->ActionLeft, keyboard->MoveRight))
    {
        spTrackEntry* entry = spAnimationState_addAnimation(stage->player.animationState, 0, highKickAnim, 0, 0.0f);
        player->trackEntries.PushBack(entry);
    }

    else if (KeyPressed(keyboard->ActionLeft))
    {
        DoComboMoveWith(player, rightCrossAnim, leftJabAnim, rightUpperCutAnim);
    };

    if (KeyPressed(keyboard->ActionRight))
    {
        DoComboMoveWith(player, lowKickAnim, rightCrossAnim, punchFlurryAnim);
    }

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

    Collision_Box hitBox {};

    if (player->trackEntries.Size() > 0)
    {
        local_persist f32 hitBoxDuration {};

        if (spTrackEntry_getAnimationTime(player->trackEntries.GetFirstElem()) > 0.0f)
        {
            spTrackEntry* entry = player->trackEntries.GetFirstElem();

            if (NOT entry->animation->hitBoxTimerStarted)
            {
                entry->animation->hitBoxTimerStarted = true;
                hitBoxDuration = entry->animation->hitBoxDuration;
                entry->animation->hitBoxStartTime = platformServices->realLifeTimeInSecs;
                entry->animation->hitBoxEndTime = entry->animation->hitBoxStartTime + hitBoxDuration;
            }

            if (platformServices->realLifeTimeInSecs <= entry->animation->hitBoxEndTime)
            {
                v2f hitBoxCenterOffset = entry->animation->hitBoxCenterOffset;
                v2f hitBoxSize = entry->animation->hitBoxSize;

                v2f hitBoxCenterWorldPos = hitBoxCenterOffset + player->worldPos;
                InitCollisionBox_1(&hitBox, hitBoxCenterWorldPos, hitBoxSize);
            }
            else
            {
                entry->animation->hitBoxTimerStarted = false;
                player->trackEntries.RemoveElem();
            }
        };
    };

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

            hitBox.bounds.minCorner += translationToCameraSpace;
            hitBox.bounds.maxCorner += translationToCameraSpace;

            renderCmds.DrawRect(hitBox.bounds.minCorner, hitBox.bounds.maxCorner, v4f { 0.9f, 0.0f, 0.0f, 0.6f });
        };
    };
};
/*
    TODO List:
    1.) Figure out if stb_image has a way to modify how images are read (so I can have all reads go through my platform services struct)
    2.) Figure out a way to remove pixelsPerMeter constant that I have strune about in my skeleton and animaiton intialization code
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
#include <stb/stb_image.h>

#define ATOMIC_TYPES_IMPL
#include "atomic_types.h"

global_variable i32 heap;

#include "memory_handling.h"
#include "runtime_array.h"
#include "debug_array.h"
#include "array.h"
#include "dynamic_array.h"
#include "hashmap_str.h"
#include "ring_buffer.h"
#include "linked_list.h"
#include <utility>

#include "shared.h"
#include "renderer_stuff.h"
#include "gamecode.h"
#include "my_math.h"
#include "utilities.h"

global_variable Platform_Services* globalPlatformServices;
global_variable Rendering_Info* global_renderingInfo;
global_variable f32 deltaT;
global_variable f32 deltaTFixed;
global_variable i32 renderBuffer;

//Third Party source
#define STB_IMAGE_IMPLEMENTATION
#define STBI_MALLOC(sz) MallocSize(heap, sz)
#define STBI_REALLOC(p, newsz) ReAllocSize(heap, p, newsz)
#define STBI_FREE(p) DeAlloc(heap, p)
#include <stb/stb_image.h>
#include <boagz/error_context.cpp>

//User source
#define MEMORY_HANDLING_IMPL
#include "memory_handling.h"
#define DYNAMIC_ALLOCATOR_IMPL
#include "dynamic_allocator.h"
#define LINEAR_ALLOCATOR_IMPL
#include "linear_allocator.h"
#define COLLISION_DETECTION_IMPL
#include "collision_detection.h"
#define JSON_IMPL
#include "json.h"
#define ATLAS_IMPL
#include "atlas.h"
#define SKELETON_IMPL
#include "skeleton.h"
#define ANIMATION_IMPL
#include "animation.h"
#define FIGHTER_IMPL
#include "fighter.h"
#define GAME_RENDERER_STUFF_IMPL
#include "renderer_stuff.h"

//Move out to Renderer eventually
#if 0
local_func
Image CreateEmptyImage(i32 width, i32 height)
{
    Image image{};
    
    i32 numBytesPerPixel{4};
    image.data = (ui8*)MallocSize(heap, width*height*numBytesPerPixel);
    image.size = v2i{width, height};
    image.pitch = width*numBytesPerPixel;
    
    return image;
};

local_func
void GenerateSphereNormalMap(Image&& sourceImage)
{
    f32 invWidth = 1.0f / (f32)(sourceImage.size.width - 1);
    f32 invHeight = 1.0f / (f32)(sourceImage.size.height - 1);
    
    ui8* row = (ui8*)sourceImage.data;
    for(i32 y = 0; y < sourceImage.size.height; ++y)
    {
        ui32* pixel = (ui32*)row;
        
        for(i32 x = 0; x < sourceImage.size.width; ++x)
        {
            v2 normalUV = {invWidth*(f32)x, invHeight*(f32)y};
            
            f32 normalX = 2.0f*normalUV.x - 1.0f;
            f32 normalY = 2.0f*normalUV.y - 1.0f;
            
            f32 rootTerm = 1.0f - normalX*normalX - normalY*normalY;
            f32 normalZ = 0.0f;
            
            v3 normal {0.0f, 0.0f, 1.0f};
            
            if(rootTerm >= 0.0f)
            {
                normalZ = Sqrt(rootTerm);
                normal = v3{normalX, normalY, normalZ};
            };
            
            //Convert from -1 to 1 range to value between 0 and 255
            v4 color = {255.0f*(.5f*(normal.x + 1.0f)),
                255.0f*(.5f*(normal.y + 1.0f)),
                255.0f*(.5f*(normal.z + 1.0f)),
                0.0f};
            
            *pixel++ = (((ui8)(color.a + .5f) << 24) |
                        ((ui8)(color.r + .5f) << 16) |
                        ((ui8)(color.g + .5f) << 8) |
                        ((ui8)(color.b + .5f) << 0));
        };
        
        row += sourceImage.pitch;
    };
};

local_func
Image FlipImage(Image image)
{
    i32 widthInBytes = image.size.width * 4;
    ui8* p_topRowOfTexels = nullptr;
    ui8* p_bottomRowOfTexels = nullptr;
    ui8 temp = 0;
    i32 halfHeight = image.size.height / 2;
    
    for (i32 row = 0; row < halfHeight; ++row)
    {
        p_topRowOfTexels = image.data + row * widthInBytes;
        p_bottomRowOfTexels = image.data + (image.size.height - row - 1) * widthInBytes;
        
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
#endif

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

f32 WidthInMeters(Image bitmap, f32 heightInMeters)
{
    f32 width_meters = bitmap.aspectRatio * heightInMeters;
    
    return width_meters;
};

Quadf ParentTransform(Quadf localCoords, Transform transformInfo_world)
{
    Coordinate_Space parentSpace {};
    parentSpace.origin = transformInfo_world.translation;
    parentSpace.xBasis = v2 { CosR(transformInfo_world.rotation), SinR(transformInfo_world.rotation) };
    parentSpace.yBasis = transformInfo_world.scale.y * PerpendicularOp(parentSpace.xBasis);
    parentSpace.xBasis *= transformInfo_world.scale.x;
    
    Quadf transformedCoords {};
    for (i32 vertIndex {}; vertIndex < transformedCoords.vertices.Size(); ++vertIndex)
    {
        //This equation rotates first then moves to correct parent position
        transformedCoords.vertices[vertIndex] = parentSpace.origin + (localCoords.vertices[vertIndex].x * parentSpace.xBasis) + (localCoords.vertices[vertIndex].y * parentSpace.yBasis);
    };
    
    return transformedCoords;
};

extern "C" void GameUpdate(Application_Memory* gameMemory, Platform_Services* platformServices, Rendering_Info* renderingInfo, Game_Sound_Output_Buffer* soundOutput, Game_Input* gameInput)
{
    auto TranslateCurrentMeasurementsToGameUnits = [](Skeleton&& skel, AnimationData&& animData) {
        f32 pixelsPerMeter { global_renderingInfo->_pixelsPerMeter };
        
        skel.width /= pixelsPerMeter;
        skel.height /= pixelsPerMeter;
        
        for (i32 boneIndex {}; boneIndex < skel.bones.length; ++boneIndex)
        {
            skel.bones[boneIndex].parentBoneSpace.translation.x /= pixelsPerMeter;
            skel.bones[boneIndex].parentBoneSpace.translation.y /= pixelsPerMeter;
            skel.bones[boneIndex].initialPos_parentBoneSpace.x /= pixelsPerMeter;
            skel.bones[boneIndex].initialPos_parentBoneSpace.y /= pixelsPerMeter;
            
            skel.bones[boneIndex].parentBoneSpace.rotation = ToRadians(skel.bones[boneIndex].parentBoneSpace.rotation);
            skel.bones[boneIndex].initialRotation_parentBoneSpace = ToRadians(skel.bones[boneIndex].initialRotation_parentBoneSpace);
            
            skel.bones[boneIndex].length /= pixelsPerMeter;
        };
        
        for (i32 slotI {}; slotI < skel.slots.length; ++slotI)
        {
            skel.slots[slotI].regionAttachment.height /= pixelsPerMeter;
            skel.slots[slotI].regionAttachment.width /= pixelsPerMeter;
            skel.slots[slotI].regionAttachment.parentBoneSpace.rotation = ToRadians(skel.slots[slotI].regionAttachment.parentBoneSpace.rotation);
            skel.slots[slotI].regionAttachment.parentBoneSpace.translation.x /= pixelsPerMeter;
            skel.slots[slotI].regionAttachment.parentBoneSpace.translation.y /= pixelsPerMeter;
        };
        
        for (i32 animIndex {}; animIndex < animData.animMap.animations.length; ++animIndex)
        {
            Animation* anim = &animData.animMap.animations[animIndex];
            
            if (anim->name)
            {
                for (i32 boneIndex {}; boneIndex < anim->bones.Size(); ++boneIndex)
                {
                    TranslationTimeline* boneTranslationTimeline = &anim->boneTranslationTimelines[boneIndex];
                    for (i32 keyFrameIndex {}; keyFrameIndex < boneTranslationTimeline->translations.Size(); ++keyFrameIndex)
                    {
                        boneTranslationTimeline->translations[keyFrameIndex].x /= pixelsPerMeter;
                        boneTranslationTimeline->translations[keyFrameIndex].y /= pixelsPerMeter;
                    }
                    
                    RotationTimeline* boneRotationTimeline = &anim->boneRotationTimelines[boneIndex];
                    for (i32 keyFrameIndex {}; keyFrameIndex < boneRotationTimeline->angles.Size(); ++keyFrameIndex)
                    {
                        boneRotationTimeline->angles[keyFrameIndex] = ToRadians(boneRotationTimeline->angles[keyFrameIndex]);
                    }
                };
                
                for (i32 hitBoxIndex {}; hitBoxIndex < anim->hitBoxes.length; ++hitBoxIndex)
                {
                    anim->hitBoxes[hitBoxIndex].size.width /= pixelsPerMeter;
                    anim->hitBoxes[hitBoxIndex].size.height /= pixelsPerMeter;
                    anim->hitBoxes[hitBoxIndex].worldPosOffset.x /= pixelsPerMeter;
                    anim->hitBoxes[hitBoxIndex].worldPosOffset.y /= pixelsPerMeter;
                };
            };
        }
    };
    
    BGZ_ERRCTXT1("When entering GameUpdate");
    
    const Game_Controller* keyboard = &gameInput->Controllers[0];
    const Game_Controller* gamePad = &gameInput->Controllers[1];
    
    Game_State* gState = (Game_State*)gameMemory->permanentStorage;
    
    //Setup globals
    deltaT = platformServices->prevFrameTimeInSecs;
    deltaTFixed = platformServices->targetFrameTimeInSecs;
    globalPlatformServices = platformServices;
    global_renderingInfo = renderingInfo;
    
    Stage_Data* stage = &gState->stage;
    Fighter* player = &stage->player;
    Fighter* enemy = &stage->enemy;
    Fighter* enemy2 = &stage->enemy2;
    
    Memory_Partition* framePart = GetMemoryPartition(gameMemory, "frame");
    Memory_Partition* levelPart = GetMemoryPartition(gameMemory, "level");
    
    if (NOT gameMemory->initialized)
    {
        BGZ_ERRCTXT1("When Initializing game memory and game state");
        
        gameMemory->initialized = true;
        
        *gState = {}; //Make sure everything gets properly defaulted/Initialized (constructors are called that need to be)
        
        //Read in data
        Skeleton playerSkel {}, enemySkel {};
        AnimationData playerAnimData {}, enemyAnimData {};
        InitSkel($(playerSkel), $(*levelPart), "data/yellow_god.atlas", "data/yellow_god.json"); //TODO: In order to reduce the amount of time reading from json file think about how to implement one common skeleton/animdata file(s)
        InitAnimData($(playerAnimData), $(*levelPart), "data/yellow_god.json", playerSkel);
        //InitSkel($(enemySkel), $(*levelPart), "data/yellow_god.atlas", "data/yellow_god.json");
        //InitAnimData($(enemyAnimData), $(*levelPart), "data/yellow_god.json", enemySkel);
        
        //Stage Init
        stage->backgroundImg = LoadBitmap_BGRA("data/4k.jpg");
        stage->size.height = 40.0f;
        stage->size.width = WidthInMeters(stage->backgroundImg, stage->size.height);
        stage->centerPoint = { (f32)WidthInMeters(stage->backgroundImg, stage->size.height) / 2, (f32)stage->size.height / 2 };
        
        //Camera Init
        stage->camera.dilatePointOffset_normalized = { 0.0f, -0.3f };
        stage->camera.lookAt = { stage->size.width / 2.0f, 10.0f };
        stage->camera.zoomFactor = .4f;
        
        //Translate pixels to meters and degrees to radians (since spine exports everything in pixel/degree units)
        TranslateCurrentMeasurementsToGameUnits($(playerSkel), $(playerAnimData));
        //TranslateCurrentMeasurementsToGameUnits($(enemySkel), $(enemyAnimData));
        
        //Init fighters
        v2 playerWorldPos = { (stage->size.width / 2.0f) - 6.0f, 3.0f }, enemyWorldPos = { (stage->size.width / 2.0f) + 6.0f, 3.0f };
        HurtBox playerDefaultHurtBox { playerWorldPos, v2 { 2.0f, 8.9f }, v2 { 2.3f, 2.3f } };
        //HurtBox enemyDefaultHurtBox { enemyWorldPos, v2 { -2.0f, 8.9f }, v2 { 2.3f, 2.3f } };
        InitFighter($(*player), playerAnimData, playerSkel, /*player height*/ 20.0f, playerDefaultHurtBox, playerWorldPos, /*flipX*/ false);
        //InitFighter($(*enemy), enemyAnimData, enemySkel, /*player height*/ enemySkel.height, enemyDefaultHurtBox, enemyWorldPos, /*flipX*/ false );
        
        SetIdleAnimation($(player->animQueue), player->animData, "idle");
        //SetIdleAnimation($(enemy->animQueue), enemy->animData, "idle");
        
        gState->openGLRenderTest = LoadBitmap_BGRA("data/left-bicep.png");
        
        v2 vec2 {.4f, .2f};
        v3 vec3 = { vec2, 1.0f};
        //v4 vec4 = {v3{0.0f, 0.0f, 0.0f}, 1.0f};
    };
    
    Animation playerCurrentAnim = UpdateAnimationState($(player->animQueue), deltaT);
    //Animation enemyCurrentAnim = UpdateAnimationState($(enemy->animQueue), deltaT);
    
    ApplyAnimationToSkeleton($(player->skel), playerCurrentAnim);
    //ApplyAnimationToSkeleton($(enemy->skel), enemyCurrentAnim);
    
    UpdateSkeletonBoneWorldTransforms($(player->skel), player->world.translation);
    //UpdateSkeletonBoneWorldTransforms($(enemy->skel), enemy->world.translation);
    
    UpdateCollisionBoxWorldPos_BasedOnCenterPoint($(player->hurtBox), player->world.translation);
    //UpdateCollisionBoxWorldPos_BasedOnCenterPoint($(enemy->hurtBox), enemy->world.translation);
    
#if 0
    for (i32 hitBoxIndex {}; hitBoxIndex < playerCurrentAnim.hitBoxes.length; ++hitBoxIndex)
    {
        UpdateHitBoxStatus($(playerCurrentAnim.hitBoxes[hitBoxIndex]), playerCurrentAnim.currentTime);
        
        if (playerCurrentAnim.hitBoxes[hitBoxIndex].isActive)
        {
            playerCurrentAnim.hitBoxes[hitBoxIndex].pos_worldSpace = { 0.0f, 0.0f };
            
            Bone* bone = GetBoneFromSkeleton(&player->skel, playerCurrentAnim.hitBoxes[hitBoxIndex].boneName);
            UpdateCollisionBoxWorldPos_BasedOnCenterPoint($(playerCurrentAnim.hitBoxes[hitBoxIndex]), bone->worldSpace.translation);
            b collisionOccurred = CheckForFighterCollisions_AxisAligned(playerCurrentAnim.hitBoxes[hitBoxIndex], enemy->hurtBox);
            
            if (collisionOccurred)
                BGZ_CONSOLE("ahhahha");
        };
    };
#endif
    
    UpdateCamera(global_renderingInfo, stage->camera.lookAt, stage->camera.zoomFactor, stage->camera.dilatePointOffset_normalized);
    
    { //Render
#if 0
        auto DrawFighter = [](Fighter fighter) -> void {
            for (i32 slotIndex { 17 }; slotIndex < fighter.skel.slots.length - 1; ++slotIndex)
            {
                Slot* currentSlot = &fighter.skel.slots[slotIndex];
                
                AtlasRegion* region = &currentSlot->regionAttachment.region_image;
                Array<v2, 2> uvs2 = { v2 { region->u, region->v }, v2 { region->u2, region->v2 } };
                
                Quadf region_localCoords = ProduceQuadFromCenterPoint({ 0.0f, 0.0f }, currentSlot->regionAttachment.width, currentSlot->regionAttachment.height);
                Quadf region_boneSpaceCoords = ParentTransform(region_localCoords, currentSlot->regionAttachment.parentBoneSpace);
                Quadf region_worldSpaceCoords = ParentTransform(region_localCoords, currentSlot->bone->worldSpace);
                
                PushTexture(global_renderingInfo, region_worldSpaceCoords, $(region->page->rendererObject), v2 { currentSlot->regionAttachment.width, currentSlot->regionAttachment.height }, uvs2, region->name);
            };
        };
        
        //Push background
#if 1
        Array<v2, 2> uvs = { v2 { 0.0f, 0.0f }, v2 { 1.0f, 1.0f } };
        Quadf targetRect_worldCoords = ProduceQuadFromCenterPoint(stage->centerPoint, stage->size.width, stage->size.height);
        //PushTexture(global_renderingInfo, targetRect_worldCoords, $(stage->backgroundImg), stage->size.height, uvs, "background");
        PushRect(global_renderingInfo, targetRect_worldCoords, { 1.0f, 0.0f, 0.0f });
#endif
        
        //Push Fighters
        DrawFighter(*player);
        //DrawFighter(*enemy);
        
        v2 line_minPoint = {(stage->size.width / 2.0f) - 9.0f, 3.0f };
        v2 line_maxPoint = {(stage->size.width / 2.0f) - 3.0f, 10.0f };
        
        PushLine(global_renderingInfo, line_minPoint, line_maxPoint, {0.0f, 1.0f, 0.0f}/*color*/, 5.0f/*thickness*/);
        
#if 0
        for (i32 i {}; i < player->skel.bones.length; ++i)
        {
            Bone bone = player->skel.bones[i];
            Quadf boneRect = ProduceQuadFromCenterPoint(bone.worldSpace.translation, .1f, .1f);
            PushRect(global_renderingInfo, boneRect, { 1.0f, 0.0f, 0.0f });
        }
#endif
        
#if 0
        { //Draw collision boxes
            Quadf playerTargetRect_localCoords = ProduceQuadFromCenterPoint(v2 { 0.0f, 0.0f }, player->hurtBox.size.width, player->hurtBox.size.height);
            //Quadf enemyTargetRect_localCoords = ProduceQuadFromCenterPoint(v2 { 0.0f, 0.0f }, enemy->hurtBox.size.width, enemy->hurtBox.size.height);
            
            Quadf playerTargetRect_worldCoords = ParentTransform(playerTargetRect_localCoords, Transform { player->hurtBox.pos_worldSpace, 0.0f, { 1.0f, 1.0f } });
            //Quadf enemyTargetRect_worldCoords = ParentTransform(playerTargetRect_localCoords, Transform { enemy->hurtBox.pos_worldSpace, 0.0f, { 1.0f, 1.0f } });
            
            PushRect(global_renderingInfo, playerTargetRect_worldCoords, { 1.0f, 0.0f, 0.0f });
            //PushRect(global_renderingInfo, enemyTargetRect_worldCoords, { 1.0f, 0.0f, 0.0f });
            
            for (i32 hitBoxIndex {}; hitBoxIndex < playerCurrentAnim.hitBoxes.length; ++hitBoxIndex)
            {
                Quadf playerHitBox_worldCoords = ProduceQuadFromCenterPoint(playerCurrentAnim.hitBoxes[hitBoxIndex].pos_worldSpace, playerCurrentAnim.hitBoxes[hitBoxIndex].size.width, playerCurrentAnim.hitBoxes[hitBoxIndex].size.height);
                
                if (playerCurrentAnim.hitBoxes[hitBoxIndex].isActive)
                    PushRect(global_renderingInfo, playerHitBox_worldCoords, { 0.0f, 1.0f, 0.0f });
            };
        }
#endif
#endif
        
        Quadf targetRect_localCoords = ProduceQuadFromCenterPoint({0.0f, 0.0f}, 2.0f, 3.0f);
        Transform rectTransform { {stage->centerPoint}, 2.0f, {1.0f, 1.0f} };
        Quadf targetRect_worldCoords = ParentTransform(targetRect_localCoords, rectTransform);
        PushRect(global_renderingInfo, targetRect_worldCoords, { 1.0f, 0.0f, 0.0f });
        
        Quadf textureTargetRect_localCoords = ProduceQuadFromCenterPoint({0.0f, 0.0f}, gState->openGLRenderTest.aspectRatio * BitmapHeight_Meters(*global_renderingInfo, gState->openGLRenderTest), BitmapHeight_Meters(*global_renderingInfo, gState->openGLRenderTest));
        Quadf textureTargetRect_worldCoords = ParentTransform(textureTargetRect_localCoords, rectTransform);
        Array<v2, 2> uvs = { v2 { 0.0f, 0.0f }, v2 { 1.0f, 1.0f } };
        PushTexture(global_renderingInfo, textureTargetRect_worldCoords, $(gState->openGLRenderTest), BitmapHeight_Meters(*global_renderingInfo, gState->openGLRenderTest), uvs, "left-bicep-image");
    };
    
    IsAllTempMemoryCleared(framePart);
    IsAllTempMemoryCleared(levelPart);
    Release($(*framePart));
    
    if (gState->isLevelOver)
        Release($(*levelPart));
};

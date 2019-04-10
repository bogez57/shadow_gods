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

#define ATOMIC_TYPES_IMPL
#include "atomic_types.h"
#define MEMHANDLING_IMPL
#include "memory_handling.h"
#include "array.h"
#include "dynamic_array.h"
#include "ring_buffer.h"
#include "linked_list.h"
#include <utility>

#include "atlas.h"
#include "shared.h"
#include "dynamic_allocator.h"
#include "gamecode.h"
#include "math.h"
#include "utilities.h"

global_variable Platform_Services* globalPlatformServices;
global_variable Game_Render_Cmds globalRenderCmds;
global_variable f32 deltaT;
global_variable f32 deltaTFixed;
global_variable f32 viewportWidth;
global_variable f32 viewportHeight;
global_variable Dynamic_Allocator dynamAllocator { 0 };

const i32 bytesPerPixel{4};

#define COLLISION_IMPL
#include "collisions.h"
#define ATLAS_IMPL
#include "atlas.h"
#define JSON_IMPL
#include "json.h"
#define SKELETON_IMPL
#include "skeleton.h"

// Third Party
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

local_func void
DrawRectangle(Image&& buffer, Rectf rect, f32 r, f32 g, f32 b)
{    
    //Since I'm dealing with int pixels below
    Recti rectToDraw {};
    rectToDraw.min = RoundFloat32ToInt32(rect.min);
    rectToDraw.max = RoundFloat32ToInt32(rect.max);

    {//Make sure we don't try to draw outside current screen space
        if(rectToDraw.min.x < 0)
            rectToDraw.min.x = 0;

        if(rectToDraw.min.y < 0)
            rectToDraw.min.y = 0;

        if(rectToDraw.max.x > buffer.size.width)
            rectToDraw.max.x = buffer.size.width;

        if(rectToDraw.max.y > buffer.size.height)
            rectToDraw.max.y = buffer.size.height;
    };

    ui32 Color = ((RoundFloat32ToUInt32(r * 255.0f) << 16) |
                  (RoundFloat32ToUInt32(g * 255.0f) << 8) |
                  (RoundFloat32ToUInt32(b * 255.0f) << 0));

    ui8* currentRow = ((ui8*)buffer.data + rectToDraw.min.x*bytesPerPixel + rectToDraw.min.y*buffer.pitch);
    for(i32 column = rectToDraw.min.y; column < rectToDraw.max.y; ++column)
    {
        ui32* destPixel = (ui32*)currentRow;
        for(i32 row = rectToDraw.min.x; row < rectToDraw.max.x; ++row)
        {            
            *destPixel++ = Color;
        }
        
        currentRow += buffer.pitch;
    }
}

//For static images
local_func void
DrawImage(Image&& buffer, Rectf rect, Image image)
{
    ui32* imagePixel = (ui32*)image.data;

    //Since I'm dealing with int pixels below
    Recti targetRect{};
    targetRect.min = RoundFloat32ToInt32(rect.min);
    targetRect.max = RoundFloat32ToInt32(rect.max);

    {//Make sure we don't try to draw outside current screen space
        if(targetRect.min.x < 0)
            targetRect.min.x = 0;

        if(targetRect.min.y < 0)
            targetRect.min.y = 0;

        if(targetRect.max.x > buffer.size.width)
            targetRect.max.x = buffer.size.width;

        if(targetRect.max.y > buffer.size.height)
            targetRect.max.y = buffer.size.height;
    };

    ui8* currentRow = ((ui8*)buffer.data + targetRect.min.x*bytesPerPixel + targetRect.min.y*buffer.pitch);
    for(i32 column = targetRect.min.y; column < targetRect.max.y; ++column)
    {
        ui32* destPixel = (ui32*)currentRow;

        for(i32 row = targetRect.min.x; row < targetRect.max.x; ++row)
        {            

#if 0 //Non-premultiplied alpha - aka post multiplied alpha (assuming premultiplication hasn't been done already)
            auto[blendedPixel_R,blendedPixel_G,blendedPixel_B] = LinearBlend(*imagePixel, *destPixel, BGRA);
            

            *destPixel = ((0xFF << 24) |
                           (blendedPixel_R << 16) |
                           (blendedPixel_G << 8) |
                           (blendedPixel_B << 0));
            ++destPixel;
            ++imagePixel;
#else
//Pre-multiplied alpha
            v4f backgroundColors = UnPackPixelValues(*destPixel, BGRA);
            v4f foregroundColors = UnPackPixelValues(*imagePixel, BGRA);
            f32 alphaBlend = foregroundColors.a / 255.0f;
            v4f finalBlendedColor = (1.0f - alphaBlend)*backgroundColors + foregroundColors;

            *destPixel = ((0xFF << 24) |
                           ((ui8)finalBlendedColor.r << 16) |
                           ((ui8)finalBlendedColor.g << 8) |
                           ((ui8)finalBlendedColor.b << 0));

            ++destPixel;
            ++imagePixel;
#endif
        }
        
        currentRow += buffer.pitch;
    }
}

//For images that move/rotate/scale - Assumes pre-multiplied alpha
local_func void
DrawImageSlowly(Image&& buffer, Rectf worldCoords, Image image, Image normalMap = {})
{    
    v2f origin = worldCoords.min;
    v2f targetRectXAxis = v2f{worldCoords.max.x, worldCoords.min.y} - origin;
    v2f targetRectYAxis = v2f{worldCoords.min.x, worldCoords.max.y} - origin;

    f32 widthMax = (f32)(buffer.size.width - 1);
    f32 heightMax = (f32)(buffer.size.height - 1);
    
    f32 xMin = widthMax;
    f32 xMax = 0.0f;
    f32 yMin = heightMax;
    f32 yMax = 0.0f;

    {//Optimization to avoid iterating over every pixel on the screen - HH ep 92
        Array<v2f, 4> vecs = {origin, origin + targetRectXAxis, origin + targetRectXAxis + targetRectYAxis, origin + targetRectYAxis};
        for(i32 vecIndex = 0; vecIndex < vecs.Size(); ++vecIndex)
        {
            v2f testVec = vecs.At(vecIndex);
            i32 flooredX = FloorF32ToI32(testVec.x);
            i32 ceiledX = CeilF32ToI32(testVec.x);
            i32 flooredY= FloorF32ToI32(testVec.y);
            i32 ceiledY = CeilF32ToI32(testVec.y);

            if(xMin > flooredX) {xMin = (f32)flooredX;}
            if(yMin > flooredY) {yMin = (f32)flooredY;}
            if(xMax < ceiledX) {xMax = (f32)ceiledX;}
            if(yMax < ceiledY) {yMax = (f32)ceiledY;}
        }

        if(xMin < 0.0f) {xMin = 0.0f;}
        if(yMin < 0.0f) {yMin = 0.0f;}
        if(xMax > widthMax) {xMax = widthMax;}
        if(yMax > heightMax) {yMax = heightMax;}
    };

    f32 invertedXAxisSqd = 1.0f / MagnitudeSqd(targetRectXAxis);
    f32 invertedYAxisSqd = 1.0f / MagnitudeSqd(targetRectYAxis);
    ui8* currentRow = (ui8*)buffer.data + (i32)xMin * bytesPerPixel + (i32)yMin * buffer.pitch; 

    for(f32 screenY = yMin; screenY < yMax; ++screenY)
    {
        ui32* destPixel = (ui32*)currentRow;
        for(f32 screenX = xMin; screenX < xMax; ++screenX)
        {            
            //In order to fill only pixels defined by our rectangle points/vecs, we are
            //testing to see if a pixel iterated over falls into that rectangle. This is
            //done by checking if the dot product of the screen pixel vector, subtracted from
            //origin, and the certain 'edge' vector of the rect comes out positive. If 
            //positive then pixel is within, if negative then it is outside. 
            v2f screenPixelCoord{screenX, screenY};
            v2f d {screenPixelCoord - origin};
            f32 edge1 = DotProduct(d, targetRectYAxis);
            f32 edge2 = DotProduct(d + -targetRectXAxis, -targetRectXAxis);
            f32 edge3 = DotProduct(d + -targetRectXAxis + -targetRectYAxis, -targetRectYAxis);
            f32 edge4 = DotProduct(d + -targetRectYAxis, targetRectXAxis);

            if(edge1 > 0 && edge2 > 0 && edge3 > 0 && edge4 > 0)
            {
                //Infer texel position from normalized target rect pos (gather uv's)
                f32 u = invertedXAxisSqd * DotProduct(d, targetRectXAxis);
                f32 v = invertedYAxisSqd * DotProduct(d, targetRectYAxis);

                f32 texelPosX = 1.0f + (u*(f32)(image.size.width - 3));
                f32 texelPosY = 1.0f + (v*(f32)(image.size.height - 3)); 

                f32 epsilon = 0.00001f;//TODO: Remove????
                BGZ_ASSERT(((u + epsilon) >= 0.0f) && ((u - epsilon) <= 1.0f), "u is out of range! %f", u);
                BGZ_ASSERT(((v + epsilon) >= 0.0f) && ((v - epsilon) <= 1.0f), "v is out of range! %f", v);
                BGZ_ASSERT((texelPosX >= 0) && (texelPosX <= (i32)image.size.width), "x coord is out of range!: ");
                BGZ_ASSERT((texelPosY >= 0) && (texelPosY <= (i32)image.size.height), "x coord is out of range!");

                ui8* texelPtr = ((ui8*)image.data) + ((ui32)texelPosY*image.pitch) + ((ui32)texelPosX*sizeof(ui32));//size of pixel

                //Grab 4 texels (in a square pattern) to blend
                ui32 texelPtrA = *(ui32*)(texelPtr);
                ui32 texelPtrB = *(ui32*)(texelPtr + sizeof(ui32));
                ui32 texelPtrC = *(ui32*)(texelPtr + image.pitch);
                ui32 texelPtrD = *(ui32*)(texelPtr + image.pitch + sizeof(ui32));

                //Blend between all 4 pixels to produce new color for sub pixel accruacy - Bilinear filtering
                v4f texelA = UnPackPixelValues(texelPtrA, BGRA);
                v4f texelB = UnPackPixelValues(texelPtrB, BGRA);
                v4f texelC = UnPackPixelValues(texelPtrC, BGRA);
                v4f texelD = UnPackPixelValues(texelPtrD, BGRA);
                f32 percentToLerpInX = texelPosX - Floor(texelPosX);
                f32 percentToLerpInY = texelPosY - Floor(texelPosY);
                v4f ABLerpColor = Lerp(texelA, texelB, percentToLerpInX);
                v4f CDLerpColor = Lerp(texelC, texelD, percentToLerpInX);
                v4f newBlendedTexel = Lerp(ABLerpColor, CDLerpColor, percentToLerpInY);

                //Linearly Blend with background - Pre-multiplied alpha
                v4f backgroundColors = UnPackPixelValues(*destPixel, BGRA);
                f32 alphaBlend = newBlendedTexel.a / 255.0f;
                v4f finalBlendedColor = (1.0f - alphaBlend)*backgroundColors + newBlendedTexel;

                *destPixel = ((0xFF << 24) |
                           ((ui8)finalBlendedColor.r << 16) |
                           ((ui8)finalBlendedColor.g << 8) |
                           ((ui8)finalBlendedColor.b << 0));

                if(normalMap.data)
                {
                    ui8* normalPtr = ((ui8*)normalMap.data) + ((ui32)texelPosY*image.pitch) + ((ui32)texelPosX*sizeof(ui32));//size of pixel

                    //Grab 4 texels (in a square pattern) to blend
                    ui32 normalPtrA = *(ui32*)(normalPtr);
                    ui32 normalPtrB = *(ui32*)(normalPtr + sizeof(ui32));
                    ui32 normalPtrC = *(ui32*)(normalPtr + image.pitch);
                    ui32 normalPtrD = *(ui32*)(normalPtr + image.pitch + sizeof(ui32));

                    //Blend between all 4 pixels to produce new color for sub pixel accruacy - Bilinear filtering
                    v4f normalA = UnPackPixelValues(normalPtrA, BGRA);
                    v4f normalB = UnPackPixelValues(normalPtrB, BGRA);
                    v4f normalC = UnPackPixelValues(normalPtrC, BGRA);
                    v4f normalD = UnPackPixelValues(normalPtrD, BGRA);
                    f32 percentToLerpInX = texelPosX - Floor(texelPosX);
                    f32 percentToLerpInY = texelPosY - Floor(texelPosY);
                    v4f ABLerp = Lerp(normalA, normalB, percentToLerpInX);
                    v4f CDLerp = Lerp(normalC, normalD, percentToLerpInX);
                    v4f blendedNormal = Lerp(ABLerp, CDLerp, percentToLerpInY);
                }

            }

            ++destPixel;
        }
        
        currentRow += buffer.pitch;
    }
}

void RenderToImage(Image&& renderTarget, Image sourceImage, Rectf targetArea)
{
    DrawImageSlowly($(renderTarget), targetArea, sourceImage);
};

extern "C" void GameUpdate(Application_Memory* gameMemory, Game_Offscreen_Buffer* gameBackBuffer, Platform_Services* platformServices, Game_Render_Cmds renderCmds, Game_Sound_Output_Buffer* soundOutput, Game_Input* gameInput)
{
    BGZ_ERRCTXT1("When entering GameUpdate");

    const Game_Controller* keyboard = &gameInput->Controllers[0];
    const Game_Controller* gamePad = &gameInput->Controllers[1];

    Game_State* gState = (Game_State*)gameMemory->PermanentStorage;

    //Setup globals
    deltaT = platformServices->prevFrameTimeInSecs;
    deltaTFixed = platformServices->targetFrameTimeInSecs;
    globalPlatformServices = platformServices;
    globalRenderCmds = renderCmds;

    Stage_Data* stage = &gState->stage;
    Fighter* player = &stage->player;
    Fighter* enemy = &stage->enemy;
    Fighter* enemy2 = &stage->enemy2;

    gState->colorBuffer.data = (ui8*)gameBackBuffer->memory;
    gState->colorBuffer.size = v2i{gameBackBuffer->width, gameBackBuffer->height};
    gState->colorBuffer.pitch = gameBackBuffer->pitch;

    if (NOT gameMemory->Initialized)
    {
        BGZ_ERRCTXT1("When Initializing game memory and game state");

        gameMemory->Initialized = true;
        *gState = {}; //Make sure everything gets properly defaulted (constructors are called that need to be)

        viewportWidth = 1280.0f;
        viewportHeight = 720.0f;

        InitApplicationMemory(gameMemory);
        CreateRegionFromMemory(gameMemory, Megabytes(500));

        //Stage Init
        stage->info.size.x = viewportWidth;
        stage->info.size.y = viewportHeight;
        stage->info.backgroundImg.data = platformServices->LoadBGRAbitImage("data/mountain.jpg", $(stage->info.backgroundImg.size.width), $(stage->info.backgroundImg.size.height));

        //Player Init
        player->image.data = platformServices->LoadBGRAbitImage("data/test_head_front.bmp", $(player->image.size.width), $(player->image.size.height));
        player->image.pitch = player->image.size.width * bytesPerPixel;
        player->world.pos = {200.0f, -50.0f};
        player->world.rotation = 0.0f;
        player->world.scale = 2.0f;
        player->image.opacity = .5f;
        
        //Enemy Init
        enemy->image.data = platformServices->LoadBGRAbitImage("data/test_body_back.bmp", $(enemy->image.size.width), $(enemy->image.size.height));
        enemy->image.pitch = enemy->image.size.width * bytesPerPixel;
        enemy->world.pos = {200.0f, 150.0f};
        enemy->world.rotation = 0.0f;
        enemy->world.scale = 2.3f;
        enemy->image.opacity = .7f;

         //Enemy Init
        enemy2->image.data = platformServices->LoadBGRAbitImage("data/test_cape_front.bmp", $(enemy2->image.size.width), $(enemy2->image.size.height));
        enemy2->image.pitch = enemy2->image.size.width * bytesPerPixel;
        enemy2->world.pos = {200.0f, 100.0f};
        enemy2->world.rotation = 0.0f;
        enemy2->world.scale = 2.3f;
        enemy2->image.opacity = .7f;

        //Create empty image
        auto CreateEmptyImage = [](i32 width, i32 height) -> Image
                            {
                                Image image{};

                                image.data = (ui8*)CallocSize(0, width*height*bytesPerPixel);
                                image.size = v2i{width, height};
                                image.pitch = width*bytesPerPixel;

                                return image;
                            };

        auto GenerateSphereNormalMap = [](Image sourceImage) -> void
                            {
                                f32 invWidth = 1.0f / (1.0f - sourceImage.size.width);
                                f32 invHeight = 1.0f / (1.0f - sourceImage.size.height);
                                
                                ui8* row = (ui8*)sourceImage.data;
                                for(i32 y = 0; y < sourceImage.size.height; ++y)
                                {
                                    ui32* pixel = (ui32*)row;

                                    for(i32 x = 0; x < sourceImage.size.width; ++x)
                                    {
                                        v2f normalUV = {invWidth*(f32)x, invHeight*(f32)y};
                                        
                                        //Retreive normal and normalize
                                        v3f normal = {2.0f*normalUV.x - 1.0f, 2.0f*normalUV.y - 1.0f, 0.0f};
                                        normal = Normalize(normal);

                                        //Convert to value between 0 and 255
                                        v4f color = {255.0f*(.5f*(normal.x + 1.0f)),
                                                     255.0f*(.5f*(normal.y + 1.0f)),
                                                     0.0f, 
                                                     0.0f};

                                        *pixel = (((ui8)color.a << 24) |
                                                     ((ui8)color.r << 16) |
                                                     ((ui8)color.g << 8) |
                                                     ((ui8)color.b << 0));
                                    }
                                }

                                row += sourceImage.pitch;
                            };
        
        gState->composite = CreateEmptyImage((i32)stage->info.size.x, (i32)stage->info.size.y);

        //Render to Image
        v2f origin{0.0f, 0.0f};
        origin += 300.0f;
        origin.x += 100.0f;
    };

    if (globalPlatformServices->DLLJustReloaded)
    {
        BGZ_CONSOLE("Dll reloaded!");
        globalPlatformServices->DLLJustReloaded = false;
    };

    if(KeyHeld(keyboard->MoveRight))
    {
        player->world.pos.x += 10.0f;
        enemy->world.pos.x += 5.0f;
    };

    //Essentially local fighter coordinates
    Rectf playerTargetRect = ProduceRectFromBottomLeftPoint(v2f{0.0f, 0.0f}, (f32)player->image.size.width, (f32)player->image.size.height);
    Rectf enemyTargetRect = ProduceRectFromBottomLeftPoint(v2f{0.0f, 0.0f}, (f32)enemy->image.size.width, (f32)enemy->image.size.height);
    Rectf enemy2TargetRect = ProduceRectFromBottomLeftPoint(v2f{0.0f, 0.0f}, (f32)enemy2->image.size.width, (f32)enemy2->image.size.height);

    { // Render
        player->image.opacity = Clamp(player->image.opacity, 0.0f, 1.0f); 
        enemy->image.opacity = Clamp(enemy->image.opacity, 0.0f, 1.0f); 

        auto WorldTransform = [](Rectf localCoords, Fighter fighterInfo) -> Rectf
        {
            Rectf transformedCoords{};

            //With world space origin at 0, 0
            Coordinate_Space fighterSpace{};
            fighterSpace.origin = fighterInfo.world.pos;
            fighterSpace.xBasis = fighterInfo.world.scale * v2f{CosInRadians(Radians(fighterInfo.world.rotation)), SinInRadians(Radians(fighterInfo.world.rotation))};
            fighterSpace.yBasis = 2.5f* PerpendicularOp(fighterSpace.xBasis);

            //This equation rotates first then moves to correct world position
            transformedCoords.min = fighterSpace.origin + (localCoords.min.x * fighterSpace.xBasis) + (localCoords.min.y * fighterSpace.yBasis);
            transformedCoords.max = fighterSpace.origin + (localCoords.max.x * fighterSpace.xBasis) + (localCoords.max.y * fighterSpace.yBasis);

            return transformedCoords;
        };

        playerTargetRect = WorldTransform(playerTargetRect, *player);
        enemyTargetRect  = WorldTransform(enemyTargetRect, *enemy);
        enemy2TargetRect = WorldTransform(enemy2TargetRect, *enemy2);

        Rectf backgroundTargetRect{v2f{0, 0}, v2f{(f32)stage->info.backgroundImg.size.width, (f32)stage->info.backgroundImg.size.height}};
        DrawRectangle($(gState->colorBuffer), backgroundTargetRect, .5f, .5f, .5f);
        DrawImageSlowly($(gState->colorBuffer), playerTargetRect, player->image, gState->normalMap);
    };
};
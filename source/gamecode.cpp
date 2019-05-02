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

void ConvertNegativeAngleToRadians(f32&& angle)
{
    f32 circumferenceInRadians = 2*PI;
    angle = Mod(angle, circumferenceInRadians);
    if (angle < 0) angle += circumferenceInRadians;
};

void ConvertToCorrectPositiveRadian(f32&& angle)
{
    f32 unitCircleCircumferenceInRadians = 2*PI;
    angle = Mod(angle, unitCircleCircumferenceInRadians);
};

//For images that move/rotate/scale - Assumes pre-multiplied alpha
local_func void
DrawImageSlowly(Image&& buffer, Quad worldCoords, Image image, f32 lightAngle = {}, f32 lightThreshold = {}, Image normalMap = {}, f32 rotation = {}, v2f scale = {1.0f, 1.0f})
{    
    auto Grab4NearestPixelPtrs_SquarePattern = [](ui8* pixelToSampleFrom, ui32 pitch) -> v4ui
    {
        v4ui result{};

        result.x = *(ui32*)(pixelToSampleFrom);
        result.y = *(ui32*)(pixelToSampleFrom + sizeof(ui32));
        result.z = *(ui32*)(pixelToSampleFrom + pitch);
        result.w = *(ui32*)(pixelToSampleFrom + pitch + sizeof(ui32));

        return result;
    };

    auto BiLinearLerp = [](v4ui pixelsToLerp, f32 percentToLerpInX, f32 percentToLerpInY) -> v4f
    {
        v4f newBlendedValue {};
        v4f pixelA = UnPackPixelValues(pixelsToLerp.x, BGRA);
        v4f pixelB = UnPackPixelValues(pixelsToLerp.y, BGRA);
        v4f pixelC = UnPackPixelValues(pixelsToLerp.z, BGRA);
        v4f pixelD = UnPackPixelValues(pixelsToLerp.w, BGRA);

        v4f ABLerpColor = Lerp(pixelA, pixelB, percentToLerpInX);
        v4f CDLerpColor = Lerp(pixelC, pixelD, percentToLerpInX);
        newBlendedValue = Lerp(ABLerpColor, CDLerpColor, percentToLerpInY);

        return newBlendedValue;
    };

    v2f origin = worldCoords.bottomLeft;
    v2f targetRectXAxis = worldCoords.bottomRight - origin;
    v2f targetRectYAxis = worldCoords.topLeft - origin;

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

                v4ui texelSquare = Grab4NearestPixelPtrs_SquarePattern(texelPtr, image.pitch);

                //Blend between all 4 pixels to produce new color for sub pixel accruacy - Bilinear filtering
                v4f newBlendedTexel = BiLinearLerp(texelSquare, (texelPosX - Floor(texelPosX)), (texelPosY - Floor(texelPosY)));

                //Linearly Blend with background - Assuming Pre-multiplied alpha
                v4f backgroundColors = UnPackPixelValues(*destPixel, BGRA);
                f32 alphaBlend = newBlendedTexel.a / 255.0f;
                v4f finalBlendedColor = (1.0f - alphaBlend)*backgroundColors + newBlendedTexel;

                b shadePixel{false};
                if(normalMap.data)
                {
                    ui8* normalPtr = ((ui8*)normalMap.data) + ((ui32)texelPosY*image.pitch) + ((ui32)texelPosX*sizeof(ui32));//size of pixel

                    //Grab 4 normals (in a square pattern) to blend
                    v4ui normalSquare = Grab4NearestPixelPtrs_SquarePattern(normalPtr, normalMap.pitch);

                    v4f blendedNormal = BiLinearLerp(normalSquare, (texelPosX - Floor(texelPosX)), (texelPosY - Floor(texelPosY)));

                    //Convert normal from color value range (0 - 255) to vector range (-1 to 1)
                    f32 inv255 = 1.0f / 255.0f;
                    blendedNormal.x = -1.0f + 2.0f*(inv255*blendedNormal.x);
                    blendedNormal.y = -1.0f + 2.0f*(inv255*blendedNormal.y);
                    blendedNormal.z = -1.0f + 2.0f*(inv255*blendedNormal.z);


                    {//Rotating and scaling normals (supports non-uniform scaling of normal x and y)
                        v2f normalXBasis = v2f{CosR(rotation), SinR(rotation)};
                        v2f normalYBasis = scale.y * PerpendicularOp(normalXBasis);
                        normalXBasis *= scale.x;

                        normalXBasis *= (Magnitude(normalYBasis) / Magnitude(normalXBasis));
                        normalYBasis *= (Magnitude(normalXBasis) / Magnitude(normalYBasis));

                        blendedNormal.xy = (blendedNormal.x * normalXBasis) + (blendedNormal.y * normalYBasis);
                    };

					Normalize($(blendedNormal.xyz));

                    if(blendedNormal.z > 0.0f)                
                    {
                        *destPixel = ((0xFF << 24) |
                            ((ui8)finalBlendedColor.r << 16) |
                            ((ui8)finalBlendedColor.g << 8) |
                            ((ui8)finalBlendedColor.b << 0));
                    }
                    else
                    {
                        if (lightAngle > (PI*2)) ConvertToCorrectPositiveRadian($(lightAngle));
                        
                        f32 normalAngle{};
                        {//Calculate correct raidan angle from normal
                            if((blendedNormal.x + epsilon) > 0.0f && blendedNormal.y > 0.0f)
                            {
                                normalAngle = InvTanR(blendedNormal.y / blendedNormal.x);
                            }
                            else if(blendedNormal.x < 0.0f && blendedNormal.y > 0.0f)
                            {
                                normalAngle = InvTanR(blendedNormal.x / blendedNormal.y);
                                AbsoluteVal($(normalAngle));
                                normalAngle += (PI / 2.0f);
                            }
                            else if(blendedNormal.x < 0.0f && blendedNormal.y < 0.0f)
                            {
                                normalAngle = InvTanR(blendedNormal.x / blendedNormal.y);
                                normalAngle -= ((3.0f*PI) / 2.0f);
                                AbsoluteVal($(normalAngle));
                            }
                            else if((blendedNormal.x + epsilon) > 0.0f && blendedNormal.y < 0.0f)
                            {
                                normalAngle = InvTanR(blendedNormal.y / blendedNormal.x);
                                ConvertNegativeAngleToRadians($(normalAngle));
                            }
                        };

                        f32 maxAngle = Max(lightAngle, normalAngle);
                        f32 minAngle = Min(lightAngle, normalAngle);
                        f32 shadeThreholdDirection1 = maxAngle - minAngle;
                        f32 shadeThreholdDirection2 = ((PI*2) - maxAngle) + minAngle;

                        if(shadeThreholdDirection1 > lightThreshold || shadeThreholdDirection2 < lightThreshold)
                            shadePixel = true;
                    }

                    if(shadePixel && finalBlendedColor.a > 100.0f)
                    {
                            //Shade pixel
                        *destPixel = ((0xFF << 24) |
                            (0 << 16) |
                            (0 << 8) |
                            (0 << 0));
                    }
                    else
                    {
                        *destPixel = ((0xFF << 24) |
                            ((ui8)finalBlendedColor.r << 16) |
                            ((ui8)finalBlendedColor.g << 8) |
                            ((ui8)finalBlendedColor.b << 0));
                    }
                }
                else
                {
                        *destPixel = ((0xFF << 24) |
                            ((ui8)finalBlendedColor.r << 16) |
                            ((ui8)finalBlendedColor.g << 8) |
                            ((ui8)finalBlendedColor.b << 0));
                }
            }

            ++destPixel;
        }
        
        currentRow += buffer.pitch;
    }
}

void RenderToImage(Image&& renderTarget, Image sourceImage, Quad targetArea)
{
    DrawImageSlowly($(renderTarget), targetArea, sourceImage, 0.0f);
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

    if (NOT gameMemory->Initialized)
    {
        BGZ_ERRCTXT1("When Initializing game memory and game state");

        gameMemory->Initialized = true;
        *gState = {}; //Make sure everything gets properly defaulted (constructors are called that need to be)

        gState->colorBuffer.data = (ui8*)gameBackBuffer->memory;
        gState->colorBuffer.size = v2i{gameBackBuffer->width, gameBackBuffer->height};
        gState->colorBuffer.pitch = gameBackBuffer->pitch;

        viewportWidth = 1280.0f;
        viewportHeight = 720.0f;

        InitApplicationMemory(gameMemory);
        CreateRegionFromMemory(gameMemory, Megabytes(500));

        //Stage Init
        stage->info.size.x = viewportWidth;
        stage->info.size.y = viewportHeight;
        stage->info.backgroundImg.data = platformServices->LoadBGRAbitImage("data/mountain.jpg", $(stage->info.backgroundImg.size.width), $(stage->info.backgroundImg.size.height));

        //Camera Init

        //Player Init
        player->image.data = platformServices->LoadBGRAbitImage("data/left-bicep.png", $(player->image.size.width), $(player->image.size.height));
        player->image.pitch = player->image.size.width * bytesPerPixel;
        player->world.pos = {300.0f, 100.0f};
        player->world.rotation = 0.0f;
        player->world.scale = {1.0f, 1.0f};
        player->image.opacity = .5f;
        
        //Enemy Init
        enemy->image.data = platformServices->LoadBGRAbitImage("data/test_body_back.bmp", $(enemy->image.size.width), $(enemy->image.size.height));
        enemy->image.pitch = enemy->image.size.width * bytesPerPixel;
        enemy->world.pos = {200.0f, 150.0f};
        enemy->world.rotation = 0.0f;
        enemy->world.scale = {2.3f, 2.3f};
        enemy->image.opacity = .7f;

        gState->normalMap.data = platformServices->LoadBGRAbitImage("data/test.png", $(gState->normalMap.size.width), $(gState->normalMap.size.height));

        //Create empty image
        auto CreateEmptyImage = [](i32 width, i32 height) -> Image
                            {
                                Image image{};

                                image.data = (ui8*)CallocSize(0, width*height*bytesPerPixel);
                                image.size = v2i{width, height};
                                image.pitch = width*bytesPerPixel;

                                return image;
                            };

        auto GenerateSphereNormalMap = [](Image&& sourceImage) -> void
                            {
                                f32 invWidth = 1.0f / (f32)(sourceImage.size.width - 1);
                                f32 invHeight = 1.0f / (f32)(sourceImage.size.height - 1);
                                
                                ui8* row = (ui8*)sourceImage.data;
                                for(i32 y = 0; y < sourceImage.size.height; ++y)
                                {
                                    ui32* pixel = (ui32*)row;

                                    for(i32 x = 0; x < sourceImage.size.width; ++x)
                                    {
                                        v2f normalUV = {invWidth*(f32)x, invHeight*(f32)y};
                                        
                                        f32 normalX = 2.0f*normalUV.x - 1.0f;
                                        f32 normalY = 2.0f*normalUV.y - 1.0f;
                                        
                                        f32 rootTerm = 1.0f - normalX*normalX - normalY*normalY;
                                        f32 normalZ = 0.0f;

                                        v3f normal {0.0f, 0.0f, 1.0f};

                                        if(rootTerm >= 0.0f)
                                        {
                                            normalZ = Sqrt(rootTerm);
                                            normal = v3f{normalX, normalY, normalZ};
                                        };

                                        //Convert from -1 to 1 range to value between 0 and 255
                                        v4f color = {255.0f*(.5f*(normal.x + 1.0f)),
                                                     255.0f*(.5f*(normal.y + 1.0f)),
                                                     255.0f*(.5f*(normal.z + 1.0f)),
                                                     0.0f};

                                        *pixel++ = (((ui8)(color.a + .5f) << 24) |
                                                     ((ui8)(color.r + .5f) << 16) |
                                                     ((ui8)(color.g + .5f) << 8) |
                                                     ((ui8)(color.b + .5f) << 0));
                                    }

                                    row += sourceImage.pitch;
                                }
                            };
        
        gState->composite = CreateEmptyImage((i32)stage->info.size.x, (i32)stage->info.size.y);

        gState->lightThreshold = 1.0f;
        gState->lightAngle = 1.0f;

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
        player->world.rotation += .01f;
    };

    //Essentially local fighter coordinates
    Quad playerTargetRect = ProduceQuadFromBottomLeftPoint(v2f{0.0f, 0.0f}, (f32)player->image.size.width, (f32)player->image.size.height);
    Quad enemyTargetRect = ProduceQuadFromBottomLeftPoint(v2f{0.0f, 0.0f}, (f32)enemy->image.size.width, (f32)enemy->image.size.height);

    ConvertToCorrectPositiveRadian($(player->world.rotation));

    { // Render
        auto WorldTransform = [](Quad localCoords, Fighter fighterInfo) -> Quad
        {
            //With world space origin at 0, 0
            Coordinate_Space fighterSpace{};
            fighterSpace.origin = fighterInfo.world.pos;
            fighterSpace.xBasis = v2f{CosR(fighterInfo.world.rotation), SinR(fighterInfo.world.rotation)};
            fighterSpace.yBasis = fighterInfo.world.scale.y * PerpendicularOp(fighterSpace.xBasis);
            fighterSpace.xBasis *= fighterInfo.world.scale.x;

            Quad transformedCoords{};
            for(i32 vertIndex{}; vertIndex < transformedCoords.vertices.Size(); ++vertIndex)
            {
                //This equation rotates first then moves to correct world position
                transformedCoords.vertices.At(vertIndex) = fighterSpace.origin + (localCoords.vertices.At(vertIndex).x * fighterSpace.xBasis) + (localCoords.vertices.At(vertIndex).y * fighterSpace.yBasis);
            };

            return transformedCoords;
        };

        playerTargetRect = WorldTransform(playerTargetRect, *player);
        enemyTargetRect  = WorldTransform(enemyTargetRect, *enemy);

        Rectf backgroundTargetRect{v2f{0, 0}, v2f{(f32)stage->info.backgroundImg.size.width, (f32)stage->info.backgroundImg.size.height}};
        DrawImage($(gState->colorBuffer), backgroundTargetRect, gState->stage.info.backgroundImg);
        DrawImageSlowly($(gState->colorBuffer), playerTargetRect, player->image, gState->lightAngle, gState->lightThreshold, gState->normalMap, player->world.rotation, player->world.scale);
    };
};
#ifndef RENDERER_STUFF_INCLUDE_H
#define RENDERER_STUFF_INCLUDE_H

struct Quadf
{
    union
    {
        Array<v2f, 4> vertices;
        struct
        {
            v2f bottomLeft;
            v2f bottomRight;
            v2f topRight;
            v2f topLeft;
        };
    };
};

struct Quadi
{
    union
    {
        Array<v2i, 4> vertices;
        struct
        {
            v2i bottomLeft;
            v2i bottomRight;
            v2i topRight;
            v2i topLeft;
        };
    };
};

enum Render_Entry_Type
{
    EntryType_Image
};

struct RenderEntry_Header
{
    Render_Entry_Type type;
};

struct RenderEntry_Image
{
    RenderEntry_Header header;
    Transform world;
    Image normalMap;
    Image imageData;
};

void PushImage(Game_Render_Cmds&& bufferInfo, Image imageToDraw, Image normalMap, Transform image_worldTransformInfo);
void Render(Game_State* gState, Image&& colorBuffer, Game_Render_Cmds renderBufferInfo, Game_Camera camera);

void ConvertNegativeAngleToRadians(f32&& angle);
void ConvertToCorrectPositiveRadian(f32&& angle);
void RenderToImage(Image&& renderTarget, Image sourceImage, Quadf targetArea);

#endif //RENDERER_STUFF_INCLUDE_H 

#ifdef RENDERER_STUFF_IMPL

void* _RenderCmdBuf_Push(Game_Render_Cmds* commandBuf, i32 sizeOfCommand)
{
    void* memoryPointer = (void*)(commandBuf->baseAddress + commandBuf->usedAmount);
    commandBuf->usedAmount += (sizeOfCommand);

    return memoryPointer;
};

#define RenderCmdBuf_Push(commandBuffer, commandType) (commandType*)_RenderCmdBuf_Push(commandBuffer, sizeof(commandType))

local_func
Rectf _DilateAboutArbitraryPoint(v2f PointOfDilation, f32 ScaleFactor, Rectf RectToDilate)
{
    Rectf DilatedRect {};

    v2f Distance = PointOfDilation - RectToDilate.min;
    Distance *= ScaleFactor;
    DilatedRect.min = PointOfDilation - Distance;

    Distance = PointOfDilation - RectToDilate.max;
    Distance *= ScaleFactor;
    DilatedRect.max = PointOfDilation - Distance;

    return DilatedRect;
};

auto _DilateAboutArbitraryPoint(v2f PointOfDilation, f32 ScaleFactor, Quadf QuadToDilate) -> Quadf
{
    Quadf DilatedQuad {};

    for (i32 vertIndex = 0; vertIndex < 4; ++vertIndex)
    {
        v2f Distance = PointOfDilation - QuadToDilate.vertices[vertIndex];
        Distance *= ScaleFactor;
        DilatedQuad.vertices[vertIndex] = PointOfDilation - Distance;
    };

    return DilatedQuad;
};

Quadf WorldTransform(Quadf localCoords, Transform transformInfo_world)
{
    //With world space origin at 0, 0
    Coordinate_Space imageSpace{};
    imageSpace.origin = transformInfo_world.pos;
    imageSpace.xBasis = v2f{CosR(transformInfo_world.rotation), SinR(transformInfo_world.rotation)};
    imageSpace.yBasis = transformInfo_world.scale.y * PerpendicularOp(imageSpace.xBasis);
    imageSpace.xBasis *= transformInfo_world.scale.x;

    Quadf transformedCoords{};
    for(i32 vertIndex{}; vertIndex < transformedCoords.vertices.Size(); ++vertIndex)
    {
        //This equation rotates first then moves to correct world position
        transformedCoords.vertices.At(vertIndex) = imageSpace.origin + (localCoords.vertices.At(vertIndex).x * imageSpace.xBasis) + (localCoords.vertices.At(vertIndex).y * imageSpace.yBasis);
    };

    return transformedCoords;
};

Quadf CameraTransform(Quadf worldCoords, Game_Camera camera)
{
    Quadf transformedCoords{};

    v2f translationToCameraSpace = camera.viewCenter - camera.lookAt;

    for(i32 vertIndex{}; vertIndex < 4; vertIndex++) 
    {
        worldCoords.vertices[vertIndex] += translationToCameraSpace;
    };

    transformedCoords = _DilateAboutArbitraryPoint(camera.dilatePoint, camera.zoomFactor, worldCoords);

    return transformedCoords;
};

local_func
auto _LinearBlend(ui32 foregroundColor, ui32 backgroundColor, ChannelType colorFormat) 
{
    struct Result {ui8 blendedPixel_R, blendedPixel_G, blendedPixel_B;};
    Result blendedColor{};
    
    v4f foreGroundColors = UnPackPixelValues(foregroundColor, colorFormat);
    v4f backgroundColors = UnPackPixelValues(backgroundColor, colorFormat);

    f32 blendPercent = foreGroundColors.a / 255.0f;

    blendedColor.blendedPixel_R = (ui8)Lerp(backgroundColors.r, foreGroundColors.r, blendPercent);
    blendedColor.blendedPixel_G = (ui8)Lerp(backgroundColors.g, foreGroundColors.g, blendPercent);
    blendedColor.blendedPixel_B = (ui8)Lerp(backgroundColors.b, foreGroundColors.b, blendPercent);

    return blendedColor;
};

local_func
Rectf _ProduceRectFromCenterPoint(v2f OriginPoint, f32 width, f32 height)
{
    Rectf Result;

    Result.min = { OriginPoint.x - (width / 2), OriginPoint.y - (height / 2) };
    Result.max = { OriginPoint.x + (width / 2), OriginPoint.y + (height / 2) };

    return Result;
};

local_func
Rectf _ProduceRectFromBottomMidPoint(v2f OriginPoint, f32 width, f32 height)
{
    Rectf Result;

    Result.min = { OriginPoint.x - (width / 2.0f), OriginPoint.y };
    Result.max = { OriginPoint.x + (width / 2.0f), OriginPoint.y + height };

    return Result;
};

local_func
Rectf _ProduceRectFromBottomLeftPoint(v2f originPoint, f32 width, f32 height)
{
    Rectf Result;

    Result.min = originPoint;
    Result.max = { originPoint.x + width, originPoint.y + height };

    return Result;
};

local_func
Quadf _ProduceQuadFromBottomLeftPoint(v2f originPoint, f32 width, f32 height)
{
    Quadf Result;

    Result.bottomLeft = originPoint;
    Result.bottomRight = { originPoint.x + width, originPoint.y };
    Result.topRight = { originPoint.x + width, originPoint.y + height };
    Result.topLeft = { originPoint.x, originPoint.y + height };

    return Result;
};

local_func
void ConvertNegativeAngleToRadians(f32&& angle)
{
    f32 circumferenceInRadians = 2*PI;
    angle = Mod(angle, circumferenceInRadians);
    if (angle < 0) angle += circumferenceInRadians;
};

local_func
void ConvertToCorrectPositiveRadian(f32&& angle)
{
    f32 unitCircleCircumferenceInRadians = 2*PI;
    angle = Mod(angle, unitCircleCircumferenceInRadians);
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
};

//Avoids a lot of the calcuations found in DrawImageSlowly for better performance (Since background won't rotate or
//do other fancier things. Though it will still scale)
local_func void
DrawBackground(Image&& buffer, Quadf targetQuad, Image image)
{
    ui32* imagePixel = (ui32*)image.data;

    v2f targetQuadOrigin = targetQuad.bottomLeft;
    v2f targetQuadXAxis = targetQuad.bottomRight - targetQuadOrigin;
    v2f targetQuadYAxis = targetQuad.topLeft - targetQuadOrigin;

    ui8* currentRow = (ui8*)buffer.data; 
    f32 invertedXAxisSqd = 1.0f / MagnitudeSqd(targetQuadXAxis);
    f32 invertedYAxisSqd = 1.0f / MagnitudeSqd(targetQuadYAxis);

    for(i32 screenPixelColumn = 0; screenPixelColumn < buffer.size.height; ++screenPixelColumn)
    {
        ui32* destPixel = (ui32*)currentRow;

        for(i32 screenPixelRow = 0; screenPixelRow < buffer.size.width; ++screenPixelRow)
        {            
            v2f screenPixelCoord{(f32)screenPixelRow, (f32)screenPixelColumn};
            v2f d {screenPixelCoord - targetQuadOrigin};

            f32 u = invertedXAxisSqd * DotProduct(d, targetQuadXAxis);
            f32 v = invertedYAxisSqd * DotProduct(d, targetQuadYAxis);

            f32 texelPosX = 1.0f + (u*(f32)(image.size.width));
            f32 texelPosY = 1.0f + (v*(f32)(image.size.height)); 

            BGZ_ASSERT(texelPosX <= (f32)image.size.width, "Trying to fill in pixel outside current background image width boundry!");
            BGZ_ASSERT(texelPosY <= (f32)image.size.height, "Trying to fill in pixel outside current background image height boundry!");

            ui32* backgroundTexel = (ui32*)(((ui8*)image.data) + ((ui32)texelPosY*image.pitch) + ((ui32)texelPosX*sizeof(ui32)));//size of pixel

            v4f imagePixel = UnPackPixelValues(*backgroundTexel, BGRA);

            *destPixel = ((0xFF << 24) |
                           ((ui8)imagePixel.r << 16) |
                           ((ui8)imagePixel.g << 8) |
                           ((ui8)imagePixel.b << 0));

            ++destPixel;
        }
        
        currentRow += buffer.pitch;
    }
};

//For images that move/rotate/scale - Assumes pre-multiplied alpha
local_func void
DrawImageSlowly(Image&& buffer, Quadf cameraCoords, Image image, f32 lightAngle = {}, f32 lightThreshold = {}, Image normalMap = {}, f32 rotation = {}, v2f scale = {1.0f, 1.0f})
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

    v2f origin = cameraCoords.bottomLeft;
    v2f targetRectXAxis = cameraCoords.bottomRight - origin;
    v2f targetRectYAxis = cameraCoords.topLeft - origin;

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
                //Gather normalized coordinates (uv's) in order to find the correct texel position below
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

void RenderToImage(Image&& renderTarget, Image sourceImage, Quadf targetArea)
{
    DrawImageSlowly($(renderTarget), targetArea, sourceImage, 0.0f);
};

void PushImage(Game_Render_Cmds* bufferInfo, Image imageToDraw, Image normalMap, Transform image_worldTransformInfo)
{
    RenderEntry_Image* imageEntry = RenderCmdBuf_Push(bufferInfo, RenderEntry_Image);

    imageEntry->header.type = EntryType_Image;
    imageEntry->world = image_worldTransformInfo;
    imageEntry->normalMap = normalMap;
    imageEntry->imageData = imageToDraw;

    ++bufferInfo->entryCount;
};

void Render(Game_State* gState, Image&& colorBuffer, Game_Render_Cmds renderBufferInfo, Game_Camera camera)
{
    ui8* currentRenderBufferEntry = renderBufferInfo.baseAddress;
    for(i32 entryNumber = 0; entryNumber < renderBufferInfo.entryCount; ++entryNumber)
    {
        RenderEntry_Header* entryHeader = (RenderEntry_Header*)currentRenderBufferEntry;
        switch(entryHeader->type)
        {
            case EntryType_Image:
            {
                RenderEntry_Image* imageEntry = (RenderEntry_Image*)currentRenderBufferEntry;
                Quadf imageTargetRect = _ProduceQuadFromBottomLeftPoint(v2f{0.0f, 0.0f}, (f32)imageEntry->imageData.size.width, (f32)imageEntry->imageData.size.height);

                ConvertToCorrectPositiveRadian($(imageEntry->world.rotation));

                Quadf imageTargetRect_world = WorldTransform(imageTargetRect, imageEntry->world);
                Quadf imageTargetRect_camera = CameraTransform(imageTargetRect_world, camera);

                DrawImageSlowly($(colorBuffer), imageTargetRect_camera, imageEntry->imageData, gState->lightAngle, gState->lightThreshold, imageEntry->normalMap, imageEntry->world.rotation, imageEntry->world.scale);

                currentRenderBufferEntry += sizeof(RenderEntry_Image);
            }break;

            InvalidDefaultCase;
        }
    };
};

#endif //RENDERER_STUFF_IMPL
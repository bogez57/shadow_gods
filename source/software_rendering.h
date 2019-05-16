#ifndef SOFTWARE_RENDERING_INCLUDE_H
#define SOFTWARE_RENDERING_INCLUDE_H

#include "renderer_stuff.h"

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

    ui8* currentRow = ((ui8*)buffer.data + rectToDraw.min.x*BYTES_PER_PIXEL+ rectToDraw.min.y*buffer.pitch);
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

    ui8* currentRow = ((ui8*)buffer.data + targetRect.min.x*BYTES_PER_PIXEL+ targetRect.min.y*buffer.pitch);
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
//TODO: Will probably end up removing and just combine with DrawImage call so there is just one imaging drawing func
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

void DrawImageQuickly(Image&& buffer, Quadf cameraCoords, Image image, Image normalMap, f32 rotation, v2f scale) 
{
    auto Grab4NearestPixelPtrs_SquarePattern = [](ui8* pixelToSampleFrom, ui32 pitch) -> v4ui32
    {
        v4ui32 result{};

        result.x = *(ui32*)(pixelToSampleFrom);
        result.y = *(ui32*)(pixelToSampleFrom + sizeof(ui32));
        result.z = *(ui32*)(pixelToSampleFrom + pitch);
        result.w = *(ui32*)(pixelToSampleFrom + pitch + sizeof(ui32));

        return result;
    };

    auto BiLinearLerp = [](v4ui32 pixelsToLerp, f32 percentToLerpInX, f32 percentToLerpInY) -> v4f
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
    ui8* currentRow = (ui8*)buffer.data + (i32)xMin * 4+ (i32)yMin * buffer.pitch; 

    for(f32 screenY = yMin; screenY < yMax; ++screenY)
    {
        ui32* destPixel = (ui32*)currentRow;
        for(f32 screenX = xMin; screenX < xMax; ++screenX)
        {            
            v2f screenPixelCoord{screenX, screenY};
            v2f d {screenPixelCoord - origin};

            f32 u = invertedXAxisSqd * DotProduct(d, targetRectXAxis);
            f32 v = invertedYAxisSqd * DotProduct(d, targetRectYAxis);

            f32 epsilon = 0.00001f;//TODO: Remove????
            BGZ_ASSERT(((u + epsilon) >= 0.0f) && ((u - epsilon) <= 1.0f), "u is out of range! %f", u);
            BGZ_ASSERT(((v + epsilon) >= 0.0f) && ((v - epsilon) <= 1.0f), "v is out of range! %f", v);

            if(u >= 0.0f && u <= 1.0f && v >= 0.0f && v <= 1.0f)
            {
                //Gather normalized coordinates (uv's) in order to find the correct texel position below
                f32 texelPosX = 1.0f + (u*(f32)(image.size.width - 3));
                f32 texelPosY = 1.0f + (v*(f32)(image.size.height - 3)); 
                
                BGZ_ASSERT((texelPosX >= 0) && (texelPosX <= (i32)image.size.width), "x coord is out of range!: ");
                BGZ_ASSERT((texelPosY >= 0) && (texelPosY <= (i32)image.size.height), "y coord is out of range!");

                //Gather 4 texels (in a square pattern) from certain texel Ptr
                v4ui32 texelSquare {}; 
                ui8* texelPtr = ((ui8*)image.data) + ((ui32)texelPosY*image.pitch) + ((ui32)texelPosX*sizeof(ui32));//size of pixel
                texelSquare.x = *(ui32*)(texelPtr);
                texelSquare.y = *(ui32*)(texelPtr + sizeof(ui32));
                texelSquare.z = *(ui32*)(texelPtr + image.pitch);
                texelSquare.w = *(ui32*)(texelPtr + image.pitch + sizeof(ui32));

                //Unpack individual color values from pixels found in texel square
                v4f pixelA {}, pixelB {}, pixelC {}, pixelD {};
                {
                    ui32* pixel1 = &texelSquare.x;
                    pixelA.b = (f32)*((ui8*)pixel1 + 0);
                    pixelA.g = (f32)*((ui8*)pixel1 + 1);
                    pixelA.r = (f32)*((ui8*)pixel1 + 2);
                    pixelA.a = (f32)*((ui8*)pixel1 + 3);

                    ui32* pixel2 = &texelSquare.y;
                    pixelB.b = (f32)*((ui8*)pixel2 + 0);
                    pixelB.g = (f32)*((ui8*)pixel2 + 1);
                    pixelB.r = (f32)*((ui8*)pixel2 + 2);
                    pixelB.a = (f32)*((ui8*)pixel2 + 3);

                    ui32* pixel3 = &texelSquare.z;
                    pixelC.b = (f32)*((ui8*)pixel3 + 0);
                    pixelC.g = (f32)*((ui8*)pixel3 + 1);
                    pixelC.r = (f32)*((ui8*)pixel3 + 2);
                    pixelC.a = (f32)*((ui8*)pixel3 + 3);

                    ui32* pixel4 = &texelSquare.w;
                    pixelD.b = (f32)*((ui8*)pixel4 + 0);
                    pixelD.g = (f32)*((ui8*)pixel4 + 1);
                    pixelD.r = (f32)*((ui8*)pixel4 + 2);
                    pixelD.a = (f32)*((ui8*)pixel4 + 3);
                };
                
                f32 percentToLerpInX = texelPosX - Floor(texelPosX);
                f32 percentToLerpInY = texelPosY - Floor(texelPosY);

                v4f ABLerpColor = (1.0f - percentToLerpInX)*pixelA + percentToLerpInX*pixelB;
                v4f CDLerpColor = (1.0f - percentToLerpInX)*pixelC + percentToLerpInX*pixelD;
                v4f newBlendedTexel = (1.0f - percentToLerpInY)*ABLerpColor + percentToLerpInY*CDLerpColor; 

                {//Linearly Blend with background - Assuming Pre-multiplied alpha
                    //Unpack individual color values from dest pixel
                    v4f backgroundColors{};
                    backgroundColors.b = (f32)*((ui8*)destPixel + 0);
                    backgroundColors.g = (f32)*((ui8*)destPixel + 1);
                    backgroundColors.r = (f32)*((ui8*)destPixel + 2);
                    backgroundColors.a = (f32)*((ui8*)destPixel + 3);

                    f32 alphaBlend = newBlendedTexel.a / 255.0f;
                    v4f finalBlendedColor = (1.0f - alphaBlend)*backgroundColors + newBlendedTexel;

                    *destPixel = ((0xFF << 24) |
                                ((ui8)finalBlendedColor.r << 16) |
                                ((ui8)finalBlendedColor.g << 8) |
                                ((ui8)finalBlendedColor.b << 0));
                };

                ++destPixel;
            }
        }
        
        currentRow += buffer.pitch;
    }

};

//For images that move/rotate/scale - Assumes pre-multiplied alpha
//Also involves lighting calcuation
//TODO: Will eventually be removed entirely/combine with DrawImageQuickly
local_func void
DrawImageSlowly(Image&& buffer, Quadf cameraCoords, Image image, Image normalMap, f32 rotation, v2f scale, f32 lightAngle = {}, f32 lightThreshold = {})
{    
    auto Grab4NearestPixelPtrs_SquarePattern = [](ui8* pixelToSampleFrom, ui32 pitch) -> v4ui32
    {
        v4ui32 result{};

        result.x = *(ui32*)(pixelToSampleFrom);
        result.y = *(ui32*)(pixelToSampleFrom + sizeof(ui32));
        result.z = *(ui32*)(pixelToSampleFrom + pitch);
        result.w = *(ui32*)(pixelToSampleFrom + pitch + sizeof(ui32));

        return result;
    };

    auto BiLinearLerp = [](v4ui32 pixelsToLerp, f32 percentToLerpInX, f32 percentToLerpInY) -> v4f
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
    ui8* currentRow = (ui8*)buffer.data + (i32)xMin * 4+ (i32)yMin * buffer.pitch; 

    for(f32 screenY = yMin; screenY < yMax; ++screenY)
    {
        ui32* destPixel = (ui32*)currentRow;
        for(f32 screenX = xMin; screenX < xMax; ++screenX)
        {            
            v2f screenPixelCoord{screenX, screenY};
            v2f d {screenPixelCoord - origin};

            f32 u = invertedXAxisSqd * DotProduct(d, targetRectXAxis);
            f32 v = invertedYAxisSqd * DotProduct(d, targetRectYAxis);

            if(u >= 0.0f && u <= 1.0f && v >= 0.0f && v <= 1.0f)
            {
                //Gather normalized coordinates (uv's) in order to find the correct texel position below
                f32 texelPosX = 1.0f + (u*(f32)(image.size.width - 3));
                f32 texelPosY = 1.0f + (v*(f32)(image.size.height - 3)); 

                f32 epsilon = 0.00001f;//TODO: Remove????
                BGZ_ASSERT(((u + epsilon) >= 0.0f) && ((u - epsilon) <= 1.0f), "u is out of range! %f", u);
                BGZ_ASSERT(((v + epsilon) >= 0.0f) && ((v - epsilon) <= 1.0f), "v is out of range! %f", v);
                BGZ_ASSERT((texelPosX >= 0) && (texelPosX <= (i32)image.size.width), "x coord is out of range!: ");
                BGZ_ASSERT((texelPosY >= 0) && (texelPosY <= (i32)image.size.height), "x coord is out of range!");

                ui8* texelPtr = ((ui8*)image.data) + ((ui32)texelPosY*image.pitch) + ((ui32)texelPosX*sizeof(ui32));//size of pixel

                v4ui32 texelSquare {}; 
                texelSquare.x = *(ui32*)(texelPtr);
                texelSquare.y = *(ui32*)(texelPtr + sizeof(ui32));
                texelSquare.z = *(ui32*)(texelPtr + image.pitch);
                texelSquare.w = *(ui32*)(texelPtr + image.pitch + sizeof(ui32));

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
                    v4ui32 normalSquare = Grab4NearestPixelPtrs_SquarePattern(normalPtr, normalMap.pitch);

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
    //DrawImageSlowly($(renderTarget), targetArea, sourceImage, 0.0f);
};

void RenderViaSoftware(Image&& colorBuffer, Game_Render_Cmd_Buffer renderBufferInfo)
{
    ui8* currentRenderBufferEntry = renderBufferInfo.baseAddress;

    //First element should always be camera so I can use it for multiple entry's
    RenderEntry_2DCamera* camera = (RenderEntry_2DCamera*)currentRenderBufferEntry;
    currentRenderBufferEntry += sizeof(RenderEntry_2DCamera);

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
                Quadf imageTargetRect_camera = CameraTransform(imageTargetRect_world, *camera);

                DrawImageQuickly($(colorBuffer), imageTargetRect_camera, imageEntry->imageData, imageEntry->normalMap, imageEntry->world.rotation, imageEntry->world.scale);

                currentRenderBufferEntry += sizeof(RenderEntry_Image);
            }break;

            InvalidDefaultCase;
        }
    };
};

#endif //SOFTWARE_RENDERING_INCLUDE_H


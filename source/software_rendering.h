#ifndef SOFTWARE_RENDERING_INCLUDE_H
#define SOFTWARE_RENDERING_INCLUDE_H

#include "renderer_stuff.h"
#include "shared.h"

/*
    TODO:
*/


#if 0
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

            f32 texelPos_x = 1.0f + (u*(f32)(image.size.width));
            f32 texelPos_y = 1.0f + (v*(f32)(image.size.height)); 

            BGZ_ASSERT(texelPos_x <= (f32)image.size.width, "Trying to fill in pixel outside current background image width boundry!");
            BGZ_ASSERT(texelPos_y <= (f32)image.size.height, "Trying to fill in pixel outside current background image height boundry!");

            ui32* backgroundTexel = (ui32*)(((ui8*)image.data) + ((ui32)texelPos_y*image.pitch) + ((ui32)texelPos_x*sizeof(ui32)));//size of pixel

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
#endif

local_func void
DrawRectangle(ui32* colorBufferData, v2i colorBufferSize, i32 colorBufferPitch, Quadf cameraCoords, v2f rectDims, v3f rectColor, Rectf clipRect)
{    
    v2f origin = cameraCoords.bottomLeft;
    v2f targetRectXAxis = cameraCoords.bottomRight - origin;
    v2f targetRectYAxis = cameraCoords.topLeft - origin;

    ui32 pixelColor = {(0xFF << 24) |
                       (RoundFloat32ToUInt32(rectColor.r * 255.0f) << 16) |
                       (RoundFloat32ToUInt32(rectColor.g * 255.0f) << 8) |
                       (RoundFloat32ToUInt32(rectColor.b * 255.0f) << 0)};

    f32 widthMax = clipRect.max.x;
    f32 heightMax = clipRect.max.y;
    
    f32 xMin = widthMax;
    f32 xMax = clipRect.min.x;
    f32 yMin = heightMax;
    f32 yMax = clipRect.min.y;

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

        if(xMin < clipRect.min.x) {xMin = clipRect.min.x;}
        if(yMin < clipRect.min.y) {yMin = clipRect.min.y;}
        if(xMax > widthMax) {xMax = widthMax;}
        if(yMax > heightMax) {yMax = heightMax;}
    };

    f32 invertedXAxisSqd = 1.0f / MagnitudeSqd(targetRectXAxis);
    f32 invertedYAxisSqd = 1.0f / MagnitudeSqd(targetRectYAxis);

    ui8* currentRow = (ui8*)colorBufferData + (i32)xMin * 4+ (i32)yMin * colorBufferPitch; 
    for(f32 screenY = yMin; screenY < yMax; ++screenY)
    {
        ui32* destPixel = (ui32*)currentRow;

        for(f32 screenX = xMin; screenX < xMax; ++screenX)
        {            
            v2f screenPixelCoord{screenX, screenY};
            v2f d {screenPixelCoord - origin};

            f32 u = invertedXAxisSqd * DotProduct(d, targetRectXAxis);
            f32 v = invertedYAxisSqd * DotProduct(d, targetRectYAxis);

            //Used to decide, given what screen pixel were on, whether or not that screen pixel falls
            //within the target rect's bounds or not
            if(u >= 0.0f && u <= 1.0f && v >= 0.0f && v <= 1.0f)
            {
                *destPixel = pixelColor;
            }

            ++destPixel;
        }
        
        currentRow += colorBufferPitch;
    }
};



#include <immintrin.h>
void DrawTextureQuickly(ui32* colorBufferData, v2i colorBufferSize, i32 colorBufferPitch, Quadf cameraCoords, RenderEntry_Texture image, f32 rotation, v2f scale, Rectf clipRect) 
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
        v4f texelA = UnPackPixelValues(pixelsToLerp.x, BGRA);
        v4f texelB = UnPackPixelValues(pixelsToLerp.y, BGRA);
        v4f texelC = UnPackPixelValues(pixelsToLerp.z, BGRA);
        v4f texelD = UnPackPixelValues(pixelsToLerp.w, BGRA);

        v4f ABLerpColor = Lerp(texelA, texelB, percentToLerpInX);
        v4f CDLerpColor = Lerp(texelC, texelD, percentToLerpInX);
        newBlendedValue = Lerp(ABLerpColor, CDLerpColor, percentToLerpInY);

        return newBlendedValue;
    };

    v2f origin = cameraCoords.bottomLeft;
    v2f targetRectXAxis = cameraCoords.bottomRight - origin;
    v2f targetRectYAxis = cameraCoords.topLeft - origin;

    f32 widthMax = clipRect.max.x;
    f32 heightMax = clipRect.max.y;
    
    f32 xMin = widthMax;
    f32 xMax = clipRect.min.x;
    f32 yMin = heightMax;
    f32 yMax = clipRect.min.y;

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

        if(xMin < clipRect.min.x) {xMin = clipRect.min.x;}
        if(yMin < clipRect.min.y) {yMin = clipRect.min.y;}
        if(xMax > widthMax) {xMax = widthMax;}
        if(yMax > heightMax) {yMax = heightMax;}
    };

    //Pre calcuations for optimization
    f32 invertedXAxisSqd = 1.0f / MagnitudeSqd(targetRectXAxis);
    f32 invertedYAxisSqd = 1.0f / MagnitudeSqd(targetRectYAxis);
    i32 imageWidth = image.size.width - 3;
    i32 imageHeight = image.size.height - 3;
    v2f normalizedXAxis = invertedXAxisSqd * targetRectXAxis;
    v2f normalizedYAxis = invertedYAxisSqd * targetRectYAxis;

    ui8* currentRow = (ui8*)colorBufferData + (i32)xMin * 4+ (i32)yMin * colorBufferPitch; 
    for(f32 screenY = yMin; screenY < yMax; ++screenY)
    {
        ui32* destPixel = (ui32*)currentRow;
        for(f32 screenX = xMin; screenX < xMax; screenX += 8)
        {            
            //Initial setup variables for SIMD code
            __m256 one = _mm256_set1_ps(1.0f);
            __m256 zero = _mm256_set1_ps(0.0f);
            __m256 imgWidth = _mm256_set1_ps((f32)imageWidth);
            __m256 imgHeight = _mm256_set1_ps((f32)imageHeight);
            __m256 normalizedXAxis_x = _mm256_set1_ps(normalizedXAxis.x);
            __m256 normalizedXAxis_y = _mm256_set1_ps(normalizedXAxis.y);
            __m256 normalizedYAxis_x = _mm256_set1_ps(normalizedYAxis.x);
            __m256 normalizedYAxis_y = _mm256_set1_ps(normalizedYAxis.y);
            __m256 targetRectOrigin_x = _mm256_set1_ps(origin.x);
            __m256 targetRectOrigin_y = _mm256_set1_ps(origin.y);

            __m256 screenPixelCoords_x = _mm256_set_ps(screenX + 7, screenX + 6, screenX + 5, screenX + 4, screenX + 3, screenX + 2, screenX + 1, screenX + 0);
            __m256 screenPixelCoords_y = _mm256_set1_ps(screenY);
            
            __m256 uvRangeForTexture_u = _mm256_set1_ps(image.uvBounds.At(1).u - image.uvBounds.At(0).u);
            __m256 uvRangeForTexture_v = _mm256_set1_ps(image.uvBounds.At(1).v - image.uvBounds.At(0).v);

            __m256 minUVBounds_u = _mm256_set1_ps(image.uvBounds.At(0).u);
            __m256 minUVBounds_v = _mm256_set1_ps(image.uvBounds.At(0).v);

            //Gather normalized coordinates (uv's) in order to find the correct texel position below
            __m256 dXs = _mm256_sub_ps(screenPixelCoords_x, targetRectOrigin_x);
            __m256 dYs = _mm256_sub_ps(screenPixelCoords_y, targetRectOrigin_y);
            __m256 Us = _mm256_add_ps(_mm256_mul_ps(dXs, normalizedXAxis_x), _mm256_mul_ps(dYs, normalizedXAxis_y));
            __m256 Vs = _mm256_add_ps(_mm256_mul_ps(dXs, normalizedYAxis_x), _mm256_mul_ps(dYs, normalizedYAxis_y));

            //Using a mask to determine what colors final 8 wide pixel destintion buffer should except 
            //(background texels or image texels). This replaces the need for a conditional 
            __m256i writeMask = _mm256_castps_si256(_mm256_and_ps(_mm256_and_ps(_mm256_cmp_ps(Us, zero, _CMP_GE_OQ),
                                                                                _mm256_cmp_ps(Us, one, _CMP_LE_OQ)),
                                                                  _mm256_and_ps(_mm256_cmp_ps(Vs, zero, _CMP_GE_OQ),
                                                                                _mm256_cmp_ps(Vs, one, _CMP_LE_OQ))));

            //Clamp UVs to prevent accessing memory that is invalid 
            Us = _mm256_min_ps(_mm256_max_ps(Us, zero), one);
            Vs = _mm256_min_ps(_mm256_max_ps(Vs, zero), one);

            __m256 textureUs = _mm256_add_ps(minUVBounds_u, _mm256_mul_ps(uvRangeForTexture_u, Us));
            __m256 textureVs = _mm256_add_ps(minUVBounds_v, _mm256_mul_ps(uvRangeForTexture_v, Vs));

            __m256 texelCoords_x = _mm256_mul_ps(textureUs, imgWidth);
            __m256 texelCoords_y = _mm256_mul_ps(textureVs, imgHeight);

            __m256i sampleTexelAs{}, sampleTexelBs{}, sampleTexelCs{}, sampleTexelDs{};
            for(i32 index{}; index < 8; ++index)
            {
                BGZ_ASSERT((texelCoords_x.m256_f32[index] >= 0) && (texelCoords_x.m256_f32[index] <= (i32)image.size.width), "x coord is out of range!: ");
                BGZ_ASSERT((texelCoords_y.m256_f32[index] >= 0) && (texelCoords_y.m256_f32[index] <= (i32)image.size.height), "y coord is out of range!");

                //Gather 4 texels (in a square pattern) from certain texel Ptr
                ui8* texelPtr = ((ui8*)image.colorData) + ((ui32)texelCoords_y.m256_f32[index]*image.pitch) + ((ui32)texelCoords_x.m256_f32[index]*sizeof(ui32));//size of pixel
                sampleTexelAs.m256i_u32[index] = *(ui32*)(texelPtr);
                sampleTexelBs.m256i_u32[index] = *(ui32*)(texelPtr + sizeof(ui32));
                sampleTexelCs.m256i_u32[index] = *(ui32*)(texelPtr + image.pitch);
                sampleTexelDs.m256i_u32[index] = *(ui32*)(texelPtr + image.pitch + sizeof(ui32));
            };

#if __AVX2__
            //Unpack 4 sample texels to prepare for bilinear blend
            __m256i maskFF = _mm256_set1_epi32(0xFF);
            __m256 texelA_b = _mm256_cvtepi32_ps(_mm256_and_si256(sampleTexelAs, maskFF));
            __m256 texelA_g = _mm256_cvtepi32_ps(_mm256_and_si256(_mm256_srli_epi32(sampleTexelAs, 8), maskFF));
            __m256 texelA_r = _mm256_cvtepi32_ps(_mm256_and_si256(_mm256_srli_epi32(sampleTexelAs, 16), maskFF));
            __m256 texelA_a = _mm256_cvtepi32_ps(_mm256_and_si256(_mm256_srli_epi32(sampleTexelAs, 24), maskFF));
            __m256 texelB_b = _mm256_cvtepi32_ps(_mm256_and_si256(sampleTexelBs, maskFF));
            __m256 texelB_g = _mm256_cvtepi32_ps(_mm256_and_si256(_mm256_srli_epi32(sampleTexelBs, 8), maskFF));
            __m256 texelB_r = _mm256_cvtepi32_ps(_mm256_and_si256(_mm256_srli_epi32(sampleTexelBs, 16), maskFF));
            __m256 texelB_a = _mm256_cvtepi32_ps(_mm256_and_si256(_mm256_srli_epi32(sampleTexelBs, 24), maskFF));
            __m256 texelC_b = _mm256_cvtepi32_ps(_mm256_and_si256(sampleTexelCs, maskFF));
            __m256 texelC_g = _mm256_cvtepi32_ps(_mm256_and_si256(_mm256_srli_epi32(sampleTexelCs, 8), maskFF));
            __m256 texelC_r = _mm256_cvtepi32_ps(_mm256_and_si256(_mm256_srli_epi32(sampleTexelCs, 16), maskFF));
            __m256 texelC_a = _mm256_cvtepi32_ps(_mm256_and_si256(_mm256_srli_epi32(sampleTexelCs, 24), maskFF));
            __m256 texelD_b = _mm256_cvtepi32_ps(_mm256_and_si256(sampleTexelDs, maskFF));
            __m256 texelD_g = _mm256_cvtepi32_ps(_mm256_and_si256(_mm256_srli_epi32(sampleTexelDs, 8), maskFF));
            __m256 texelD_r = _mm256_cvtepi32_ps(_mm256_and_si256(_mm256_srli_epi32(sampleTexelDs, 16), maskFF));
            __m256 texelD_a = _mm256_cvtepi32_ps(_mm256_and_si256(_mm256_srli_epi32(sampleTexelDs, 24), maskFF));
            __m256i backGroundPixels = _mm256_load_si256((__m256i*)destPixel);
            __m256 backgroundColors_b = _mm256_cvtepi32_ps(_mm256_and_si256(backGroundPixels , maskFF)); 
            __m256 backgroundColors_g = _mm256_cvtepi32_ps(_mm256_and_si256(_mm256_srli_epi32(backGroundPixels, 8), maskFF));
            __m256 backgroundColors_r = _mm256_cvtepi32_ps(_mm256_and_si256(_mm256_srli_epi32(backGroundPixels, 16), maskFF));
            __m256 backgroundColors_a = _mm256_cvtepi32_ps(_mm256_and_si256(_mm256_srli_epi32(backGroundPixels, 24), maskFF));

#elif __AVX__
            //Unpack 4 sample texels to prepare for bilinear blend
            __m256i* ptrToSampleTexelAs = &sampleTexelAs;
            __m256 texelA_b = _mm256_set_ps((f32)*((ui8*)ptrToSampleTexelAs + 28), (f32)*((ui8*)ptrToSampleTexelAs + 24), (f32)*((ui8*)ptrToSampleTexelAs + 20), (f32)*((ui8*)ptrToSampleTexelAs + 16), (f32)*((ui8*)ptrToSampleTexelAs + 12), (f32)*((ui8*)ptrToSampleTexelAs + 8), (f32)*((ui8*)ptrToSampleTexelAs + 4), (f32)*((ui8*)ptrToSampleTexelAs + 0));
            __m256 texelA_g = _mm256_set_ps((f32)*((ui8*)ptrToSampleTexelAs + 29), (f32)*((ui8*)ptrToSampleTexelAs + 25), (f32)*((ui8*)ptrToSampleTexelAs + 21), (f32)*((ui8*)ptrToSampleTexelAs + 17), (f32)*((ui8*)ptrToSampleTexelAs + 13), (f32)*((ui8*)ptrToSampleTexelAs + 9), (f32)*((ui8*)ptrToSampleTexelAs + 5), (f32)*((ui8*)ptrToSampleTexelAs + 1));
            __m256 texelA_r = _mm256_set_ps((f32)*((ui8*)ptrToSampleTexelAs + 30), (f32)*((ui8*)ptrToSampleTexelAs + 26), (f32)*((ui8*)ptrToSampleTexelAs + 22), (f32)*((ui8*)ptrToSampleTexelAs + 18), (f32)*((ui8*)ptrToSampleTexelAs + 14), (f32)*((ui8*)ptrToSampleTexelAs + 10), (f32)*((ui8*)ptrToSampleTexelAs + 6), (f32)*((ui8*)ptrToSampleTexelAs + 2));
            __m256 texelA_a = _mm256_set_ps((f32)*((ui8*)ptrToSampleTexelAs + 31), (f32)*((ui8*)ptrToSampleTexelAs + 27), (f32)*((ui8*)ptrToSampleTexelAs + 23), (f32)*((ui8*)ptrToSampleTexelAs + 19), (f32)*((ui8*)ptrToSampleTexelAs + 15), (f32)*((ui8*)ptrToSampleTexelAs + 11), (f32)*((ui8*)ptrToSampleTexelAs + 7), (f32)*((ui8*)ptrToSampleTexelAs + 3));

            __m256i* ptrToSampleTexelBs = &sampleTexelBs;
            __m256 texelB_b = _mm256_set_ps((f32)*((ui8*)ptrToSampleTexelBs + 28), (f32)*((ui8*)ptrToSampleTexelBs + 24), (f32)*((ui8*)ptrToSampleTexelBs + 20), (f32)*((ui8*)ptrToSampleTexelBs + 16), (f32)*((ui8*)ptrToSampleTexelBs + 12), (f32)*((ui8*)ptrToSampleTexelBs + 8), (f32)*((ui8*)ptrToSampleTexelBs + 4), (f32)*((ui8*)ptrToSampleTexelBs + 0));
            __m256 texelB_g = _mm256_set_ps((f32)*((ui8*)ptrToSampleTexelBs + 29), (f32)*((ui8*)ptrToSampleTexelBs + 25), (f32)*((ui8*)ptrToSampleTexelBs + 21), (f32)*((ui8*)ptrToSampleTexelBs + 17), (f32)*((ui8*)ptrToSampleTexelBs + 13), (f32)*((ui8*)ptrToSampleTexelBs + 9), (f32)*((ui8*)ptrToSampleTexelBs + 5), (f32)*((ui8*)ptrToSampleTexelBs + 1));
            __m256 texelB_r = _mm256_set_ps((f32)*((ui8*)ptrToSampleTexelBs + 30), (f32)*((ui8*)ptrToSampleTexelBs + 26), (f32)*((ui8*)ptrToSampleTexelBs + 22), (f32)*((ui8*)ptrToSampleTexelBs + 18), (f32)*((ui8*)ptrToSampleTexelBs + 14), (f32)*((ui8*)ptrToSampleTexelBs + 10), (f32)*((ui8*)ptrToSampleTexelBs + 6), (f32)*((ui8*)ptrToSampleTexelBs + 2));
            __m256 texelB_a = _mm256_set_ps((f32)*((ui8*)ptrToSampleTexelBs + 31), (f32)*((ui8*)ptrToSampleTexelBs + 27), (f32)*((ui8*)ptrToSampleTexelBs + 23), (f32)*((ui8*)ptrToSampleTexelBs + 19), (f32)*((ui8*)ptrToSampleTexelBs + 15), (f32)*((ui8*)ptrToSampleTexelBs + 11), (f32)*((ui8*)ptrToSampleTexelBs + 7), (f32)*((ui8*)ptrToSampleTexelBs + 3));

            __m256i* ptrToSampleTexelCs = &sampleTexelCs;
            __m256 texelC_b = _mm256_set_ps((f32)*((ui8*)ptrToSampleTexelCs + 28), (f32)*((ui8*)ptrToSampleTexelCs + 24), (f32)*((ui8*)ptrToSampleTexelCs + 20), (f32)*((ui8*)ptrToSampleTexelCs + 16), (f32)*((ui8*)ptrToSampleTexelCs + 12), (f32)*((ui8*)ptrToSampleTexelCs + 8), (f32)*((ui8*)ptrToSampleTexelCs + 4), (f32)*((ui8*)ptrToSampleTexelCs + 0));
            __m256 texelC_g = _mm256_set_ps((f32)*((ui8*)ptrToSampleTexelCs + 29), (f32)*((ui8*)ptrToSampleTexelCs + 25), (f32)*((ui8*)ptrToSampleTexelCs + 21), (f32)*((ui8*)ptrToSampleTexelCs + 17), (f32)*((ui8*)ptrToSampleTexelCs + 13), (f32)*((ui8*)ptrToSampleTexelCs + 9), (f32)*((ui8*)ptrToSampleTexelCs + 5), (f32)*((ui8*)ptrToSampleTexelCs + 1));
            __m256 texelC_r = _mm256_set_ps((f32)*((ui8*)ptrToSampleTexelCs + 30), (f32)*((ui8*)ptrToSampleTexelCs + 26), (f32)*((ui8*)ptrToSampleTexelCs + 22), (f32)*((ui8*)ptrToSampleTexelCs + 18), (f32)*((ui8*)ptrToSampleTexelCs + 14), (f32)*((ui8*)ptrToSampleTexelCs + 10), (f32)*((ui8*)ptrToSampleTexelCs + 6), (f32)*((ui8*)ptrToSampleTexelCs + 2));
            __m256 texelC_a = _mm256_set_ps((f32)*((ui8*)ptrToSampleTexelCs + 31), (f32)*((ui8*)ptrToSampleTexelCs + 27), (f32)*((ui8*)ptrToSampleTexelCs + 23), (f32)*((ui8*)ptrToSampleTexelCs + 19), (f32)*((ui8*)ptrToSampleTexelCs + 15), (f32)*((ui8*)ptrToSampleTexelCs + 11), (f32)*((ui8*)ptrToSampleTexelCs + 7), (f32)*((ui8*)ptrToSampleTexelCs + 3));

            __m256i* ptrToSampleTexelDs = &sampleTexelDs;
            __m256 texelD_b = _mm256_set_ps((f32)*((ui8*)ptrToSampleTexelDs + 28), (f32)*((ui8*)ptrToSampleTexelDs + 24), (f32)*((ui8*)ptrToSampleTexelDs + 20), (f32)*((ui8*)ptrToSampleTexelDs + 16), (f32)*((ui8*)ptrToSampleTexelDs + 12), (f32)*((ui8*)ptrToSampleTexelDs + 8), (f32)*((ui8*)ptrToSampleTexelDs + 4), (f32)*((ui8*)ptrToSampleTexelDs + 0));
            __m256 texelD_g = _mm256_set_ps((f32)*((ui8*)ptrToSampleTexelDs + 29), (f32)*((ui8*)ptrToSampleTexelDs + 25), (f32)*((ui8*)ptrToSampleTexelDs + 21), (f32)*((ui8*)ptrToSampleTexelDs + 17), (f32)*((ui8*)ptrToSampleTexelDs + 13), (f32)*((ui8*)ptrToSampleTexelDs + 9), (f32)*((ui8*)ptrToSampleTexelDs + 5), (f32)*((ui8*)ptrToSampleTexelDs + 1));
            __m256 texelD_r = _mm256_set_ps((f32)*((ui8*)ptrToSampleTexelDs + 30), (f32)*((ui8*)ptrToSampleTexelDs + 26), (f32)*((ui8*)ptrToSampleTexelDs + 22), (f32)*((ui8*)ptrToSampleTexelDs + 18), (f32)*((ui8*)ptrToSampleTexelDs + 14), (f32)*((ui8*)ptrToSampleTexelDs + 10), (f32)*((ui8*)ptrToSampleTexelDs + 6), (f32)*((ui8*)ptrToSampleTexelDs + 2));
            __m256 texelD_a = _mm256_set_ps((f32)*((ui8*)ptrToSampleTexelDs + 31), (f32)*((ui8*)ptrToSampleTexelDs + 27), (f32)*((ui8*)ptrToSampleTexelDs + 23), (f32)*((ui8*)ptrToSampleTexelDs + 19), (f32)*((ui8*)ptrToSampleTexelDs + 15), (f32)*((ui8*)ptrToSampleTexelDs + 11), (f32)*((ui8*)ptrToSampleTexelDs + 7), (f32)*((ui8*)ptrToSampleTexelDs + 3));

            __m256i backGroundPixels = _mm256_load_si256((__m256i*)destPixel);
            __m256i* ptrToBackgroundPixels = &backGroundPixels; 
            __m256 backgroundColors_b = _mm256_set_ps((f32)*((ui8*)ptrToBackgroundPixels + 28), (f32)*((ui8*)ptrToBackgroundPixels + 24), (f32)*((ui8*)ptrToBackgroundPixels + 20), (f32)*((ui8*)ptrToBackgroundPixels + 16), (f32)*((ui8*)ptrToBackgroundPixels + 12), (f32)*((ui8*)ptrToBackgroundPixels + 8), (f32)*((ui8*)ptrToBackgroundPixels + 4), (f32)*((ui8*)ptrToBackgroundPixels + 0));
            __m256 backgroundColors_g = _mm256_set_ps((f32)*((ui8*)ptrToBackgroundPixels + 29), (f32)*((ui8*)ptrToBackgroundPixels + 25), (f32)*((ui8*)ptrToBackgroundPixels + 21), (f32)*((ui8*)ptrToBackgroundPixels + 17), (f32)*((ui8*)ptrToBackgroundPixels + 13), (f32)*((ui8*)ptrToBackgroundPixels + 9), (f32)*((ui8*)ptrToBackgroundPixels + 5), (f32)*((ui8*)ptrToBackgroundPixels + 1));
            __m256 backgroundColors_r = _mm256_set_ps((f32)*((ui8*)ptrToBackgroundPixels + 30), (f32)*((ui8*)ptrToBackgroundPixels + 26), (f32)*((ui8*)ptrToBackgroundPixels + 22), (f32)*((ui8*)ptrToBackgroundPixels + 18), (f32)*((ui8*)ptrToBackgroundPixels + 14), (f32)*((ui8*)ptrToBackgroundPixels + 10), (f32)*((ui8*)ptrToBackgroundPixels + 6), (f32)*((ui8*)ptrToBackgroundPixels + 2));
            __m256 backgroundColors_a = _mm256_set_ps((f32)*((ui8*)ptrToBackgroundPixels + 31), (f32)*((ui8*)ptrToBackgroundPixels + 27), (f32)*((ui8*)ptrToBackgroundPixels + 23), (f32)*((ui8*)ptrToBackgroundPixels + 19), (f32)*((ui8*)ptrToBackgroundPixels + 15), (f32)*((ui8*)ptrToBackgroundPixels + 11), (f32)*((ui8*)ptrToBackgroundPixels + 7), (f32)*((ui8*)ptrToBackgroundPixels + 3));
#endif

            //Bilinear blend 
            __m256 percentToLerpInX = _mm256_sub_ps(texelCoords_x, _mm256_floor_ps(texelCoords_x));
            __m256 percentToLerpInY = _mm256_sub_ps(texelCoords_y, _mm256_floor_ps(texelCoords_y));
            __m256 oneMinusXLerp = _mm256_sub_ps(one, percentToLerpInX);
            __m256 oneMinusYLerp = _mm256_sub_ps(one, percentToLerpInY);
            __m256 coefficient1 = _mm256_mul_ps(oneMinusYLerp, oneMinusXLerp);
            __m256 coefficient2 = _mm256_mul_ps(oneMinusYLerp, percentToLerpInX);
            __m256 coefficient3 = _mm256_mul_ps(percentToLerpInY, oneMinusXLerp);
            __m256 coefficient4 = _mm256_mul_ps(percentToLerpInY, percentToLerpInX);
            __m256 newBlendedTexel_r = _mm256_add_ps(
                                       _mm256_add_ps(_mm256_mul_ps(coefficient1, texelA_r), _mm256_mul_ps(coefficient2, texelB_r)), 
                                    _mm256_add_ps(_mm256_mul_ps(coefficient3, texelC_r), _mm256_mul_ps(coefficient4, texelD_r))); 
            __m256 newBlendedTexel_g = _mm256_add_ps(
                                       _mm256_add_ps(_mm256_mul_ps(coefficient1, texelA_g), _mm256_mul_ps(coefficient2, texelB_g)), 
                                       _mm256_add_ps(_mm256_mul_ps(coefficient3, texelC_g), _mm256_mul_ps(coefficient4, texelD_g))); 
            __m256 newBlendedTexel_b = _mm256_add_ps(
                                       _mm256_add_ps(_mm256_mul_ps(coefficient1, texelA_b), _mm256_mul_ps(coefficient2, texelB_b)), 
                                       _mm256_add_ps(_mm256_mul_ps(coefficient3, texelC_b), _mm256_mul_ps(coefficient4, texelD_b))); 
            __m256 newBlendedTexel_a = _mm256_add_ps(
                                       _mm256_add_ps(_mm256_mul_ps(coefficient1, texelA_a), _mm256_mul_ps(coefficient2, texelB_a)), 
                                       _mm256_add_ps(_mm256_mul_ps(coefficient3, texelC_a), _mm256_mul_ps(coefficient4, texelD_a))); 

            //Linear blend (w/ pre multiplied alpha)
            __m256 maxColorValue = _mm256_set1_ps(255.0f);
            __m256 alphaBlend = _mm256_div_ps(newBlendedTexel_a, maxColorValue);
            __m256 oneMinusAlphaBlend = _mm256_sub_ps(one, alphaBlend);
            __m256 finalBlendedColor_r = _mm256_add_ps(_mm256_mul_ps(oneMinusAlphaBlend, backgroundColors_r), newBlendedTexel_r);
            __m256 finalBlendedColor_g = _mm256_add_ps(_mm256_mul_ps(oneMinusAlphaBlend, backgroundColors_g), newBlendedTexel_g);
            __m256 finalBlendedColor_b = _mm256_add_ps(_mm256_mul_ps(oneMinusAlphaBlend, backgroundColors_b), newBlendedTexel_b);
            __m256 finalBlendedColor_a = _mm256_add_ps(_mm256_mul_ps(oneMinusAlphaBlend, backgroundColors_a), newBlendedTexel_a);

#if __AVX2__
            {//Convert and Pack into dest pixels to write out
                __m256i finalBlendedColori_r = _mm256_cvtps_epi32(finalBlendedColor_r);
                __m256i finalBlendedColori_g = _mm256_cvtps_epi32(finalBlendedColor_g);
                __m256i finalBlendedColori_b = _mm256_cvtps_epi32(finalBlendedColor_b);
                __m256i finalBlendedColori_a = _mm256_cvtps_epi32(finalBlendedColor_a);

                //Move pixels (through bitwise operations and shifting) from RRRR GGGG etc. format to expected BGRA format
                __m256i out = _mm256_or_si256(_mm256_or_si256(_mm256_or_si256(_mm256_slli_epi32(finalBlendedColori_r, 16), _mm256_slli_epi32(finalBlendedColori_g, 8)), finalBlendedColori_b), _mm256_slli_epi32(finalBlendedColori_a, 24));

                //Use write mask in order to correctly fill 8 wide pixel lane (properly writing either the texel color or 
                //the background color)
                __m256i maskedOut = _mm256_or_si256(_mm256_and_si256(writeMask, out),
                                                _mm256_andnot_si256(writeMask, backGroundPixels));

                *(__m256i*)destPixel = maskedOut;
            };
#endif

#if __AVX__
            {//Convert and Pack into dest pixels to write out
                __m256i finalBlendedColori_r = _mm256_cvtps_epi32(finalBlendedColor_r);
                __m256i finalBlendedColori_g = _mm256_cvtps_epi32(finalBlendedColor_g);
                __m256i finalBlendedColori_b = _mm256_cvtps_epi32(finalBlendedColor_b);
                __m256i finalBlendedColori_a = _mm256_cvtps_epi32(finalBlendedColor_a);

                __m256i backgroundColorsi_r = _mm256_cvtps_epi32(backgroundColors_r);
                __m256i backgroundColorsi_g = _mm256_cvtps_epi32(backgroundColors_g);
                __m256i backgroundColorsi_b = _mm256_cvtps_epi32(backgroundColors_b);
                __m256i backgroundColorsi_a = _mm256_cvtps_epi32(backgroundColors_a);

                //Since AVX doesn't have certain bitwise operations I need to extract 128 bit values from
                //256 bit ones and then use the available bitwise operations on those 
                __m128i pixelSet1_r = _mm256_extractf128_si256(finalBlendedColori_r, 0);
                __m128i pixelSet2_r = _mm256_extractf128_si256(finalBlendedColori_r, 1);
                __m128i pixelSet1_g = _mm256_extractf128_si256(finalBlendedColori_g, 0);
                __m128i pixelSet2_g = _mm256_extractf128_si256(finalBlendedColori_g, 1);
                __m128i pixelSet1_b = _mm256_extractf128_si256(finalBlendedColori_b, 0);
                __m128i pixelSet2_b = _mm256_extractf128_si256(finalBlendedColori_b, 1);
                __m128i pixelSet1_a = _mm256_extractf128_si256(finalBlendedColori_a, 0);
                __m128i pixelSet2_a = _mm256_extractf128_si256(finalBlendedColori_a, 1);
                __m128i backgroundPixelSet1_r = _mm256_extractf128_si256(backgroundColorsi_r, 0);
                __m128i backgroundPixelSet2_r = _mm256_extractf128_si256(backgroundColorsi_r, 1);
                __m128i backgroundPixelSet1_g = _mm256_extractf128_si256(backgroundColorsi_g, 0);
                __m128i backgroundPixelSet2_g = _mm256_extractf128_si256(backgroundColorsi_g, 1);
                __m128i backgroundPixelSet1_b = _mm256_extractf128_si256(backgroundColorsi_b, 0);
                __m128i backgroundPixelSet2_b = _mm256_extractf128_si256(backgroundColorsi_b, 1);
                __m128i backgroundPixelSet1_a = _mm256_extractf128_si256(backgroundColorsi_a, 0);
                __m128i backgroundPixelSet2_a = _mm256_extractf128_si256(backgroundColorsi_a, 1);
                __m128i writeMaskSet1 = _mm256_extractf128_si256(writeMask, 0);
                __m128i writeMaskSet2 = _mm256_extractf128_si256(writeMask, 1);

                //Move pixels (through bitwise operations and shifting) from RRRR GGGG ... format to expected BGRA format
                __m128i pixels1Through4 = _mm_or_si128(_mm_or_si128(_mm_or_si128(_mm_slli_epi32(pixelSet1_r, 16), _mm_slli_epi32(pixelSet1_g, 8)), pixelSet1_b), _mm_slli_epi32(pixelSet1_a, 24));
                __m128i pixels5Through8 = _mm_or_si128(_mm_or_si128(_mm_or_si128(_mm_slli_epi32(pixelSet2_r, 16), _mm_slli_epi32(pixelSet2_g, 8)), pixelSet2_b), _mm_slli_epi32(pixelSet2_a, 24));
                __m128i backgroundPixels1Through4 = _mm_or_si128(_mm_or_si128(_mm_or_si128(_mm_slli_epi32(backgroundPixelSet1_r, 16), _mm_slli_epi32(backgroundPixelSet1_g, 8)), backgroundPixelSet1_b), _mm_slli_epi32(backgroundPixelSet1_a, 24));
                __m128i backgroundPixels5Through8 = _mm_or_si128(_mm_or_si128(_mm_or_si128(_mm_slli_epi32(backgroundPixelSet2_r, 16), _mm_slli_epi32(backgroundPixelSet2_g, 8)), backgroundPixelSet2_b), _mm_slli_epi32(backgroundPixelSet2_a, 24));

                //Use write mask in order to correctly fill 8 wide pixel lane (properly writing either the texel color or 
                //the background color)
                __m128i maskedOutSet1 = _mm_or_si128(_mm_and_si128(writeMaskSet1, pixels1Through4),
                                                    _mm_andnot_si128(writeMaskSet1, backgroundPixels1Through4));
                __m128i maskedOutSet2 = _mm_or_si128(_mm_and_si128(writeMaskSet2, pixels5Through8),
                                                    _mm_andnot_si128(writeMaskSet2, backgroundPixels5Through8));

                //Pack 128 bit pixel values back into 256 bit values to write out
                __m256i maskedOut = _mm256_castsi128_si256(maskedOutSet1);
                maskedOut = _mm256_insertf128_si256(maskedOut, maskedOutSet2, 1);

                *(__m256i*)destPixel = maskedOut;
            };

#endif

            destPixel += 8;
        };


        
        currentRow += colorBufferPitch;
    };

};

//For images that move/rotate/scale - Assumes pre-multiplied alpha
//Also involves lighting calcuation
//TODO: Will eventually be removed entirely/combine with DrawTextureQuickly
local_func void
DrawTextureSlowly(ui32* colorBufferData, v2i colorBufferSize, i32 colorBufferPitch, Quadf cameraCoords, RenderEntry_Texture image, f32 rotation, v2f scale, Image normalMap = {}, f32 lightAngle = {}, f32 lightThreshold = {})
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
        v4f texelA = UnPackPixelValues(pixelsToLerp.x, BGRA);
        v4f texelB = UnPackPixelValues(pixelsToLerp.y, BGRA);
        v4f texelC = UnPackPixelValues(pixelsToLerp.z, BGRA);
        v4f texelD = UnPackPixelValues(pixelsToLerp.w, BGRA);

        v4f ABLerpColor = Lerp(texelA, texelB, percentToLerpInX);
        v4f CDLerpColor = Lerp(texelC, texelD, percentToLerpInX);
        newBlendedValue = Lerp(ABLerpColor, CDLerpColor, percentToLerpInY);

        return newBlendedValue;
    };

    v2f origin = cameraCoords.bottomLeft;
    v2f targetRectXAxis = cameraCoords.bottomRight - origin;
    v2f targetRectYAxis = cameraCoords.topLeft - origin;

    f32 widthMax = (f32)(colorBufferSize.width - 1);
    f32 heightMax = (f32)(colorBufferSize.height - 1);
    
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

    v2f uvRangeForTexture = {image.uvBounds.At(1).u - image.uvBounds.At(0).u, image.uvBounds.At(1).v - image.uvBounds.At(0).v};
    f32 invertedXAxisSqd = 1.0f / MagnitudeSqd(targetRectXAxis);
    f32 invertedYAxisSqd = 1.0f / MagnitudeSqd(targetRectYAxis);

    ui8* currentRow = (ui8*)colorBufferData + (i32)xMin * 4+ (i32)yMin * colorBufferPitch; 
    for(f32 screenY = yMin; screenY < yMax; ++screenY)
    {
        ui32* destPixel = (ui32*)currentRow;

        for(f32 screenX = xMin; screenX < xMax; ++screenX)
        {            
            v2f screenPixelCoord{screenX, screenY};
            v2f d {screenPixelCoord - origin};

            f32 u = invertedXAxisSqd * DotProduct(d, targetRectXAxis);
            f32 v = invertedYAxisSqd * DotProduct(d, targetRectYAxis);

            //Used to decide, given what screen pixel were on, whether or not that screen pixel falls
            //within the target rect's bounds or not
            if(u >= 0.0f && u <= 1.0f && v >= 0.0f && v <= 1.0f)
            {
                f32 textureU = image.uvBounds.At(0).u + (uvRangeForTexture.u * u);
                f32 textureV = image.uvBounds.At(0).y + (uvRangeForTexture.v * v);

                f32 texelPos_x = (textureU*(f32)(image.size.width));
                f32 texelPos_y = (textureV*(f32)(image.size.height)); 

                f32 epsilon = 0.00001f;//TODO: Remove????
                BGZ_ASSERT(((u + epsilon) >= 0.0f) && ((u - epsilon) <= 1.0f), "u is out of range! %f", u);
                BGZ_ASSERT(((v + epsilon) >= 0.0f) && ((v - epsilon) <= 1.0f), "v is out of range! %f", v);
                BGZ_ASSERT((texelPos_x >= 0) && (texelPos_x <= (i32)image.size.width), "x coord is out of range!: %f", texelPos_x);
                BGZ_ASSERT((texelPos_y >= 0) && (texelPos_y <= (i32)image.size.height), "x coord is out of range!: %f", texelPos_y);

                    ui8* texelPtr = ((ui8*)image.colorData) + ((ui32)texelPos_y*image.pitch) + ((ui32)texelPos_x*sizeof(ui32));//size of pixel

                    v4ui32 texelSquare {}; 
                    texelSquare.x = *(ui32*)(texelPtr);
                    texelSquare.y = *(ui32*)(texelPtr + sizeof(ui32));
                    texelSquare.z = *(ui32*)(texelPtr + image.pitch);
                    texelSquare.w = *(ui32*)(texelPtr + image.pitch + sizeof(ui32));

                    //Blend between all 4 pixels to produce new color for sub pixel accruacy - Bilinear filtering
                    v4f newBlendedTexel = BiLinearLerp(texelSquare, (texelPos_x - Floor(texelPos_x)), (texelPos_y - Floor(texelPos_y)));

                    //Linearly Blend with background - Assuming Pre-multiplied alpha
                    v4f backgroundColors = UnPackPixelValues(*destPixel, BGRA);
                    f32 alphaBlend = newBlendedTexel.a / 255.0f;
                    v4f finalBlendedColor = (1.0f - alphaBlend)*backgroundColors + newBlendedTexel;

                    b shadePixel{false};
                    if(normalMap.data)
                    {
                        ui8* normalPtr = ((ui8*)normalMap.data) + ((ui32)texelPos_y*image.pitch) + ((ui32)texelPos_x*sizeof(ui32));//size of pixel

                        //Grab 4 normals (in a square pattern) to blend
                        v4ui32 normalSquare = Grab4NearestPixelPtrs_SquarePattern(normalPtr, normalMap.pitch);

                        v4f blendedNormal = BiLinearLerp(normalSquare, (texelPos_x - Floor(texelPos_x)), (texelPos_y - Floor(texelPos_y)));

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
        
        currentRow += colorBufferPitch;
    }
}

void RenderToImage(Image&& renderTarget, Image sourceImage, Quadf targetArea)
{
    //DrawImageSlowly($(renderTarget), targetArea, sourceImage, 0.0f);
};

struct Screen_Region_Render_Work
{
    Rendering_Info renderingInfo;
    void* colorBufferData;
    v2i colorBufferSize;
    i32 colorBufferPitch;
    Rectf screenRegionCoords;
};

//Multi-threaded
PLATFORM_WORK_QUEUE_CALLBACK(DrawScreenRegion)
{
    Screen_Region_Render_Work* work = (Screen_Region_Render_Work*)data;

    ui8* currentRenderBufferEntry = work->renderingInfo.cmdBuffer.baseAddress;
    Camera2D* camera = &work->renderingInfo.camera;

                for(i32 entryNumber = 0; entryNumber < work->renderingInfo.cmdBuffer.entryCount; ++entryNumber)
                {
                    RenderEntry_Header* entryHeader = (RenderEntry_Header*)currentRenderBufferEntry;
                    switch(entryHeader->type)
                    {
                        case EntryType_Texture:
                        {
                            RenderEntry_Texture textureEntry = *(RenderEntry_Texture*)currentRenderBufferEntry;

                            ConvertToCorrectPositiveRadian($(textureEntry.world.rotation));

                            Quadf imageTargetRect = _ProduceQuadFromCenterPoint(textureEntry.world.pos, (f32)textureEntry.targetRectSize.width, (f32)textureEntry.targetRectSize.height);

                            Quadf imageTargetRect_world = WorldTransform_CenterPoint(imageTargetRect, textureEntry.world);
                            Quadf imageTargetRect_camera = CameraTransform(imageTargetRect_world, *camera);

                            DrawTextureQuickly((ui32*)work->colorBufferData, work->colorBufferSize, work->colorBufferPitch, imageTargetRect_camera, textureEntry, textureEntry.world.rotation, textureEntry.world.scale, work->screenRegionCoords);

                            currentRenderBufferEntry += sizeof(RenderEntry_Texture);
                        }break;

                        case EntryType_Rect:
                        {
                            RenderEntry_Rect rectEntry = *(RenderEntry_Rect*)currentRenderBufferEntry;

                            ConvertToCorrectPositiveRadian($(rectEntry.world.rotation));

                            Quadf targetQuad = _ProduceQuadFromCenterPoint(rectEntry.world.pos, rectEntry.dimensions.x, rectEntry.dimensions.y);

                            Quadf targetQuad_world = WorldTransform_CenterPoint(targetQuad, rectEntry.world);
                            Quadf targetQuad_camera = CameraTransform(targetQuad_world, *camera);

                            DrawRectangle((ui32*)work->colorBufferData, work->colorBufferSize, work->colorBufferPitch, targetQuad_camera, rectEntry.dimensions, rectEntry.color, work->screenRegionCoords);
                            currentRenderBufferEntry += sizeof(RenderEntry_Rect);
                        }break;

                        InvalidDefaultCase;
                    };
                };
};

//Single Threaded
void DoRenderWork(void* data)
{
    Screen_Region_Render_Work* work = (Screen_Region_Render_Work*)data;

    ui8* currentRenderBufferEntry = work->renderingInfo.cmdBuffer.baseAddress;
    Camera2D* camera = &work->renderingInfo.camera;

                for(i32 entryNumber = 0; entryNumber < work->renderingInfo.cmdBuffer.entryCount; ++entryNumber)
                {
                    RenderEntry_Header* entryHeader = (RenderEntry_Header*)currentRenderBufferEntry;
                    switch(entryHeader->type)
                    {
                        case EntryType_Texture:
                        {
                            RenderEntry_Texture textureEntry = *(RenderEntry_Texture*)currentRenderBufferEntry;

                            ConvertToCorrectPositiveRadian($(textureEntry.world.rotation));

                            Quadf imageTargetRect = _ProduceQuadFromCenterPoint(textureEntry.world.pos, (f32)textureEntry.targetRectSize.width, (f32)textureEntry.targetRectSize.height);

                            Quadf imageTargetRect_world = WorldTransform_CenterPoint(imageTargetRect, textureEntry.world);
                            Quadf imageTargetRect_camera = CameraTransform(imageTargetRect_world, *camera);

                            DrawTextureQuickly((ui32*)work->colorBufferData, work->colorBufferSize, work->colorBufferPitch, imageTargetRect_camera, textureEntry, textureEntry.world.rotation, textureEntry.world.scale, work->screenRegionCoords);

                            currentRenderBufferEntry += sizeof(RenderEntry_Texture);
                        }break;

                        case EntryType_Rect:
                        {
                            RenderEntry_Rect rectEntry = *(RenderEntry_Rect*)currentRenderBufferEntry;

                            ConvertToCorrectPositiveRadian($(rectEntry.world.rotation));

                            Quadf targetQuad = _ProduceQuadFromCenterPoint(rectEntry.world.pos, rectEntry.dimensions.x, rectEntry.dimensions.y);

                            Quadf targetQuad_world = WorldTransform_CenterPoint(targetQuad, rectEntry.world);
                            Quadf targetQuad_camera = CameraTransform(targetQuad_world, *camera);

                            DrawRectangle((ui32*)work->colorBufferData, work->colorBufferSize, work->colorBufferPitch, targetQuad_camera, rectEntry.dimensions, rectEntry.color, work->screenRegionCoords);
                            currentRenderBufferEntry += sizeof(RenderEntry_Rect);
                        }break;

                        InvalidDefaultCase;
                    };
                };
};

struct Platform_Services;
void RenderViaSoftware(Rendering_Info&& renderingInfo, void* colorBufferData, v2i colorBufferSize, i32 colorBufferPitch, Platform_Services* platformServices)
{
    f32 const screenRegionCount_x = 8.0f;
    f32 const screenRegionCount_y = 8.0f;
    i32 workIndex{};
    Array<Screen_Region_Render_Work, (i64)(screenRegionCount_x*screenRegionCount_y)> workArray{};

    f32 singleScreenRegion_width = colorBufferSize.width / screenRegionCount_x;
    f32 singleScreenRegion_height = colorBufferSize.height / screenRegionCount_y;

    for(ui32 screenRegion_y{}; screenRegion_y < screenRegionCount_y; ++screenRegion_y)
    {
        for(ui32 screenRegion_x{}; screenRegion_x < screenRegionCount_x; ++screenRegion_x)
        {
            v2f screenRegion_min = v2f{screenRegion_x * singleScreenRegion_width, screenRegion_y * singleScreenRegion_height};
            v2f screenRegion_max = v2f{screenRegion_min.x + singleScreenRegion_width, screenRegion_min.y + singleScreenRegion_height};
            Rectf screenRegionCoords{screenRegion_min, screenRegion_max};

            Screen_Region_Render_Work* renderWork = &workArray.At(workIndex);

            renderWork->renderingInfo = renderingInfo;
            renderWork->colorBufferData = colorBufferData;
            renderWork->colorBufferSize = colorBufferSize;
            renderWork->colorBufferPitch = colorBufferPitch;
            renderWork->screenRegionCoords = screenRegionCoords;

#if 1 //Multi-Threaded
            platformServices->AddWorkQueueEntry(DrawScreenRegion, renderWork);
#else //Single Threaded
            Screen_Region_Render_Work* work = &workArray.At(workIndex);
            DoRenderWork(work);
#endif

            workIndex++;
        };
    };

    platformServices->FinishAllWork();

    renderingInfo.cmdBuffer.entryCount = 0; 
};

#endif //SOFTWARE_RENDERING_INCLUDE_H


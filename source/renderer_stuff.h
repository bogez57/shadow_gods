#ifndef RENDERER_STUFF_INCLUDE
#define RENDERER_STUFF_INCLUDE

#ifndef BYTES_PER_PIXEL
#define BYTES_PER_PIXEL 4
#endif

#include "my_math.h"

/*

    Current Renderer assumptions:

    1.) User sends world coordinates to renderer. 4 verts pushed per texture/rect.
    2.) Renderer expects all verts to be in meters and not pixels.
    3.) Y axis is going up and bottom left corner of rect is expected to be origin

*/

//TODO: Separate out transform from pushtexture so that user pushes transform and textures separately
struct Camera2D
{
    v2 lookAt {};
    v2 viewCenter {};
    v2 dilatePoint_inScreenCoords {};
    v2 dilatePointOffset_normalized {};
    f32 zoomFactor {};
};

struct Camera3D
{
    f32 FOV{};
    v3 posOffset{};//From center of screen (in meters)
    v3 rotation{};
    f32 aspectRatio{};
    f32 nearPlane{};
    f32 farPlane{};
};

struct Rect
{
    v2 min {};
    v2 max {};
};

struct Quad
{
    union
    {
        Array<v2, 4> vertices;
        struct
        {
            v2 bottomLeft;
            v2 bottomRight;
            v2 topRight;
            v2 topLeft;
        };
    };
};

struct Cube
{
    Array<v3, 8> verts{};
    v3 color{};
};

struct Game_Render_Cmd_Buffer
{
    ui8* baseAddress { nullptr };
    i32 usedAmount {};
    i32 size {};
    i32 entryCount {};
};

struct Rendering_Info
{
    Game_Render_Cmd_Buffer cmdBuffer;
    Camera2D camera2d;
    Camera3D camera3d;
    f32 _pixelsPerMeter {};
};

struct NormalMap
{
    ui8* mapData { nullptr };
    f32 rotation {};
    v2 scale { 1.0f, 1.0f };
    f32 lightAngle {};
    f32 lightThreshold {};
};

struct Image
{
    ui8* data { nullptr };
    f32 aspectRatio {};
    i32 width_pxls {};
    i32 height_pxls {};
    i32 pitch_pxls {};
    f32 opacity { 1.0f };
    v2 scale { 1.0f, 1.0f };
    b isLoadedOnGPU { false }; //TODO: Eventually remove
};

struct Coordinate_Space
{
    v2 origin {};
    v2 xBasis {};
    v2 yBasis {};
};

struct Object_Transform
{
    f32 rotation {};
    v2 pos {};
    v2 scale {};
};

enum Render_Entry_Type
{
    EntryType_Line,
    EntryType_Rect,
    EntryType_Cube,
    EntryType_Texture,
    EntryType_Test
};

struct RenderEntry_Header
{
    Render_Entry_Type type;
};

struct RenderEntry_Rect
{
    RenderEntry_Header header;
    Quad worldCoords;
    v3 color {};
};

struct RenderEntry_Cube
{
    RenderEntry_Header header;
    Array<v3, 8> verts{};
    v3 color;
};

struct RenderEntry_Texture
{
    RenderEntry_Header header;
    const char* name;
    ui8* colorData { nullptr };
    NormalMap normalMap {};
    int width;
    int height;
    i32 pitch_pxls {};
    v2 dimensions {};
    Array<v2, 2> uvBounds;
    Quad targetRect_worldCoords;
    b isLoadedOnGPU { false }; //TODO: Eventually remove
};

struct RenderEntry_Line
{
    RenderEntry_Header header;
    v2 minPoint;
    v2 maxPoint;
    v3 color;
    f32 thickness;
};

struct RenderEntry_Test
{
    RenderEntry_Header header;
};

Image LoadBitmap_BGRA(const char* fileName);

//Helpers
f32 BitmapWidth_Meters(Image bitmap);
f32 BitmapHeight_Meters(Rendering_Info info, Image bitmap);
v2 viewPortDimensions_Meters(Rendering_Info&& renderingInfo);

//Render Commands
void PushTest(Rendering_Info&& renderingInfo);
void PushTexture(Rendering_Info&& renderingInfo, Quad worldVerts, Image bitmap, f32 objectHeight_inMeters, Array<v2, 2> uvs, const char* name);
void PushTexture(Rendering_Info&& renderingInfo, Quad worldVerts, Image bitmap, v2 objectSize_meters, Array<v2, 2> uvs, const char* name);
void PushRect(Rendering_Info* renderingInfo, Quad worldVerts, v3 color);
void PushLine(Rendering_Info* renderingInfo, v2 minPoint, v2 maxPoint, v3 color, f32 thickness);
void PushCube(Rendering_Info* renderingInfo, Array<v3, 8> cubeVerts, v3 color);
void PushCamera(Rendering_Info* renderingInfo, v2 lookAt, v2 dilatePoint_inScreenCoords, f32 zoomFactor);
void PushCamera3d(Rendering_Info* renderingInfo, f32 FOV, f32 aspectRatio, f32 nearPlane, f32 farPlane, v3 posOffset);
void UpdateCamera3d(Rendering_Info* renderingInfo, v3 amountToAddToCurrentPos, v3 amountToAddToCurrentRotation_radians);
void UpdateCamera(Rendering_Info* renderingInfo, v2 cameraLookAtCoords_meters, f32 zoomFactor);
void RenderViaSoftware(Rendering_Info&& renderBufferInfo, void* colorBufferData, v2 colorBufferSize, i32 colorBufferPitch);

void ConvertNegativeToPositiveAngle_Radians(f32&& angle);
void ConvertToCorrectPositiveRadian(f32&& angle);
//void RenderToImage(Image&& renderTarget, Image sourceImage, Quad targetArea);
Quad WorldTransform(Quad localCoords, Object_Transform transformInfo_world);
Quad CameraTransform(Quad worldCoords, Camera2D camera);
v2 CameraTransform(v2 worldCoords, Camera2D camera);
Quad ProjectionTransform_Ortho(Quad cameraCoords, f32 pixelsPerMeter);
v2 ProjectionTransform_Ortho(v2 cameraCoords, f32 pixelsPerMeter);
Rect _ProduceRectFromCenterPoint(v2 OriginPoint, f32 width, f32 height);
Rect _ProduceRectFromBottomMidPoint(v2 OriginPoint, f32 width, f32 height);
Rect _ProduceRectFromBottomLeftPoint(v2 originPoint, f32 width, f32 height);
Quad _ProduceQuadFromBottomLeftPoint(v2 originPoint, f32 width, f32 height);
Quad _ProduceQuadFromBottomMidPoint(v2 originPoint, f32 width, f32 height);
Quad ProduceQuadFromCenterPoint(v2 originPoint, f32 width, f32 height);

#endif //RENDERER_STUFF_INCLUDE_H

#ifdef GAME_RENDERER_STUFF_IMPL

void* _RenderCmdBuf_Push(Game_Render_Cmd_Buffer* commandBuf, i32 sizeOfCommand)
{
    void* memoryPointer = (void*)(commandBuf->baseAddress + commandBuf->usedAmount);
    commandBuf->usedAmount += (sizeOfCommand);
    return memoryPointer;
};
#define RenderCmdBuf_Push(commandBuffer, commandType) (commandType*)_RenderCmdBuf_Push(commandBuffer, sizeof(commandType))

void PushTest(Rendering_Info&& renderingInfo)
{
    RenderEntry_Test* testEntry = RenderCmdBuf_Push(&renderingInfo.cmdBuffer, RenderEntry_Test);
    
    testEntry->header.type = EntryType_Test;
    
    ++renderingInfo.cmdBuffer.entryCount;
};

void PushLine(Rendering_Info* renderingInfo, v2 minPoint, v2 maxPoint, v3 color, f32 thickness)
{
    RenderEntry_Line* lineEntry = RenderCmdBuf_Push(&renderingInfo->cmdBuffer, RenderEntry_Line);
    
    lineEntry->header.type = EntryType_Line;
    lineEntry->minPoint = minPoint;
    lineEntry->maxPoint = maxPoint;
    lineEntry->color = color;
    lineEntry->thickness = thickness;
    
    ++renderingInfo->cmdBuffer.entryCount;
};

void PushRect(Rendering_Info* renderingInfo, Quad worldVerts, v3 color)
{
    RenderEntry_Rect* rectEntry = RenderCmdBuf_Push(&renderingInfo->cmdBuffer, RenderEntry_Rect);
    
    rectEntry->header.type = EntryType_Rect;
    rectEntry->color = color;
    rectEntry->worldCoords = worldVerts;
    
    ++renderingInfo->cmdBuffer.entryCount;
};

void PushCube(Rendering_Info* renderingInfo, Array<v3, 8> cubeVerts, v3 color)
{
    RenderEntry_Cube* cubeEntry = RenderCmdBuf_Push(&renderingInfo->cmdBuffer, RenderEntry_Cube);
    
    cubeEntry->header.type = EntryType_Cube;
    CopyArray(cubeVerts, $(cubeEntry->verts));
    cubeEntry->color = color;
    
    ++renderingInfo->cmdBuffer.entryCount;
};

void PushTexture(Rendering_Info* renderingInfo, Quad worldVerts, Image&& bitmap, v2 objectSize_meters, Array<v2, 2> uvs, const char* name, NormalMap normalMap = {})
{
    RenderEntry_Texture* textureEntry = RenderCmdBuf_Push(&renderingInfo->cmdBuffer, RenderEntry_Texture);
    
    textureEntry->header.type = EntryType_Texture;
    textureEntry->name = name;
    textureEntry->targetRect_worldCoords = worldVerts;
    textureEntry->colorData = bitmap.data;
    textureEntry->normalMap = normalMap;
    textureEntry->width = (int)bitmap.width_pxls;
    textureEntry->height = (int)bitmap.height_pxls;
    textureEntry->pitch_pxls = bitmap.pitch_pxls;
    textureEntry->uvBounds = uvs;
    textureEntry->isLoadedOnGPU = bitmap.isLoadedOnGPU; //TODO: Remove
    bitmap.isLoadedOnGPU = true; //TODO: Remove
    
    textureEntry->dimensions = { objectSize_meters.width, objectSize_meters.height };
    
    ++renderingInfo->cmdBuffer.entryCount;
};

//TODO: Consider not having overloaded function here? The reason this is here is to support current
//skeleton drawing with regions
void PushTexture(Rendering_Info* renderingInfo, Quad worldVerts, Image&& bitmap, f32 objectHeight_meters, Array<v2, 2> uvs, const char* name, NormalMap normalMap = {})
{
    RenderEntry_Texture* textureEntry = RenderCmdBuf_Push(&renderingInfo->cmdBuffer, RenderEntry_Texture);
    
    f32 desiredWidth = bitmap.aspectRatio * objectHeight_meters;
    f32 desiredHeight = objectHeight_meters;
    
    textureEntry->header.type = EntryType_Texture;
    textureEntry->name = name;
    textureEntry->targetRect_worldCoords = worldVerts;
    textureEntry->colorData = bitmap.data;
    textureEntry->normalMap = normalMap;
    textureEntry->width = (int)bitmap.width_pxls;
    textureEntry->height = (int)bitmap.height_pxls;
    textureEntry->pitch_pxls = bitmap.pitch_pxls;
    textureEntry->uvBounds = uvs;
    textureEntry->isLoadedOnGPU = bitmap.isLoadedOnGPU; //TODO: Remove
    bitmap.isLoadedOnGPU = true; //TODO: Remove
    
    textureEntry->dimensions = { desiredWidth, desiredHeight };
    
    ++renderingInfo->cmdBuffer.entryCount;
};

void PushCamera3d(Rendering_Info* renderingInfo, f32 FOV, f32 aspectRatio, f32 nearPlane, f32 farPlane, v3 posOffset_fromCenterOfScreen)
{
    renderingInfo->camera3d.FOV = FOV;
    renderingInfo->camera3d.aspectRatio = aspectRatio;
    renderingInfo->camera3d.nearPlane = nearPlane;
    renderingInfo->camera3d.farPlane = farPlane;
    renderingInfo->camera3d.posOffset = posOffset_fromCenterOfScreen;
};

void UpdateCamera3d(Rendering_Info* renderingInfo, v3 amountOfTranslation, v3 amountOfRotation)
{
    renderingInfo->camera3d.posOffset += amountOfTranslation;
    renderingInfo->camera3d.rotation += amountOfRotation;
};

void UpdateCamera(Rendering_Info* renderingInfo, v2 cameraLookAtCoords_meters, f32 zoomFactor, v2 normalizedDilatePointOffset)
{
    BGZ_ASSERT(normalizedDilatePointOffset.x >= -1.0f && normalizedDilatePointOffset.x <= 1.0f
               && normalizedDilatePointOffset.y >= -1.0f && normalizedDilatePointOffset.y <= 1.0f,
               "Dilate point is not normalized!");
    
    renderingInfo->camera2d.lookAt = cameraLookAtCoords_meters;
    renderingInfo->camera2d.zoomFactor = zoomFactor;
    renderingInfo->camera2d.dilatePointOffset_normalized = normalizedDilatePointOffset;
};

void UpdateCamera(Rendering_Info* renderingInfo, v2 cameraLookAtCoords_meters, f32 zoomFactor)
{
    renderingInfo->camera2d.lookAt = cameraLookAtCoords_meters;
    renderingInfo->camera2d.zoomFactor = zoomFactor;
};

void UpdateCamera(Rendering_Info* renderingInfo, f32 zoomFactor)
{
    renderingInfo->camera2d.zoomFactor = zoomFactor;
};

Image LoadBitmap_BGRA(const char* fileName)
{
    Image result;
    
    { //Load image data using stb (w/ user defined read/seek functions and memory allocation functions)
        stbi_set_flip_vertically_on_load(true); //So first byte stbi_load() returns is bottom left instead of top-left of image (which is stb's default)
        
        i32 numOfLoadedChannels {};
        i32 desiredChannels { 4 }; //Since I still draw assuming 4 byte pixels I need 4 channels
        
        //Returns RGBA
        unsigned char* imageData = stbi_load(fileName, &result.width_pxls, &result.height_pxls, &numOfLoadedChannels, desiredChannels);
        BGZ_ASSERT(imageData, "Invalid image data!");
        
        i32 totalPixelCountOfImg = result.width_pxls * result.height_pxls;
        ui32* imagePixel = (ui32*)imageData;
        
        //Swap R and B channels of image
        for (int i = 0; i < totalPixelCountOfImg; ++i)
        {
            auto color = UnPackPixelValues(*imagePixel, RGBA);
            
            //Pre-multiplied alpha
            f32 alphaBlend = color.a / 255.0f;
            color.rgb *= alphaBlend;
            
            ui32 newSwappedPixelColor = (((ui8)color.a << 24) | ((ui8)color.r << 16) | ((ui8)color.g << 8) | ((ui8)color.b << 0));
            
            *imagePixel++ = newSwappedPixelColor;
        }
        
        result.data = (ui8*)imageData;
    };
    
    result.aspectRatio = (f32)result.width_pxls / (f32)result.height_pxls;
    result.pitch_pxls = (ui32)result.width_pxls * BYTES_PER_PIXEL;
    
    return result;
};

Quad ProduceQuadFromCenterPoint(v2 originPoint, f32 width, f32 height)
{
    Quad result;
    
    result.bottomLeft = { originPoint.x - (width / 2.0f), originPoint.y - (height / 2.0f) };
    result.bottomRight = { originPoint.x + (width / 2.0f), originPoint.y - (height / 2.0f) };
    result.topRight = { originPoint.x + (width / 2.0f), originPoint.y + (height / 2.0f) };
    result.topLeft = { originPoint.x - (width / 2.0f), originPoint.y + (height / 2.0f) };
    
    return result;
};

f32 BitmapHeight_Meters(Rendering_Info renderingInfo, Image bitmap)
{
    return bitmap.height_pxls / renderingInfo._pixelsPerMeter;
};

#endif //GAME_RENDERER_STUFF_IMPL

#ifdef PLATFORM_RENDERER_STUFF_IMPL

local_func
Rect
_DilateAboutArbitraryPoint(v2 PointOfDilation, f32 ScaleFactor, Rect RectToDilate)
{
    Rect DilatedRect {};
    
    v2 Distance = PointOfDilation - RectToDilate.min;
    Distance *= ScaleFactor;
    DilatedRect.min = PointOfDilation - Distance;
    
    Distance = PointOfDilation - RectToDilate.max;
    Distance *= ScaleFactor;
    DilatedRect.max = PointOfDilation - Distance;
    
    return DilatedRect;
};

v2 _DilateAboutArbitraryPoint(v2 PointOfDilation, f32 ScaleFactor, v2 vectorToDilate)
{
    v2 dilatedVector {};
    
    v2 distance = PointOfDilation - vectorToDilate;
    distance *= ScaleFactor;
    dilatedVector = PointOfDilation - distance;
    
    return dilatedVector;
};

auto _DilateAboutArbitraryPoint(v2 PointOfDilation, f32 ScaleFactor, Quad QuadToDilate) -> Quad
{
    Quad DilatedQuad {};
    
    for (i32 vertIndex = 0; vertIndex < 4; ++vertIndex)
    {
        v2 Distance = PointOfDilation - QuadToDilate.vertices[vertIndex];
        Distance *= ScaleFactor;
        DilatedQuad.vertices[vertIndex] = PointOfDilation - Distance;
    };
    
    return DilatedQuad;
};

v2 CameraTransform(v2 worldCoords, Camera2D camera)
{
    v2 transformedCoords {};
    v2 translationToCameraSpace = camera.viewCenter - camera.lookAt;
    worldCoords += translationToCameraSpace;
    
    transformedCoords = _DilateAboutArbitraryPoint(camera.dilatePoint_inScreenCoords, camera.zoomFactor, worldCoords);
    
    return transformedCoords;
};

Quad CameraTransform(Quad worldCoords, Camera2D camera)
{
    Quad transformedCoords {};
    
    v2 translationToCameraSpace = camera.viewCenter - camera.lookAt;
    
    for (i32 vertIndex {}; vertIndex < 4; vertIndex++)
    {
        worldCoords.vertices[vertIndex] += translationToCameraSpace;
    };
    
    transformedCoords = _DilateAboutArbitraryPoint(camera.dilatePoint_inScreenCoords, camera.zoomFactor, worldCoords);
    
    return transformedCoords;
};

v2 ProjectionTransform_Ortho(v2 cameraCoords, f32 pixelsPerMeter)
{
    cameraCoords *= pixelsPerMeter;
    
    return cameraCoords;
};

Quad ProjectionTransform_Ortho(Quad cameraCoords, f32 pixelsPerMeter)
{
    for (i32 vertIndex {}; vertIndex < 4; vertIndex++)
        cameraCoords.vertices[vertIndex] *= pixelsPerMeter;
    
    return cameraCoords;
};

local_func auto _LinearBlend(ui32 foregroundColor, ui32 backgroundColor, ChannelType colorFormat)
{
    struct Result
    {
        ui8 blendedPixel_R, blendedPixel_G, blendedPixel_B;
    };
    Result blendedColor {};
    
    v4 foreGroundColors = UnPackPixelValues(foregroundColor, colorFormat);
    v4 backgroundColors = UnPackPixelValues(backgroundColor, colorFormat);
    
    f32 blendPercent = foreGroundColors.a / 255.0f;
    
    blendedColor.blendedPixel_R = (ui8)Lerp(backgroundColors.r, foreGroundColors.r, blendPercent);
    blendedColor.blendedPixel_G = (ui8)Lerp(backgroundColors.g, foreGroundColors.g, blendPercent);
    blendedColor.blendedPixel_B = (ui8)Lerp(backgroundColors.b, foreGroundColors.b, blendPercent);
    
    return blendedColor;
};

local_func
Rect
_ProduceRectFromCenterPoint(v2 OriginPoint, f32 width, f32 height)
{
    Rect Result;
    
    Result.min = { OriginPoint.x - (width / 2), OriginPoint.y - (height / 2) };
    Result.max = { OriginPoint.x + (width / 2), OriginPoint.y + (height / 2) };
    
    return Result;
};

local_func
Rect
_ProduceRectFromBottomMidPoint(v2 OriginPoint, f32 width, f32 height)
{
    Rect Result;
    
    Result.min = { OriginPoint.x - (width / 2.0f), OriginPoint.y };
    Result.max = { OriginPoint.x + (width / 2.0f), OriginPoint.y + height };
    
    return Result;
};

local_func
Rect
_ProduceRectFromBottomLeftPoint(v2 originPoint, f32 width, f32 height)
{
    Rect Result;
    
    Result.min = originPoint;
    Result.max = { originPoint.x + width, originPoint.y + height };
    
    return Result;
};

Quad _ProduceQuadFromBottomMidPoint(v2 originPoint, f32 width, f32 height)
{
    Quad result;
    
    result.bottomLeft = { originPoint.x - (width / 2.0f), originPoint.y };
    result.bottomRight = { originPoint.x + (width / 2.0f), originPoint.y };
    result.topRight = { originPoint.x + (width / 2.0f), originPoint.y + height };
    result.topLeft = { originPoint.x - (width / 2.0f), originPoint.y + height };
    
    return result;
};

local_func
Quad
_ProduceQuadFromBottomLeftPoint(v2 originPoint, f32 width, f32 height)
{
    Quad Result;
    
    Result.bottomLeft = originPoint;
    Result.bottomRight = { originPoint.x + width, originPoint.y };
    Result.topRight = { originPoint.x + width, originPoint.y + height };
    Result.topLeft = { originPoint.x, originPoint.y + height };
    
    return Result;
};

#endif //PLATFORM_RENDERER_STUFF_IMPL
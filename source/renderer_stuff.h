#ifndef RENDERER_STUFF_INCLUDE
#define RENDERER_STUFF_INCLUDE

#ifndef BYTES_PER_PIXEL
#define BYTES_PER_PIXEL 4
#endif

#include "my_math.h"
#include "runtime_array.h"

/*

    Current Renderer assumptions:

    1.) User sends world coordinates to renderer. 4 verts pushed per texture/rect.
    2.) Renderer expects all verts to be in meters and not pixels.
    3.) Y axis is going up and bottom left corner of rect is expected to be origin

*/

global_variable s32 bufferCount{};
global_variable s32 textureCount{};

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
    v3 worldPos{};
    v3 rotation{};
};

struct Rectf
{
    v2 min {};
    v2 max {};
};

struct Recti
{
    v2i min {};
    v2i max {};
};

struct Quadf
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

struct Geometry
{
    RunTimeArr<f32> vertAttribs{};
    RunTimeArr<s16> indicies{};
    Mat4x4 worldTransform{};
};

struct Game_Render_Cmd_Buffer
{
    u8* baseAddress { nullptr };
    s32 usedAmount {};
    s32 size {};
    s32 entryCount {};
};

struct Rendering_Info
{
    Game_Render_Cmd_Buffer cmdBuffer;
    Camera2D camera2d{};
    Camera3D camera3d{};
    f32 fov{};
    f32 aspectRatio{};
    f32 nearPlane{};
    f32 farPlane{};
    f32 _pixelsPerMeter {};
};

struct NormalMap
{
    u8* mapData { nullptr };
    f32 rotation {};
    v2 scale { 1.0f, 1.0f };
    f32 lightAngle {};
    f32 lightThreshold {};
};

struct Image
{
    u8* data { nullptr };
    f32 aspectRatio {};
    s32 width_pxls {};
    s32 height_pxls {};
    s32 pitch_pxls {};
    f32 opacity { 1.0f };
    v2 scale { 1.0f, 1.0f };
    bool isLoadedOnGPU{false};//TODO: Eventually remove
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
    EntryType_InitVertexBuffer,
    EntryType_Line,
    EntryType_Rect,
    EntryType_Geometry,
    EntryType_Texture,
    EntryType_LoadTexture
};

struct RenderEntry_Header
{
    Render_Entry_Type type;
};

struct RenderEntry_LoadTexture
{
    RenderEntry_Header header;
    Image texture;
};

struct RenderEntry_Rect
{
    RenderEntry_Header header;
    Quadf worldCoords;
    v3 color {};
};

struct RenderEntry_Geometry
{
    RenderEntry_Header header;
    u32 meshID{};
    s32 textureID{};
    RunTimeArr<v3> verts{};
    RunTimeArr<s16> indicies{};
    Mat4x4 worldTransform{};
};

struct RenderEntry_InitVertexBuffer
{
    RenderEntry_Header header;
    RunTimeArr<f32> vertAttribs{};
    RunTimeArr<s16> indicies{};
};

struct RenderEntry_Texture
{
    RenderEntry_Header header;
    const char* name;
    u8* colorData { nullptr };
    NormalMap normalMap {};
    v2i size {};
    s32 pitch_pxls {};
    v2 dimensions {};
    Array<v2, 2> uvBounds;
    Quadf targetRect_worldCoords;
    bool isLoadedOnGPU{false};//TODO: Eventually remove
};

struct RenderEntry_Line
{
    RenderEntry_Header header;
    v2 minPoint;
    v2 maxPoint;
    v3 color;
    f32 thickness;
};

Image LoadBitmap_BGRA(const char* fileName);

//Helpers
f32 BitmapWidth_Meters(Image bitmap);
f32 BitmapHeight_Meters(Rendering_Info info, Image bitmap);
v2 viewPortDimensions_Meters(Rendering_Info&& renderingInfo);

s32 InitVertexBuffer(Rendering_Info* renderingInfo, RunTimeArr<v3> objectVerts, RunTimeArr<s16> indicies);

//Render Commands 2d - Need to redoe these with matrices in mind and vertices in object space (instead of world space) before being sent down
void PushTexture(Rendering_Info&& renderingInfo, Quadf worldVerts, Image bitmap, f32 objectHeight_inMeters, Array<v2, 2> uvs, const char* name);
void PushTexture(Rendering_Info&& renderingInfo, Quadf worldVerts, Image bitmap, v2 objectSize_meters, Array<v2, 2> uvs, const char* name);
void PushRect(Rendering_Info* renderingInfo, Quadf worldVerts, v3 color);
void PushLine(Rendering_Info* renderingInfo, v2 minPoint, v2 maxPoint, v3 color, f32 thickness);
void PushCamera(Rendering_Info* renderingInfo, v2 lookAt, v2 dilatePoint_inScreenCoords, f32 zoomFactor);
void UpdateCamera2D(Rendering_Info* renderingInfo, v2 cameraLookAtCoords_meters, f32 zoomFactor);
void UpdateCamera3D(Rendering_Info* renderingInfo, v3 camWorldPos, v3 camRotation);
void RenderViaSoftware(Rendering_Info&& renderBufferInfo, void* colorBufferData, v2i colorBufferSize, s32 colorBufferPitch);

//Render Commands 3d
void PushGeometry(Rendering_Info* renderingInfo, s32 id, s32 textureID, Mat4x4 fullTransformMatrix);
s32 LoadTexture(Rendering_Info* renderingInfo, Image texture);

void ConvertNegativeToPositiveAngle_Radians(f32&& angle);
void ConvertToCorrectPositiveRadian(f32&& angle);
//void RenderToImage(Image&& renderTarget, Image sourceImage, Quadf targetArea);
Quadf WorldTransform(Quadf localCoords, Object_Transform transformInfo_world);
Quadf CameraTransform(Quadf worldCoords, Camera2D camera);
v2 CameraTransform(v2 worldCoords, Camera2D camera);
Quadf ProjectionTransform_Ortho(Quadf cameraCoords, f32 pixelsPerMeter);
v2 ProjectionTransform_Ortho(v2 cameraCoords, f32 pixelsPerMeter);
Rectf _ProduceRectFromCenterPoint(v2 OriginPoint, f32 width, f32 height);
Rectf _ProduceRectFromBottomMidPoint(v2 OriginPoint, f32 width, f32 height);
Rectf _ProduceRectFromBottomLeftPoint(v2 originPoint, f32 width, f32 height);
Quadf _ProduceQuadFromBottomLeftPoint(v2 originPoint, f32 width, f32 height);
Quadf _ProduceQuadFromBottomMidPoint(v2 originPoint, f32 width, f32 height);
Quadf ProduceQuadFromCenterPoint(v2 originPoint, f32 width, f32 height);

#endif //RENDERER_STUFF_INCLUDE_H

#ifdef GAME_RENDERER_STUFF_IMPL

void* _RenderCmdBuf_Push(Game_Render_Cmd_Buffer* commandBuf, s32 sizeOfCommand)
{
    BGZ_ASSERT(commandBuf->usedAmount < commandBuf->size, "Not enough space on render buffer!");
    
    void* memoryPointer = (void*)(commandBuf->baseAddress + commandBuf->usedAmount);
    commandBuf->usedAmount += (sizeOfCommand);
    return memoryPointer;
};
#define RenderCmdBuf_Push(commandBuffer, commandType) (commandType*)_RenderCmdBuf_Push(commandBuffer, sizeof(commandType))

void InitRenderer(Rendering_Info* renderingInfo, f32 fov, f32 aspectRatio, f32 nearPlane, f32 farPlane)
{
    //ProjectionTransform
    //f32 fov = (InvTanR(1.0f/camMagnification) * 2.0f) / (PI / 180.0f); //For if you want to use a magnification value to calculate fov 1x, 2x, 10x, etc. Adjusting fov too much causes distortion or 'fish eye' effect though
    renderingInfo->fov = fov;
    renderingInfo->aspectRatio = aspectRatio;
    renderingInfo->nearPlane = nearPlane;
    renderingInfo->farPlane = farPlane;
};

s32 InitVertexBuffer(Rendering_Info* renderingInfo, RunTimeArr<f32> objectVerts, RunTimeArr<s16> indicies)
{
    BGZ_ASSERT(objectVerts.length > 0, "Vertex array not filled. Did you load in the object data?");
    BGZ_ASSERT(indicies.length > 0, "Index array not filled. Did you load in the object data?");
    
    RenderEntry_InitVertexBuffer* bufInit = RenderCmdBuf_Push(&renderingInfo->cmdBuffer, RenderEntry_InitVertexBuffer);
    
    bufInit->header.type = EntryType_InitVertexBuffer;
    bufInit->vertAttribs = objectVerts;
    bufInit->indicies = indicies;
    
    ++renderingInfo->cmdBuffer.entryCount;
    
    return ++bufferCount;
};

s32 LoadTexture(Rendering_Info* renderingInfo, Image texture)
{
    BGZ_ASSERT(texture.data, "Invalid/null texture data!");
    
    RenderEntry_LoadTexture* loadTextureEntry = RenderCmdBuf_Push(&renderingInfo->cmdBuffer, RenderEntry_LoadTexture);
    
    loadTextureEntry->header.type = EntryType_LoadTexture;
    loadTextureEntry->texture = texture;
    
    ++renderingInfo->cmdBuffer.entryCount;
    
    return ++textureCount;
};

void PushGeometry(Rendering_Info* renderingInfo, s32 id, s32 textureID, RunTimeArr<s16> indicies, Mat4x4 worldTransform)
{
    RenderEntry_Geometry* geomEntry = RenderCmdBuf_Push(&renderingInfo->cmdBuffer, RenderEntry_Geometry);
    
    geomEntry->header.type = EntryType_Geometry;
    geomEntry->meshID = id;
    geomEntry->indicies = indicies;
    geomEntry->worldTransform = worldTransform;
    geomEntry->textureID = textureID;
    
    ++renderingInfo->cmdBuffer.entryCount;
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

void PushRect(Rendering_Info* renderingInfo, Quadf worldVerts, v3 color)
{
    RenderEntry_Rect* rectEntry = RenderCmdBuf_Push(&renderingInfo->cmdBuffer, RenderEntry_Rect);
    
    rectEntry->header.type = EntryType_Rect;
    rectEntry->color = color;
    rectEntry->worldCoords = worldVerts;
    
    ++renderingInfo->cmdBuffer.entryCount;
};

void PushTexture(Rendering_Info* renderingInfo, Quadf worldVerts, Image&& bitmap, v2 objectSize_meters, Array<v2, 2> uvs, const char* name, NormalMap normalMap = {})
{
    RenderEntry_Texture* textureEntry = RenderCmdBuf_Push(&renderingInfo->cmdBuffer, RenderEntry_Texture);
    
    textureEntry->header.type = EntryType_Texture;
    textureEntry->name = name;
    textureEntry->targetRect_worldCoords = worldVerts;
    textureEntry->colorData = bitmap.data;
    textureEntry->normalMap = normalMap;
    textureEntry->size = v2i { (s32)bitmap.width_pxls, (s32)bitmap.height_pxls };
    textureEntry->pitch_pxls = bitmap.pitch_pxls;
    textureEntry->uvBounds = uvs;
    textureEntry->isLoadedOnGPU = bitmap.isLoadedOnGPU;//TODO: Remove
    bitmap.isLoadedOnGPU = true;//TODO: Remove
    
    textureEntry->dimensions = { objectSize_meters.width, objectSize_meters.height };
    
    ++renderingInfo->cmdBuffer.entryCount;
};

//TODO: Consider not having overloaded function here? The reason this is here is to support current
//skeleton drawing with regions
void PushTexture(Rendering_Info* renderingInfo, Quadf worldVerts, Image&& bitmap, f32 objectHeight_meters, Array<v2, 2> uvs, const char* name, NormalMap normalMap = {})
{
    RenderEntry_Texture* textureEntry = RenderCmdBuf_Push(&renderingInfo->cmdBuffer, RenderEntry_Texture);
    
    f32 desiredWidth = bitmap.aspectRatio * objectHeight_meters;
    f32 desiredHeight = objectHeight_meters;
    
    textureEntry->header.type = EntryType_Texture;
    textureEntry->name = name;
    textureEntry->targetRect_worldCoords = worldVerts;
    textureEntry->colorData = bitmap.data;
    textureEntry->normalMap = normalMap;
    textureEntry->size = v2i { (s32)bitmap.width_pxls, (s32)bitmap.height_pxls };
    textureEntry->pitch_pxls = bitmap.pitch_pxls;
    textureEntry->uvBounds = uvs;
    textureEntry->isLoadedOnGPU = bitmap.isLoadedOnGPU;//TODO: Remove
    bitmap.isLoadedOnGPU = true;//TODO: Remove
    
    textureEntry->dimensions = { desiredWidth, desiredHeight };
    
    ++renderingInfo->cmdBuffer.entryCount;
};

void UpdateCamera2D(Rendering_Info* renderingInfo, v2 cameraLookAtCoords_meters, f32 zoomFactor, v2 normalizedDilatePointOffset)
{
    BGZ_ASSERT(normalizedDilatePointOffset.x >= -1.0f && normalizedDilatePointOffset.x <= 1.0f
               && normalizedDilatePointOffset.y >= -1.0f && normalizedDilatePointOffset.y <= 1.0f,
               "Dilate point is not normalized!");
    
    renderingInfo->camera2d.lookAt = cameraLookAtCoords_meters;
    renderingInfo->camera2d.zoomFactor = zoomFactor;
    renderingInfo->camera2d.dilatePointOffset_normalized = normalizedDilatePointOffset;
};

void UpdateCamera3D(Rendering_Info* renderingInfo, v3 camWorldPos, v3 camRotation)
{
    renderingInfo->camera3d.worldPos = camWorldPos;
    renderingInfo->camera3d.rotation = camRotation;
};

void UpdateCamera2D(Rendering_Info* renderingInfo, v2 cameraLookAtCoords_meters, f32 zoomFactor)
{
    renderingInfo->camera2d.lookAt = cameraLookAtCoords_meters;
    renderingInfo->camera2d.zoomFactor = zoomFactor;
};

void UpdateCamera2D(Rendering_Info* renderingInfo, f32 zoomFactor)
{
    renderingInfo->camera2d.zoomFactor = zoomFactor;
};

Image LoadBitmap_BGRA(const char* fileName)
{
    Image result;
    
    { //Load image data using stb (w/ user defined read/seek functions and memory allocation functions)
        stbi_set_flip_vertically_on_load(true); //So first byte stbi_load() returns is bottom left instead of top-left of image (which is stb's default)
        
        s32 numOfLoadedChannels {};
        s32 desiredChannels { 4 }; //Since I still draw assuming 4 byte pixels I need 4 channels
        
        //Returns RGBA
        unsigned char* imageData = stbi_load(fileName, &result.width_pxls, &result.height_pxls, &numOfLoadedChannels, desiredChannels);
        BGZ_ASSERT(imageData, "Invalid image data!");
        
        s32 totalPixelCountOfImg = result.width_pxls * result.height_pxls;
        u32* imagePixel = (u32*)imageData;
        
        //Swap R and B channels of image
        for (int i = 0; i < totalPixelCountOfImg; ++i)
        {
            auto color = UnPackPixelValues(*imagePixel, RGBA);
            
            //Pre-multiplied alpha
            f32 alphaBlend = color.a / 255.0f;
            color.rgb *= alphaBlend;
            
            u32 newSwappedPixelColor = (((u8)color.a << 24) | ((u8)color.r << 16) | ((u8)color.g << 8) | ((u8)color.b << 0));
            
            *imagePixel++ = newSwappedPixelColor;
        }
        
        result.data = (u8*)imageData;
    };
    
    result.aspectRatio = (f32)result.width_pxls / (f32)result.height_pxls;
    result.pitch_pxls = (u32)result.width_pxls * BYTES_PER_PIXEL;
    
    return result;
};

Quadf ProduceQuadFromCenterPoint(v2 originPoint, f32 width, f32 height)
{
    Quadf result;
    
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

Quadf _ProduceQuadFromBottomLeftPoint(v2 originPoint, f32 width, f32 height)
{
    Quadf Result;
    
    Result.bottomLeft = originPoint;
    Result.bottomRight = { originPoint.x + width, originPoint.y };
    Result.topRight = { originPoint.x + width, originPoint.y + height };
    Result.topLeft = { originPoint.x, originPoint.y + height };
    
    return Result;
};

#endif //GAME_RENDERER_STUFF_IMPL

#ifdef PLATFORM_RENDERER_STUFF_IMPL

local_func
Rectf
_DilateAboutArbitraryPoint(v2 PointOfDilation, f32 ScaleFactor, Rectf RectToDilate)
{
    Rectf DilatedRect {};
    
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
    v2 dilatedVector{};
    
    v2 distance = PointOfDilation - vectorToDilate;
    distance *= ScaleFactor;
    dilatedVector = PointOfDilation - distance;
    
    return dilatedVector;
};

auto _DilateAboutArbitraryPoint(v2 PointOfDilation, f32 ScaleFactor, Quadf QuadToDilate) -> Quadf
{
    Quadf DilatedQuad {};
    
    for (s32 vertIndex = 0; vertIndex < 4; ++vertIndex)
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

Quadf CameraTransform(Quadf worldCoords, Camera2D camera)
{
    Quadf transformedCoords {};
    
    v2 translationToCameraSpace = camera.viewCenter - camera.lookAt;
    
    for (s32 vertIndex {}; vertIndex < 4; vertIndex++)
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

Quadf ProjectionTransform_Ortho(Quadf cameraCoords, f32 pixelsPerMeter)
{
    for (s32 vertIndex {}; vertIndex < 4; vertIndex++)
        cameraCoords.vertices[vertIndex] *= pixelsPerMeter;
    
    return cameraCoords;
};

local_func auto _LinearBlend(u32 foregroundColor, u32 backgroundColor, ChannelType colorFormat)
{
    struct Result
    {
        u8 blendedPixel_R, blendedPixel_G, blendedPixel_B;
    };
    Result blendedColor {};
    
    v4 foreGroundColors = UnPackPixelValues(foregroundColor, colorFormat);
    v4 backgroundColors = UnPackPixelValues(backgroundColor, colorFormat);
    
    f32 blendPercent = foreGroundColors.a / 255.0f;
    
    blendedColor.blendedPixel_R = (u8)Lerp(backgroundColors.r, foreGroundColors.r, blendPercent);
    blendedColor.blendedPixel_G = (u8)Lerp(backgroundColors.g, foreGroundColors.g, blendPercent);
    blendedColor.blendedPixel_B = (u8)Lerp(backgroundColors.b, foreGroundColors.b, blendPercent);
    
    return blendedColor;
};

local_func
Rectf
_ProduceRectFromCenterPoint(v2 OriginPoint, f32 width, f32 height)
{
    Rectf Result;
    
    Result.min = { OriginPoint.x - (width / 2), OriginPoint.y - (height / 2) };
    Result.max = { OriginPoint.x + (width / 2), OriginPoint.y + (height / 2) };
    
    return Result;
};

local_func
Rectf
_ProduceRectFromBottomMidPoint(v2 OriginPoint, f32 width, f32 height)
{
    Rectf Result;
    
    Result.min = { OriginPoint.x - (width / 2.0f), OriginPoint.y };
    Result.max = { OriginPoint.x + (width / 2.0f), OriginPoint.y + height };
    
    return Result;
};

local_func
Rectf
_ProduceRectFromBottomLeftPoint(v2 originPoint, f32 width, f32 height)
{
    Rectf Result;
    
    Result.min = originPoint;
    Result.max = { originPoint.x + width, originPoint.y + height };
    
    return Result;
};

Quadf _ProduceQuadFromBottomMidPoint(v2 originPoint, f32 width, f32 height)
{
    Quadf result;
    
    result.bottomLeft = { originPoint.x - (width / 2.0f), originPoint.y };
    result.bottomRight = { originPoint.x + (width / 2.0f), originPoint.y };
    result.topRight = { originPoint.x + (width / 2.0f), originPoint.y + height };
    result.topLeft = { originPoint.x - (width / 2.0f), originPoint.y + height };
    
    return result;
};

#endif //PLATFORM_RENDERER_STUFF_IMPL
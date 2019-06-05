#ifndef RENDERER_STUFF_INCLUDE_H
#define RENDERER_STUFF_INCLUDE_H

#ifndef BYTES_PER_PIXEL 
#define BYTES_PER_PIXEL 4
#endif

struct Image
{
    ui8* data;
    f32 aspectRatio;
    f32 height_meters;
    ui32 pitch;
    f32 opacity {1.0f};
};

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

struct Rectf
{
    v2f min;
    v2f max;
};

struct Recti
{
    v2i min;
    v2i max;
};

struct Coordinate_Space
{
    v2f origin;
    v2f xBasis;
    v2f yBasis;
};

struct Object_Transform
{
    f32 rotation;
    v2f pos;
    v2f scale;
};

enum Render_Entry_Type
{
    EntryType_OrthoProj,
    EntryType_Texture,
    EntryType_2DCamera
};

struct RenderEntry_Header
{
    Render_Entry_Type type;
};

struct RenderEntry_OrthoProj
{
    v2f screenDimensions;
};

struct RenderEntry_Texture
{
    RenderEntry_Header header;
    Object_Transform world;
    ui8* colorData;
    v2i size;
    ui32 pitch;
    v2i targetRectSize;
};

struct RenderEntry_2DCamera
{
    RenderEntry_Header header;
    v2f lookAt;
    v2f viewCenter;
    v2f viewDims;
    v2f dilatePoint;
    f32 zoomFactor;
};

Image LoadBitmap(const char* fileName);
f32 BitmapWidth_meters(Image bitmap);

void PushTexture(Game_Render_Cmd_Buffer&& bufferInfo, Image bitmap, f32 hieghtOfObject_inMeters, f32 worldRotation, v2f worldPos, v2f worldScale);
void PushCamera(Game_Render_Cmd_Buffer* bufferInfo, v2f lookAt, v2f dilatePoint, f32 zoomFactor);
void RenderViaSoftware(Game_Render_Cmd_Buffer&& renderBufferInfo, void* colorBufferData, v2i colorBufferSize, i32 colorBufferPitch);


void ConvertNegativeAngleToRadians(f32&& angle);
void ConvertToCorrectPositiveRadian(f32&& angle);
//void RenderToImage(Image&& renderTarget, Image sourceImage, Quadf targetArea);
Quadf WorldTransform(Quadf localCoords, Object_Transform transformInfo_world);
Quadf CameraTransform(Quadf worldCoords, RenderEntry_2DCamera camera);
Rectf _ProduceRectFromCenterPoint(v2f OriginPoint, f32 width, f32 height);
Rectf _ProduceRectFromBottomMidPoint(v2f OriginPoint, f32 width, f32 height);
Rectf _ProduceRectFromBottomLeftPoint(v2f originPoint, f32 width, f32 height);
Quadf _ProduceQuadFromBottomLeftPoint(v2f originPoint, f32 width, f32 height);


#endif //RENDERER_STUFF_INCLUDE_H 



#ifdef GAME_RENDERER_STUFF_IMPL

f32 pixelsPerMeter{200.0f};

void* _RenderCmdBuf_Push(Game_Render_Cmd_Buffer* commandBuf, i32 sizeOfCommand)
{
    void* memoryPointer = (void*)(commandBuf->baseAddress + commandBuf->usedAmount);
    commandBuf->usedAmount += (sizeOfCommand);
    return memoryPointer;
};
#define RenderCmdBuf_Push(commandBuffer, commandType) (commandType*)_RenderCmdBuf_Push(commandBuffer, sizeof(commandType))

void SetProjection_Ortho(Game_Render_Cmd_Buffer* bufferInfo, v2f screenDimensions_pixels)
{
    RenderEntry_OrthoProj* ortho = RenderCmdBuf_Push(bufferInfo, RenderEntry_OrthoProj);

    ortho->screenDimensions = screenDimensions_pixels;
};

void PushTexture(Game_Render_Cmd_Buffer* bufferInfo, Image bitmap, f32 objectHeight_meters, f32 rotation, v2f pos, v2f scale)
{
    RenderEntry_Texture* textureEntry = RenderCmdBuf_Push(bufferInfo, RenderEntry_Texture);

    f32 bitmapWidth_pixels = bitmap.aspectRatio* bitmap.height_meters * pixelsPerMeter;
    f32 bitmapHeight_pixels = bitmap.height_meters * pixelsPerMeter;

    f32 desiredWidth_pixels = bitmap.aspectRatio* objectHeight_meters * pixelsPerMeter;
    f32 desiredHeight_pixels = objectHeight_meters * pixelsPerMeter;

    textureEntry->header.type = EntryType_Texture;
    textureEntry->world.rotation = rotation;
    textureEntry->world.pos = pos * pixelsPerMeter;
    textureEntry->world.scale = scale;
    textureEntry->colorData = bitmap.data;
    textureEntry->size = v2i{(i32)bitmapWidth_pixels, (i32)bitmapHeight_pixels};
    textureEntry->pitch = (ui32)bitmapWidth_pixels * BYTES_PER_PIXEL;
    textureEntry->targetRectSize= v2i{(i32)desiredWidth_pixels, (i32)desiredHeight_pixels};

    ++bufferInfo->entryCount;
};

void PushCamera(Game_Render_Cmd_Buffer* bufferInfo, v2f lookAt, v2f dilatePoint, f32 zoomFactor)
{
    RenderEntry_2DCamera* camera = RenderCmdBuf_Push(bufferInfo, RenderEntry_2DCamera);
    camera->lookAt = lookAt * pixelsPerMeter;
    camera->viewCenter = v2f{1280.0f / 2.0f, 720.0f / 2.0f};//TODO: Need to base this off of actual screen dimensions that were set
    camera->dilatePoint = dilatePoint;
    camera->zoomFactor = zoomFactor;
};

Image LoadBitmap(const char* fileName)
{
    Image result;

    f32 pixelsPerMeter = 200.0f;
    i32 width_inPixels, height_inPixels;
    result.data = globalPlatformServices->LoadBGRAImage(fileName, $(width_inPixels), $(height_inPixels));
    result.aspectRatio = (f32)width_inPixels/(f32)height_inPixels;
    result.height_meters = (f32)height_inPixels / pixelsPerMeter;

    return result;
};

f32 BitmapWidth_meters(Image bitmap)
{
    f32 width_inMeters = bitmap.aspectRatio * bitmap.height_meters;

    return width_inMeters;
};

#endif //GAME_RENDERER_STUFF_IMPL



#ifdef PLATFORM_RENDERER_STUFF_IMPL

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

Quadf WorldTransform(Quadf localCoords, Object_Transform transformInfo_world)
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

Quadf CameraTransform(Quadf worldCoords, RenderEntry_2DCamera camera)
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

#endif //PLATFORM_RENDERER_STUFF_IMPL
#ifndef OPENGL_INCLUDE_H
#define OPENGL_INCLUDE_H

#include "renderer_stuff.h"

const char* vertexShaderCode =
R"HereDoc(

#version 430

in layout(location=0) vec4 position;
in layout(location=1) vec3 color;
out vec3 fragColor;

void main()
{
    gl_Position = position;
    vec3 changedColors;
    changedColors.r += color.r + .41;
    changedColors.g += color.g + .04;
    changedColors.b += color.b + .02;
    fragColor = changedColors;
};

)HereDoc";

const char* fragmentShaderCode =
R"HereDoc(

#version 430

out vec4 color;
in vec3 fragColor;

void main()
{
    color = vec4(fragColor, 1.0f);
};

)HereDoc";

struct Quadv3
{
    union
    {
        Array<v3, 4> vertices;
        struct
        {
            v3 bottomLeft;
            v3 bottomRight;
            v3 topRight;
            v3 topLeft;
        };
    };
};

struct Camera3D
{
    //What goes in here;
    f32 distanceFromMonitor_meters{};//Focal length
    f32 cameraDistanceFromTarget_meters{};//Camera distance above target
};

global_variable Camera3D globalCamera;

mat4x4 OrthographicProjection(f32 aspectRatio)
{
    f32 a = 1.0f;
    f32 b = aspectRatio;
    
    mat4x4 r
    {
        {
            {a, 0, 0, 0},
            {0, b, 0, 0},
            {0, 0, 1, 0},
            {0, 0, 0, 1}
        }
    };
    
    return r;
};

#if 0
Quadv3 CameraTransform(Quadv3 worldCoords, Camera3D camera)
{
    Quadv3 transformedCoords {};
    
    v2 translationToCameraSpace = camera.viewCenter - camera.lookAt;
    
    for (i32 vertIndex {}; vertIndex < 4; vertIndex++)
    {
        worldCoords.vertices[vertIndex] += translationToCameraSpace;
    };
    
    transformedCoords = _DilateAboutArbitraryPoint(camera.dilatePoint_inScreenCoords, camera.zoomFactor, worldCoords);
    
    return transformedCoords;
};
#endif

void CheckCompileStatus(GLuint shaderID)
{
    GLint compileStatus;
    glGetShaderiv(shaderID, GL_COMPILE_STATUS, &compileStatus);
    
    if(compileStatus != GL_TRUE)
    {
        GLint infoLogLength;
        glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &infoLogLength);
        
        GLchar buffer[512] = {};
        GLsizei bufferSize;
        glGetShaderInfoLog(shaderID, infoLogLength, &bufferSize, buffer);
        
        BGZ_CONSOLE("%s", buffer);
        InvalidCodePath;
    };
};

void CheckLinkStatus(GLuint programID)
{
    GLint linkStatus;
    glGetProgramiv(programID, GL_LINK_STATUS, &linkStatus);
    
    if(linkStatus != GL_TRUE)
    {
        GLint infoLogLength;
        glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &infoLogLength);
        
        GLchar buffer[512] = {};
        GLsizei bufferSize;
        glGetProgramInfoLog(programID, infoLogLength, &bufferSize, buffer);
        
        BGZ_CONSOLE("%s", buffer);
        InvalidCodePath;
    };
};

local_func void InstallShaders()
{
    GLuint vertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);
    
    const char* adapter[1];
    adapter[0] = vertexShaderCode;
    glShaderSource(vertexShaderID, 1, adapter, 0);
    adapter[0] = fragmentShaderCode;
    glShaderSource(fragmentShaderID, 1, adapter, 0);
    
    glCompileShader(vertexShaderID);
    glCompileShader(fragmentShaderID);
    
    CheckCompileStatus(vertexShaderID);
    CheckCompileStatus(fragmentShaderID);
    
    GLuint programID = glCreateProgram();
    glAttachShader(programID, vertexShaderID);
    glAttachShader(programID, fragmentShaderID);
    glLinkProgram(programID);
    
    CheckLinkStatus(programID);
    
    glUseProgram(programID);
};

local_func void
GLInit()
{
    //If this is set to GL_MODULATE instead then you might get unwanted texture coloring.
    //In order to avoid that in GL_MODULATE mode you need to constantly set glcolor to white after drawing.
    //For more info: https://stackoverflow.com/questions/53180760/all-texture-colors-affected-by-colored-rectangle-opengl
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glEnable(GL_DEPTH_TEST);
    InstallShaders();
}

local_func ui32
LoadTexture(ui8* textureData, v2i textureSize)
{
    ui32 textureID {};
    glEnable(GL_TEXTURE_2D);
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, textureSize.width, textureSize.height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, textureData);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    
    //Enable alpha channel for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    return textureID;
}

local_func void
DrawTexture(ui32 TextureID, Quadf textureCoords, v2 MinUV, v2 MaxUV)
{
    glBindTexture(GL_TEXTURE_2D, TextureID);
    
    glBegin(GL_QUADS);
    glTexCoord2f(MinUV.x, MinUV.y);
    glVertex2f(textureCoords.bottomLeft.x, textureCoords.bottomLeft.y);
    
    glTexCoord2f(MaxUV.x, MinUV.y);
    glVertex2f(textureCoords.bottomRight.x, textureCoords.bottomRight.y);
    
    glTexCoord2f(MaxUV.x, MaxUV.y);
    glVertex2f(textureCoords.topRight.x, textureCoords.topRight.y);
    
    glTexCoord2f(MinUV.x, MaxUV.y);
    glVertex2f(textureCoords.topLeft.x, textureCoords.topLeft.y);
    
    glEnd();
    glFlush();
    
    glBindTexture(GL_TEXTURE_2D, 0);
}

local_func void
DrawQuad(Quadf quad, v4 color)
{
    glBegin(GL_QUADS);
    
    glColor4f(color.r, color.g, color.b, color.a);
    glVertex2f(quad.bottomLeft.x, quad.bottomLeft.y);
    glColor4f(color.r, color.g, color.b, color.a);
    glVertex2f(quad.bottomRight.x, quad.bottomRight.y);
    glColor4f(color.r, color.g, color.b, color.a);
    glVertex2f(quad.topRight.x, quad.topRight.y);
    glColor4f(color.r, color.g, color.b, color.a);
    glVertex2f(quad.topLeft.x, quad.topLeft.y);
    
    glEnd();
    glFlush();
    
    glColor3f(1.0f, 1.0f, 1.0f);
};

local_func void
DrawQuad(Quadv3 quad, v4 color)
{
    glBegin(GL_QUADS);
    
    glColor4f(color.r, color.g, color.b, color.a);
    glVertex3f(quad.bottomLeft.x, quad.bottomLeft.y, quad.bottomLeft.z);
    glColor4f(color.r, color.g, color.b, color.a);
    glVertex3f(quad.bottomRight.x, quad.bottomRight.y, quad.bottomRight.z);
    glColor4f(color.r, color.g, color.b, color.a);
    glVertex3f(quad.topRight.x, quad.topRight.y, quad.topRight.z);
    glColor4f(color.r, color.g, color.b, color.a);
    glVertex3f(quad.topLeft.x, quad.topLeft.y, quad.topLeft.z);
    
    glEnd();
    glFlush();
    
    glColor3f(1.0f, 1.0f, 1.0f);
};

local_func void
DrawRect(v2 MinPoint, v2 MaxPoint, v4 color)
{
    glBegin(GL_QUADS);
    
    glColor4f(color.r, color.g, color.b, color.a);
    glVertex2f(MaxPoint.x, MaxPoint.y);
    glColor4f(color.r, color.g, color.b, color.a);
    glVertex2f(MinPoint.x, MaxPoint.y);
    glColor4f(color.r, color.g, color.b, color.a);
    glVertex2f(MinPoint.x, MinPoint.y);
    glColor4f(color.r, color.g, color.b, color.a);
    glVertex2f(MaxPoint.x, MinPoint.y);
    
    glEnd();
    glFlush();
    
    glColor3f(1.0f, 1.0f, 1.0f);
}

local_func void
DrawLine(v2 minPoint, v2 maxPoint, v3 color, f32 lineThickness)
{
    glLineWidth(lineThickness);
    glBegin(GL_LINES);
    glColor3f(color.r, color.g, color.b);
    glVertex2f(minPoint.x, minPoint.y);
    glColor3f(color.r, color.g, color.b);
    glVertex2f(maxPoint.x, maxPoint.y);
    glEnd();
    glFlush();
};

struct Transform_v3
{
    v3 translation{};
    v3 rotation{};
    v3 scale{};
};

#define NUM_VERTS 8
struct Cube
{
    Array<v3, NUM_VERTS> verts{};
    v3 centerPoint{};
};

GLushort indicies[] =
{
    1, 0, 2,  1, 2, 3,//Front
    4, 1, 3,  5, 1, 4, //Right side
    4, 5, 7,  7, 5, 6, //Back side
    6, 7, 0,  0, 7, 2 //Left side
};

f32 epsilon = 0.00001f; //TODO: Remove????

v3 CenterOfCube(Array<v3, NUM_VERTS> cubeVerts)//Find center of cube in order to rotate in camera space next
{
    f32 sumXs{}, sumYs{}, sumZs{};
    for(i32 i{}; i < NUM_VERTS; ++i)
        sumXs += cubeVerts[i].x;
    for(i32 i{}; i < NUM_VERTS; ++i)
        sumYs += cubeVerts[i].y;
    for(i32 i{}; i < NUM_VERTS; ++i)
        sumZs += cubeVerts[i].z;
    v3 centerOfCube = v3 { sumXs/8.0f, sumYs/8.0f, sumZs/8.0f };
    
    return centerOfCube;
}


Array<v4, NUM_VERTS> ProjectionTransform_UsingFocalLength(Array<v3, NUM_VERTS> squareVerts_camera, f32 windowWidth_pxls, f32 windowHeight_pxls)
{
    Array<v4, NUM_VERTS> squareVerts_openGLClipSpace{};
    
    f32 focalLength = 1.8f;
    f32 windowWidth_meters = windowWidth_pxls / 100.0f;
    f32 windowHeight_meters = windowHeight_pxls / 100.0f;
    
    for(i32 vertI{}; vertI < NUM_VERTS; ++vertI)
    {
        squareVerts_openGLClipSpace[vertI].x = (squareVerts_camera[vertI].x * focalLength) / (windowWidth_meters / 2.0f);
        squareVerts_openGLClipSpace[vertI].y = (squareVerts_camera[vertI].y * focalLength) / (windowHeight_meters / 2.0f);
        squareVerts_openGLClipSpace[vertI].z = 1.0f;
        squareVerts_openGLClipSpace[vertI].w = squareVerts_camera[vertI].z;
    };
    
    return squareVerts_openGLClipSpace;
};

struct Basis
{
    v3 origin{};//Universal space
    v3 xAxis{1.0f, 0.0f, 0.0f};
    v3 yAxis{0.0f, 1.0f, 0.0f};
    v3 zAxis{0.0f, 0.0f, 1.0f};
    v3 translation{};
};

mat4x4 XRotation(f32 Angle)
{
    f32 c = CosR(Angle);
    f32 s = SinR(Angle);
    
    mat4x4 R =
    {
        {
            {1, 0, 0, 0},
            {0, c,-s, 0},
            {0, s, c, 0},
            {0, 0, 0, 1}
        },
    };
    
    return(R);
}

inline mat4x4
YRotation(f32 Angle)
{
    f32 c = CosR(Angle);
    f32 s = SinR(Angle);
    
    mat4x4 R =
    {
        {
            { c, 0, s, 0},
            { 0, 1, 0, 0},
            {-s, 0, c, 0},
            { 0, 0, 0, 1}
        },
    };
    
    return(R);
}

inline mat4x4
ZRotation(f32 Angle)
{
    f32 c = CosR(Angle);
    f32 s = SinR(Angle);
    
    mat4x4 R =
    {
        {
            {c,-s, 0, 0},
            {s, c, 0, 0},
            {0, 0, 1, 0},
            {0, 0, 0, 1}
        },
    };
    
    return(R);
}

mat4x4 ProduceWorldTransform(v3 translation, v3 rotation, v3 scale)
{
    mat4x4 result{};
    
    ConvertToCorrectPositiveRadian($(rotation.x));
    ConvertToCorrectPositiveRadian($(rotation.y));
    ConvertToCorrectPositiveRadian($(rotation.z));
    
    mat4x4 xRotMatrix = XRotation(rotation.x);
    mat4x4 yRotMatrix = YRotation(rotation.y);
    mat4x4 zRotMatrix = ZRotation(rotation.z);
    mat4x4 fullRotMatrix = xRotMatrix * yRotMatrix * zRotMatrix;
    
    result = Translate(fullRotMatrix, v4{translation, 1.0f});
    
    return result;
};

local_func mat4x4
ProduceCameraTransform(v3 xAxis, v3 yAxis, v3 zAxis, v3 vecToTransform)
{
    mat4x4 result = RowPicture3x3(xAxis, yAxis, zAxis);
    v4 vecToTransform_4d {vecToTransform, 1.0f};
    result = Translate(result, -(result*vecToTransform_4d));
    
    return result;
};

mat4x4 ProduceProjectionTransform_UsingFOV(Array<v3, NUM_VERTS> squareVerts_camera, f32 FOV_inDegrees, f32 aspectRatio, f32 nearPlane, f32 farPlane)
{
    Array<v4, NUM_VERTS> squareVerts_openGLClipSpace{};
    
    f32 fov = Radians(FOV_inDegrees);
    f32 tanHalfFov = TanR(fov / 2.0f);
    f32 xScale = 1.0f / (tanHalfFov * aspectRatio);
    f32 yScale = 1.0f / tanHalfFov;
    
    f32 a = (-farPlane - nearPlane) / (nearPlane - farPlane);
    f32 b = (2.0f * farPlane * nearPlane) / (nearPlane - farPlane);
    
    mat4x4 result =
    {
        {
            {xScale, 0,      0,  0},
            {  0,    yScale, 0,  0},
            {  0,    0,      a,  b},
            {  0,    0,      1,  0}
        },
    };
    
    return result;
};

void DrawCube(Array<v4, NUM_VERTS> cubeVerts_glClipSpace)
{
    GLfloat verts[NUM_VERTS * 7] = {};
    i32 i{};
    f32 colorR{}, colorG{}, colorB{};
    for(i32 j{}; j < NUM_VERTS; ++j)
    {
        verts[i++] = cubeVerts_glClipSpace[j].x;
        verts[i++] = cubeVerts_glClipSpace[j].y;
        verts[i++] = cubeVerts_glClipSpace[j].z;
        verts[i++] = cubeVerts_glClipSpace[j].w;
        verts[i++] = colorR;
        verts[i++] = colorG;
        verts[i++] = colorB;
        
        if(colorR > 1.0f)
        {
            colorR = 0.0f;
        };
        
        if(colorG > 1.0f)
        {
            colorG = 0.0f;
        };
        
        if(colorB > 1.0f)
        {
            colorB = 0.0f;
        };
        
        colorR += .01f;
        colorG += .54f;
        colorB += .27f;
    };
    
    GLuint bufferID;
    glGenBuffers(1, &bufferID);
    glBindBuffer(GL_ARRAY_BUFFER, bufferID);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 7, 0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 7, (char*)(sizeof(GLfloat)*3));
    
    GLuint indexBufferID;
    glGenBuffers(1, &indexBufferID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indicies), indicies, GL_STATIC_DRAW);
    
    glDisable(GL_TEXTURE_2D);
    glDrawElements(GL_TRIANGLES, 24, GL_UNSIGNED_SHORT, 0);
    glEnable(GL_TEXTURE_2D);
};

void ProjectionTestUsingFullSquare(Cube cube, Transform_v3 worldTransform, f32 windowWidth, f32 windowHeight)
{
    mat4x4 worldTransformMatrix = ProduceWorldTransform(worldTransform.translation, worldTransform.rotation, worldTransform.scale);
    
    //World Transform
    Array<v3, NUM_VERTS> cubeVerts_world{};
    for(i32 i{}; i < NUM_VERTS; ++i)
        cubeVerts_world[i] = (worldTransformMatrix * v4{cube.verts[i], 1.0f}).xyz;
    
    Array<v3, NUM_VERTS> cubeVerts_camera{};
    {//Camera Transform
        local_persist v3 cameraPostion_world{0.0f, 0.0f, -2.0f};
        local_persist v3 rotation_camera{0.0f, 0.0f, 0.0f};
        
        Basis camera{};
        local_persist v3 camRotation{0.0f, 0.0f, 0.0f};
        mat4x4 xRotMatrix = XRotation(camRotation.x);
        mat4x4 yRotMatrix = YRotation(camRotation.y);
        mat4x4 zRotMatrix = ZRotation(camRotation.z);
        mat4x4 fullRotMatrix = xRotMatrix * yRotMatrix * zRotMatrix;
        v3 xAxis = GetColumn(fullRotMatrix, 0);
        v3 yAxis = GetColumn(fullRotMatrix, 1);
        v3 zAxis = GetColumn(fullRotMatrix, 2);
        mat4x4 camTransform = ProduceCameraTransform(xAxis, yAxis, zAxis, cameraPostion_world);
        
        for(i32 i{}; i < NUM_VERTS; ++i)
            cubeVerts_camera[i] = (camTransform * v4{cubeVerts_world[i], 1.0f}).xyz;
    }
    
    //ProjectionTransform
    mat4x4 projectionMatrix = ProduceProjectionTransform_UsingFOV(cubeVerts_camera, 90.0f, 16.0f/9.0f, .1f, 100.0f);
    Array<v4, NUM_VERTS> cubeVerts_openGLClipSpace{};
    for(i32 i{}; i < NUM_VERTS; ++i)
        cubeVerts_openGLClipSpace[i] = projectionMatrix * v4{cubeVerts_camera[i], 1.0f};
    
    DrawCube(cubeVerts_openGLClipSpace);
};

void RenderViaHardware(Rendering_Info&& renderingInfo, int windowWidth, int windowHeight)
{
    local_persist b glIsInitialized { false };
    if (NOT glIsInitialized)
    {
        GLInit();
        glClearColor(0.0f, 0.0f, 1.0f, 0.0f);
        glIsInitialized = true;
    };
    
    glViewport(0, 0, windowWidth, windowHeight);
    
    ui8* currentRenderBufferEntry = renderingInfo.cmdBuffer.baseAddress;
    Camera2D* camera = &renderingInfo.camera;
    
    f32 pixelsPerMeter = renderingInfo._pixelsPerMeter;
    v2i screenSize = { windowWidth, windowHeight };
    v2 screenSize_meters = CastV2IToV2F(screenSize) / pixelsPerMeter;
    camera->dilatePoint_inScreenCoords = (screenSize_meters / 2.0f) + (Hadamard(screenSize_meters, camera->dilatePointOffset_normalized));
    
    camera->viewCenter = screenSize_meters / 2.0f;
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glEnable(GL_TEXTURE_2D);
    
    for (i32 entryNumber = 0; entryNumber < renderingInfo.cmdBuffer.entryCount; ++entryNumber)
    {
        RenderEntry_Header* entryHeader = (RenderEntry_Header*)currentRenderBufferEntry;
        switch (entryHeader->type)
        {
            case EntryType_Texture: {
                RenderEntry_Texture textureEntry = *(RenderEntry_Texture*)currentRenderBufferEntry;
                
                local_persist ui32 textureID{};
                if (NOT textureEntry.isLoadedOnGPU)
                {
                    textureID = LoadTexture(textureEntry.colorData, v2i { textureEntry.size.width, textureEntry.size.height });
                };
                
                glBindTexture(GL_TEXTURE_2D, textureID);
                
                Quadf imageTargetRect_camera = CameraTransform(textureEntry.targetRect_worldCoords, *camera);
                Quadf imageTargetRect_screen = ProjectionTransform_Ortho(imageTargetRect_camera, pixelsPerMeter);
                
                DrawTexture(textureID, imageTargetRect_screen, textureEntry.uvBounds[0], textureEntry.uvBounds[1]);
                
                currentRenderBufferEntry += sizeof(RenderEntry_Texture);
            }
            break;
            
            case EntryType_Rect: {
                RenderEntry_Rect rectEntry = *(RenderEntry_Rect*)currentRenderBufferEntry;
                
                Quadf targetRect_camera = CameraTransform(rectEntry.worldCoords, *camera);
                Quadf targetRect_screen = ProjectionTransform_Ortho(targetRect_camera, pixelsPerMeter);
                
                v4 color = { rectEntry.color, 1.0f };
                
                glDisable(GL_TEXTURE_2D);
                DrawQuad(targetRect_screen, color);
                glEnable(GL_TEXTURE_2D);
                
                currentRenderBufferEntry += sizeof(RenderEntry_Rect);
            }
            break;
            
            case EntryType_Line: {
                RenderEntry_Line lineEntry = *(RenderEntry_Line*)currentRenderBufferEntry;
                
                v2 lineMinPoint_camera = CameraTransform(lineEntry.minPoint, *camera);
                v2 lineMaxPoint_camera = CameraTransform(lineEntry.maxPoint, *camera);
                
                v2 lineMinPoint_screen = ProjectionTransform_Ortho(lineMinPoint_camera, pixelsPerMeter);
                v2 lineMaxPoint_screen = ProjectionTransform_Ortho(lineMaxPoint_camera, pixelsPerMeter);
                lineEntry.minPoint = lineMinPoint_screen;
                lineEntry.maxPoint = lineMaxPoint_screen;
                
                glDisable(GL_TEXTURE_2D);
                DrawLine(lineEntry.minPoint, lineEntry.maxPoint, lineEntry.color, lineEntry.thickness);
                glEnable(GL_TEXTURE_2D);
                
                currentRenderBufferEntry += sizeof(RenderEntry_Line);
            }
            break;
            
            case EntryType_Test:
            {
                Cube cube0{}, cube1{};
                
                cube0.verts = {
                    v3{-0.5f, 0.5f, -0.5f }, //0
                    v3{+0.5f, 0.5f, -0.5f }, //1
                    v3{-0.5f, -0.5f, -0.5f },//2
                    v3{+0.5f, -0.5f, -0.5f },//3
                    v3{+0.5f, -0.5f, +0.5f },//4
                    v3{+0.5f, +0.5f, +0.5f },//5
                    v3{-0.5f, +0.5f, +0.5f },//6
                    v3{-0.5f, -0.5f, +0.5f },//7
                };
                
                cube1.verts = {
                    v3{-0.5f, 0.5f, -0.5f }, //0
                    v3{+0.5f, 0.5f, -0.5f }, //1
                    v3{-0.5f, -0.5f, -0.5f },//2
                    v3{+0.5f, -0.5f, -0.5f },//3
                    v3{+0.5f, -0.5f, +0.5f },//4
                    v3{+0.5f, +0.5f, +0.5f },//5
                    v3{-0.5f, +0.5f, +0.5f },//6
                    v3{-0.5f, -0.5f, +0.5f },//7
                };
                
                Transform_v3 cube0_worldT{}, cube1_worldT{};
                cube0_worldT.translation = {2.0f, 0.0f, 1.0f};
                cube0_worldT.rotation = {0.0f, 0.0f, 0.0f};
                cube0_worldT.scale = {1.0f, 1.0f, 1.0f};
                
                cube1_worldT.translation = {0.0f, 0.0f, 1.0f};
                cube1_worldT.rotation = {0.0f, 0.0f, 0.0f};
                cube1_worldT.scale = {1.0f, 1.0f, 1.0f};
                
                ProjectionTestUsingFullSquare(cube0, cube0_worldT, (f32)windowWidth, (f32)windowHeight);
                ProjectionTestUsingFullSquare(cube1, cube1_worldT, (f32)windowWidth, (f32)windowHeight);
            }break;
            
            InvalidDefaultCase;
        };
    }
    
    renderingInfo.cmdBuffer.entryCount = 0;
};

#endif //OPENGL_INCLUDE_H
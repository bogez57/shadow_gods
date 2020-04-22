#ifndef OPENGL_INCLUDE_H
#define OPENGL_INCLUDE_H

#include "renderer_stuff.h"

const char* vertexShaderCode =
"#version 430 \r\n"
"in layout(location=0) vec2 position;\n"
"in layout(location=1) vec3 color;\n"
"out vec3 fragColor;\n"
"\n"
"void main()\n"
"{\n"
"    gl_Position = vec4(position, 0.0, 1.0f); \n"
"    fragColor = color;\n"
"}\n";

const char* fragmentShaderCode =
"#version 430 \r\n"
"out vec4 color;\n"
"in vec3 fragColor;\n"
"\n"
"void main()\n"
"{\n"
"    color = vec4(fragColor, 1.0f);\n"
"}\n";

struct Quadv3f
{
    union
    {
        Array<v3f, 4> vertices;
        struct
        {
            v3f bottomLeft;
            v3f bottomRight;
            v3f topRight;
            v3f topLeft;
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
Quadv3f CameraTransform(Quadv3f worldCoords, Camera3D camera)
{
    Quadv3f transformedCoords {};
    
    v2f translationToCameraSpace = camera.viewCenter - camera.lookAt;
    
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
DrawTexture(ui32 TextureID, Quadf textureCoords, v2f MinUV, v2f MaxUV)
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
DrawQuad(Quadf quad, v4f color)
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
DrawQuad(Quadv3f quad, v4f color)
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
DrawRect(v2f MinPoint, v2f MaxPoint, v4f color)
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
DrawLine(v2f minPoint, v2f maxPoint, v3f color, f32 lineThickness)
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

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/ext/matrix_clip_space.hpp> // glm::perspective

Array<glm::vec4, 6> TransformFromClipSpaceToScreenSpace(Array<glm::vec4, 6> clipSpaceVerts, f32 windowWidth, f32 windowHeight)
{
    Array<glm::vec4, 6> screenSpaceCoords{};
    for(i32 vertI{}; vertI < 6; ++vertI)
    {
        screenSpaceCoords[vertI].x = (clipSpaceVerts[vertI].x + 1) * (windowWidth/2);
        screenSpaceCoords[vertI].y = (clipSpaceVerts[vertI].y + 1) * (windowHeight/2);
    };
    
    return screenSpaceCoords;
};

Array<glm::vec4, 24> testSquare =
{
    glm::vec4{-1.0f, +1.0f, +1.0f, 1.0f}, //0
    glm::vec4{+1.0f, +1.0f, +1.0f, 1.0f}, //1
    glm::vec4{+1.0f, +1.0f, -1.0f, 1.0f}, //2
    glm::vec4{-1.0f, +1.0f, -1.0f, 1.0f}, //3
    
    glm::vec4{-1.0f, +1.0f, -1.0f, 1.0f}, //4
    glm::vec4{+1.0f, +1.0f, -1.0f, 1.0f}, //5
    glm::vec4{+1.0f, -1.0f, -1.0f, 1.0f}, //6
    glm::vec4{-1.0f, -1.0f, -1.0f, 1.0f}, //7
    
    glm::vec4{+1.0f, +1.0f, -1.0f, 1.0f}, //8
    glm::vec4{+1.0f, +1.0f, +1.0f, 1.0f}, //9
    glm::vec4{+1.0f, -1.0f, +1.0f, 1.0f}, //10
    glm::vec4{+1.0f, -1.0f, -1.0f, 1.0f}, //11
    
    glm::vec4{-1.0f, +1.0f, +1.0f, 1.0f}, //12
    glm::vec4{-1.0f, +1.0f, -1.0f, 1.0f}, //13
    glm::vec4{-1.0f, -1.0f, -1.0f, 1.0f}, //14
    glm::vec4{-1.0f, -1.0f, +1.0f, 1.0f}, //15
    
    glm::vec4{+1.0f, +1.0f, +1.0f, 1.0f}, //16
    glm::vec4{-1.0f, +1.0f, +1.0f, 1.0f}, //17
    glm::vec4{-1.0f, -1.0f, +1.0f, 1.0f}, //18
    glm::vec4{+1.0f, -1.0f, +1.0f, 1.0f}, //19
    
    glm::vec4{+1.0f, -1.0f, -1.0f, 1.0f}, //20
    glm::vec4{-1.0f, -1.0f, -1.0f, 1.0f}, //21
    glm::vec4{-1.0f, -1.0f, +1.0f, 1.0f}, //22
    glm::vec4{+1.0f, -1.0f, +1.0f, 1.0f}, //23
};

f32 epsilon = 0.00001f; //TODO: Remove????

void PixelUnitProjectionTest(f32 windowWidth, f32 windowHeight)
{
    Array<glm::vec4, 6> squareVerts_screenCoords =
    {
        glm::vec4{200.0f, 600.0f, 400.0f, 1.0f},
        glm::vec4{300.0f, 600.0f, 300.0f, 1.0f},
        glm::vec4{400.0f, 600.0f, 400.0f, 1.0f},
        
        glm::vec4{200.0f, 400.0f, 400.0f, 1.0f},
        glm::vec4{300.0f, 400.0f, 300.0f, 1.0f},
        glm::vec4{400.0f, 400.0f, 400.0f, 1.0f}
    };
    
    {//Projection transform - my version
        f32 camDistanceFromMonitor_z = 800.0f;
        
        for(i32 vertI{}; vertI < 6; ++vertI)
        {
            f32 pointDistanceFromCamera_z = camDistanceFromMonitor_z + squareVerts_screenCoords[vertI].z;
            
            f32 numerator = squareVerts_screenCoords[vertI].x * camDistanceFromMonitor_z;
            squareVerts_screenCoords[vertI].x = Round(numerator / pointDistanceFromCamera_z);
            
            numerator = squareVerts_screenCoords[vertI].y * camDistanceFromMonitor_z;
            squareVerts_screenCoords[vertI].y = Round(numerator / pointDistanceFromCamera_z);
        };
    };
    
    Array<glm::vec4, 6> squareVerts_openGLClipSpace;
    {//Do openGL clip space transform on verts
        f32 a = 2.0f/windowWidth;
        f32 b = 2.0f/windowHeight;
        
        glm::mat4 clipSpaceTransformMatrix =
        {
            a, 0.0f, 0.0f, 0.0f,
            0.0f, b, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            -1.0f, -1.0f, 0.0f, 1.0f
        };
        
        squareVerts_openGLClipSpace[0] = clipSpaceTransformMatrix * squareVerts_screenCoords[0];
        squareVerts_openGLClipSpace[1] = clipSpaceTransformMatrix * squareVerts_screenCoords[1];
        squareVerts_openGLClipSpace[2] = clipSpaceTransformMatrix * squareVerts_screenCoords[2];
        squareVerts_openGLClipSpace[3] = clipSpaceTransformMatrix * squareVerts_screenCoords[3];
        squareVerts_openGLClipSpace[4] = clipSpaceTransformMatrix * squareVerts_screenCoords[4];
        squareVerts_openGLClipSpace[5] = clipSpaceTransformMatrix * squareVerts_screenCoords[5];
    };
    
    GLfloat verts[] =
    {
        squareVerts_openGLClipSpace[0].x, squareVerts_openGLClipSpace[0].y, squareVerts_openGLClipSpace[0].z,
        1.0f, 0.0f, 0.0f,
        squareVerts_openGLClipSpace[1].x, squareVerts_openGLClipSpace[1].y, squareVerts_openGLClipSpace[1].z,
        0.0f, 1.0f, 0.0f,
        squareVerts_openGLClipSpace[2].x, squareVerts_openGLClipSpace[2].y, squareVerts_openGLClipSpace[2].z,
        1.0f, 0.0f, 0.0f,
        squareVerts_openGLClipSpace[3].x, squareVerts_openGLClipSpace[3].y, squareVerts_openGLClipSpace[3].z,
        1.0f, 0.0f, 0.0f,
        squareVerts_openGLClipSpace[4].x, squareVerts_openGLClipSpace[4].y, squareVerts_openGLClipSpace[4].z,
        0.0f, 1.0f, 0.0f,
        squareVerts_openGLClipSpace[5].x, squareVerts_openGLClipSpace[5].y, squareVerts_openGLClipSpace[5].z,
        1.0f, 0.0f, 0.0f
    };
    
    GLuint bufferID;
    glGenBuffers(1, &bufferID);
    glBindBuffer(GL_ARRAY_BUFFER, bufferID);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 6, 0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 6, (char*)(sizeof(GLfloat)*3));
    
    GLushort indicies[] =
    {
        0, 1, 3,  3, 1, 4,  1, 2, 4,  2, 5, 4
    };
    
    GLuint indexBufferID;
    glGenBuffers(1, &indexBufferID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indicies), indicies, GL_STATIC_DRAW);
    
    glDisable(GL_TEXTURE_2D);
    //glDrawArrays(GL_TRIANGLES, 0, 3);
    glDrawElements(GL_TRIANGLES, 12, GL_UNSIGNED_SHORT, 0);
    glEnable(GL_TEXTURE_2D);
};

void MetersToPixelsProjectionTest(f32 windowWidth, f32 windowHeight)
{
    Array<glm::vec4, 6> squareVerts_meters =
    {
        glm::vec4{2.777f, 3.555f, 5.555f, 1.0f},
        glm::vec4{4.166f, 3.555f, 4.166f, 1.0f},
        glm::vec4{5.555f, 3.555f, 5.555f, 1.0f},
        
        glm::vec4{2.777f, 1.555f, 5.555f, 1.0f},
        glm::vec4{4.166f, 1.555f, 4.166f, 1.0f},
        glm::vec4{5.555f, 1.555f, 5.555f, 1.0f}
    };
    
    {//Projection transform - my version
        f32 camDistanceFromMonitor_z = 11.111f;
        
        for(i32 vertI{}; vertI < 6; ++vertI)
        {
            f32 pointDistanceFromCamera_z = camDistanceFromMonitor_z + squareVerts_meters[vertI].z;
            
            f32 numerator = squareVerts_meters[vertI].x * camDistanceFromMonitor_z;
            squareVerts_meters[vertI].x = numerator / pointDistanceFromCamera_z;
            
            numerator = squareVerts_meters[vertI].y * camDistanceFromMonitor_z;
            squareVerts_meters[vertI].y = numerator / pointDistanceFromCamera_z;
        };
    };
    
    Array<glm::vec4, 6> squareVerts_screenCoords{};
    {//Convert to pixels
        for(i32 vertI{}; vertI < 6; ++vertI)
        {
            squareVerts_screenCoords[vertI].x =  Round(squareVerts_meters[vertI].x * 72);
            squareVerts_screenCoords[vertI].y =  Round(squareVerts_meters[vertI].y * 72);
            squareVerts_screenCoords[vertI].z =  Round(squareVerts_meters[vertI].z * 72);
            squareVerts_screenCoords[vertI].w = 1.0f;
        };
    };
    
    Array<glm::vec4, 6> squareVerts_openGLClipSpace;
    {//Do openGL clip space transform on verts
        f32 b = (f32)(2.0f*windowWidth)/(f32)windowHeight;
        
        glm::mat4 clipSpaceTransformMatrix =
        {
            2.0f, 0.0f, 0.0f, 0.0f,
            0.0f, b, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            -1.0f, 0.0f, 0.0f, 1.0f
        };
        
        squareVerts_openGLClipSpace[0] = clipSpaceTransformMatrix * squareVerts_screenCoords[0];
        squareVerts_openGLClipSpace[1] = clipSpaceTransformMatrix * squareVerts_screenCoords[1];
        squareVerts_openGLClipSpace[2] = clipSpaceTransformMatrix * squareVerts_screenCoords[2];
        squareVerts_openGLClipSpace[3] = clipSpaceTransformMatrix * squareVerts_screenCoords[3];
        squareVerts_openGLClipSpace[4] = clipSpaceTransformMatrix * squareVerts_screenCoords[4];
        squareVerts_openGLClipSpace[5] = clipSpaceTransformMatrix * squareVerts_screenCoords[5];
    };
    
    GLfloat verts[] =
    {
        squareVerts_openGLClipSpace[0].x, squareVerts_openGLClipSpace[0].y, squareVerts_openGLClipSpace[0].z,
        1.0f, 0.0f, 0.0f,
        squareVerts_openGLClipSpace[1].x, squareVerts_openGLClipSpace[1].y, squareVerts_openGLClipSpace[1].z,
        0.0f, 1.0f, 0.0f,
        squareVerts_openGLClipSpace[2].x, squareVerts_openGLClipSpace[2].y, squareVerts_openGLClipSpace[2].z,
        1.0f, 0.0f, 0.0f,
        squareVerts_openGLClipSpace[3].x, squareVerts_openGLClipSpace[3].y, squareVerts_openGLClipSpace[3].z,
        1.0f, 0.0f, 0.0f,
        squareVerts_openGLClipSpace[4].x, squareVerts_openGLClipSpace[4].y, squareVerts_openGLClipSpace[4].z,
        0.0f, 1.0f, 0.0f,
        squareVerts_openGLClipSpace[5].x, squareVerts_openGLClipSpace[5].y, squareVerts_openGLClipSpace[5].z,
        1.0f, 0.0f, 0.0f
    };
    
    GLuint bufferID;
    glGenBuffers(1, &bufferID);
    glBindBuffer(GL_ARRAY_BUFFER, bufferID);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 6, 0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 6, (char*)(sizeof(GLfloat)*3));
    
    GLushort indicies[] =
    {
        0, 1, 3,  3, 1, 4,  1, 2, 4,  2, 5, 4
    };
    
    GLuint indexBufferID;
    glGenBuffers(1, &indexBufferID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indicies), indicies, GL_STATIC_DRAW);
    
    glDisable(GL_TEXTURE_2D);
    //glDrawArrays(GL_TRIANGLES, 0, 3);
    glDrawElements(GL_TRIANGLES, 12, GL_UNSIGNED_SHORT, 0);
    glEnable(GL_TEXTURE_2D);
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
    v2f screenSize_meters = CastV2IToV2F(screenSize) / pixelsPerMeter;
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
                
                v4f color = { rectEntry.color.r, rectEntry.color.g, rectEntry.color.b, 1.0f };
                
                glDisable(GL_TEXTURE_2D);
                DrawQuad(targetRect_screen, color);
                glEnable(GL_TEXTURE_2D);
                
                currentRenderBufferEntry += sizeof(RenderEntry_Rect);
            }
            break;
            
            case EntryType_Line: {
                RenderEntry_Line lineEntry = *(RenderEntry_Line*)currentRenderBufferEntry;
                
                v2f lineMinPoint_camera = CameraTransform(lineEntry.minPoint, *camera);
                v2f lineMaxPoint_camera = CameraTransform(lineEntry.maxPoint, *camera);
                
                v2f lineMinPoint_screen = ProjectionTransform_Ortho(lineMinPoint_camera, pixelsPerMeter);
                v2f lineMaxPoint_screen = ProjectionTransform_Ortho(lineMaxPoint_camera, pixelsPerMeter);
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
                
                
#if 0
                {//Projection transform - glm version
                    glm::mat4 projMatrix = glm::perspective(.76f, 16.0f/9.0f, .1f, 100.0f);
                    
                    for(i32 vertI{}; vertI < 4; ++vertI)
                        squareVerts_screenCoords[vertI] = projMatrix * squareVerts_screenCoords[vertI];
                };
#endif
                
                //PixelUnitProjectionTest((f32)windowWidth, (f32)windowHeight);
                MetersToPixelsProjectionTest((f32)windowWidth, (f32)windowHeight);
                
                currentRenderBufferEntry += sizeof(RenderEntry_Test);
            }break;
            
            InvalidDefaultCase;
        };
    }
    
    renderingInfo.cmdBuffer.entryCount = 0;
};

#endif //OPENGL_INCLUDE_H
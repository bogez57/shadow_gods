#ifndef OPENGL_INCLUDE_H
#define OPENGL_INCLUDE_H

#include "renderer_stuff.h"
#include "memory_handling.h"

const char* vertexShaderCode =
R"HereDoc(

#version 430

in layout(location=0) vec3 position;
in layout(location=1) vec3 color;
out vec3 fragColor;

uniform mat4 transformationMatrix;

void main()
{
vec4 newPos = vec4(position, 1.0) * transformationMatrix;//vector is on the left side because my matrices are row major
gl_Position = newPos;

    vec3 changedColors;
    changedColors.r += color.r + 0;
    changedColors.g += color.g + 0;
    changedColors.b += color.b + 0;
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

void GLAPIENTRY MyOpenGLErrorCallbackFunc(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
{
    BGZ_CONSOLE("%s type=0x%x %s\n", ( type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "" ), type, message);
#if _MSC_VER
    __debugbreak();
#endif
};

local_func u32
LoadTexture(u8* textureData, v2i textureSize)
{
    u32 textureID {};
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
DrawTexture(u32 TextureID, Quadf textureCoords, v2 MinUV, v2 MaxUV)
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
GLInit(int windowWidth, int windowHeight)
{
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(MyOpenGLErrorCallbackFunc, 0);
    
    glViewport(0, 0, windowWidth, windowHeight);
    
    //If this is set to GL_MODULATE instead then you might get unwanted texture coloring.
    //In order to avoid that in GL_MODULATE mode you need to constantly set glcolor to white after drawing.
    //For more info: https://stackoverflow.com/questions/53180760/all-texture-colors-affected-by-colored-rectangle-opengl
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);//Defaults to CCW ordering of indicies meaning all indicies that, from the viewers perspective, creating triangles in a CW manner repsrent visible triangles.
    glCullFace(GL_BACK);//Culls only back faces (faces facing away from viewer)
    InstallShaders();
}


void Draw(Memory_Partition* memPart, RunTimeArr<v3> meshVerts_objectSpace, RunTimeArr<s16> meshIndicies)
{
    RunTimeArr<GLfloat> verts{};
    InitArr($(verts), memPart, meshVerts_objectSpace.length * 6);
    s32 i{};
    f32 colorR{1.0f}, colorG{}, colorB{};
    for(s32 j{}; j < meshVerts_objectSpace.length; ++j)
    {
        verts.Push(meshVerts_objectSpace[j].x);
        verts.Push(meshVerts_objectSpace[j].y);
        verts.Push(meshVerts_objectSpace[j].z);
        verts.Push(colorR);
        verts.Push(colorG);
        verts.Push(colorB);
    };
    
    GLuint bufferID;
    glGenBuffers(1, &bufferID);
    glBindBuffer(GL_ARRAY_BUFFER, bufferID);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * verts.length, verts.elements, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 6, 0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 6, (char*)(sizeof(GLfloat)*3));
    
    GLuint indexBufferID;
    glGenBuffers(1, &indexBufferID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(s16) * meshIndicies.length, meshIndicies.elements, GL_STATIC_DRAW);
    
    glDisable(GL_TEXTURE_2D);
    glDrawElements(GL_TRIANGLES, (s32)meshIndicies.length, GL_UNSIGNED_SHORT, 0);
    glEnable(GL_TEXTURE_2D);
};

void RenderViaHardware(Rendering_Info&& renderingInfo, Memory_Partition* platformMemoryPart, int windowWidth, int windowHeight)
{
    local_persist bool glIsInitialized { false };
    if (NOT glIsInitialized)
    {
        GLInit(windowWidth, windowHeight);
        glClearColor(0.0f, 0.0f, 1.0f, 0.0f);
        glIsInitialized = true;
    };
    
    u8* currentRenderBufferEntry = renderingInfo.cmdBuffer.baseAddress;
    Camera2D* camera2d = &renderingInfo.camera2d;
    Camera3D camera3d = renderingInfo.camera3d;
    
    f32 pixelsPerMeter = renderingInfo._pixelsPerMeter;
    v2i screenSize = { windowWidth, windowHeight };
    v2 screenSize_meters = CastV2IToV2F(screenSize) / pixelsPerMeter;
    camera2d->dilatePoint_inScreenCoords = (screenSize_meters / 2.0f) + (Hadamard(screenSize_meters, camera2d->dilatePointOffset_normalized));
    
    camera2d->viewCenter = screenSize_meters / 2.0f;
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glEnable(GL_TEXTURE_2D);
    
    for (s32 entryNumber = 0; entryNumber < renderingInfo.cmdBuffer.entryCount; ++entryNumber)
    {
        RenderEntry_Header* entryHeader = (RenderEntry_Header*)currentRenderBufferEntry;
        switch (entryHeader->type)
        {
            case EntryType_Texture: {
                RenderEntry_Texture textureEntry = *(RenderEntry_Texture*)currentRenderBufferEntry;
                
                local_persist u32 textureID{};
                if (NOT textureEntry.isLoadedOnGPU)
                {
                    textureID = LoadTexture(textureEntry.colorData, v2i { textureEntry.size.width, textureEntry.size.height });
                };
                
                glBindTexture(GL_TEXTURE_2D, textureID);
                
                Quadf imageTargetRect_camera = CameraTransform(textureEntry.targetRect_worldCoords, *camera2d);
                Quadf imageTargetRect_screen = ProjectionTransform_Ortho(imageTargetRect_camera, pixelsPerMeter);
                
                DrawTexture(textureID, imageTargetRect_screen, textureEntry.uvBounds[0], textureEntry.uvBounds[1]);
                
                currentRenderBufferEntry += sizeof(RenderEntry_Texture);
            }break;
            
            case EntryType_Rect: {
                RenderEntry_Rect rectEntry = *(RenderEntry_Rect*)currentRenderBufferEntry;
                
                Quadf targetRect_camera = CameraTransform(rectEntry.worldCoords, *camera2d);
                Quadf targetRect_screen = ProjectionTransform_Ortho(targetRect_camera, pixelsPerMeter);
                
                v4 color = { rectEntry.color, 1.0f };
                
                glDisable(GL_TEXTURE_2D);
                DrawQuad(targetRect_screen, color);
                glEnable(GL_TEXTURE_2D);
                
                currentRenderBufferEntry += sizeof(RenderEntry_Rect);
            }break;
            
            case EntryType_Line: {
                RenderEntry_Line lineEntry = *(RenderEntry_Line*)currentRenderBufferEntry;
                
                v2 lineMinPoint_camera = CameraTransform(lineEntry.minPoint, *camera2d);
                v2 lineMaxPoint_camera = CameraTransform(lineEntry.maxPoint, *camera2d);
                
                v2 lineMinPoint_screen = ProjectionTransform_Ortho(lineMinPoint_camera, pixelsPerMeter);
                v2 lineMaxPoint_screen = ProjectionTransform_Ortho(lineMaxPoint_camera, pixelsPerMeter);
                lineEntry.minPoint = lineMinPoint_screen;
                lineEntry.maxPoint = lineMaxPoint_screen;
                
                glDisable(GL_TEXTURE_2D);
                DrawLine(lineEntry.minPoint, lineEntry.maxPoint, lineEntry.color, lineEntry.thickness);
                glEnable(GL_TEXTURE_2D);
                
                currentRenderBufferEntry += sizeof(RenderEntry_Line);
            }break;
            
            case EntryType_Geometry: {
                ScopedMemory scope{platformMemoryPart};
                
                RenderEntry_Geometry geometryEntry = *(RenderEntry_Geometry*)currentRenderBufferEntry;
                
                //camera transform
                Mat4x4 xRotMatrix = XRotation(camera3d.rotation.x);
                Mat4x4 yRotMatrix = YRotation(camera3d.rotation.y);
                Mat4x4 zRotMatrix = ZRotation(camera3d.rotation.z);
                Mat4x4 fullRotMatrix = xRotMatrix * yRotMatrix * zRotMatrix;
                v3 xAxis = GetColumn(fullRotMatrix, 0);
                v3 yAxis = GetColumn(fullRotMatrix, 1);
                v3 zAxis = GetColumn(fullRotMatrix, 2);
                Mat4x4 camTransform = ProduceCameraTransformMatrix(xAxis, yAxis, zAxis, camera3d.worldPos);
                
                Mat4x4 projectionTransform = ProduceProjectionTransformMatrix_UsingFOV(renderingInfo.fov, renderingInfo.aspectRatio, renderingInfo.nearPlane, renderingInfo.farPlane);
                
                Mat4x4 fullTransformMatrix = projectionTransform * camTransform * geometryEntry.worldTransform;
                
                GLint transformMatrixUniformLocation = glGetUniformLocation(3, "transformationMatrix");
                glUniformMatrix4fv(transformMatrixUniformLocation, 1, GL_FALSE, &fullTransformMatrix.elem[0][0]);
                
                Draw(platformMemoryPart, geometryEntry.verts, geometryEntry.indicies);
                
                currentRenderBufferEntry += sizeof(RenderEntry_Geometry);
            }break;
            
            InvalidDefaultCase;
        };
    }
    
    renderingInfo.cmdBuffer.entryCount = 0;
};

#endif //OPENGL_INCLUDE_H

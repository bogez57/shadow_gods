#ifndef OPENGL_INCLUDE_H
#define OPENGL_INCLUDE_H

#include "renderer_stuff.h"
#include "memory_handling.h"

const char* vertexShaderCode =
R"HereDoc(

#version 430

in layout(location=0) vec3 position;
in layout(location=1) vec2 texCoord;
in layout(location=2) vec3 normals;

out vec2 fragTexCoord;

uniform mat4 transformationMatrix;

void main()
{
vec4 newPos = vec4(position, 1.0) * transformationMatrix;//vector is on the left side because my matrices are row major

gl_Position = newPos;
fragTexCoord = texCoord;
};

)HereDoc";

const char* fragmentShaderCode =
R"HereDoc(

#version 430

out vec4 color;
in vec2 fragTexCoord;

uniform sampler2D id;

void main()
{
vec4 texColor = texture(id, fragTexCoord);
    color = texColor;
};

)HereDoc";

void GLAPIENTRY MyOpenGLErrorCallbackFunc(GLenum source, GLenum debugErrorType, GLuint errorID, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
{
    switch(debugErrorType)
    {
        case GL_DEBUG_TYPE_ERROR:
        {
            BGZ_CONSOLE("GL Type error: %s\nGL error id: 0x%x\n", message, errorID);
#if _MSC_VER
            __debugbreak();
#endif
        }break;
        
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        {
            BGZ_CONSOLE("GL deprecated gl function usage error: %s\nGL error id: 0x%x\n", message, errorID);
#if _MSC_VER
            __debugbreak();
#endif
        }break;
        
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        {
            BGZ_CONSOLE("GL undefined behavior error: %s\nGL error id: 0x%x\n", message, errorID);
            
#if _MSC_VER
            __debugbreak();
#endif
        }break;
        
        case GL_DEBUG_TYPE_PERFORMANCE:
        {
            BGZ_CONSOLE("GL performance warning/error: %s\nGL error id: 0x%x\n", message, errorID);
            
        }break;
        
        case GL_DEBUG_TYPE_PORTABILITY:
        {
            BGZ_CONSOLE("GL portability warning/error: %s\nGL error id: 0x%x\n", message, errorID);
            
        }break;
        
        case GL_DEBUG_TYPE_OTHER:
        {
            if(errorID == 0x20071)//Ignores the warning: Buffer object 1 (bound to WHATEVER_BUFFER, usage hint is GL_STATIC_DRAW) will use VIDEO memory.... Apparently this doesn't mean much
            {
                //Ignore
            }
            else
            {
                BGZ_CONSOLE("GL other error: %s\nGL error id: 0x%x\n", message, errorID);
            };
        }break;
    };
};

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
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);//This makes things so debug callback function doesn't get called from a thread other than which one I need to view the call stack in debugger
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

//TODO: enable texture drawing
void Draw(Memory_Partition* memPart, u32 id, s32 textureID, RunTimeArr<s16> meshIndicies)
{
    glBindVertexArray(id);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glDrawElements(GL_TRIANGLES, (s32)meshIndicies.length, GL_UNSIGNED_SHORT, 0);
    glBindVertexArray(0);
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
            case EntryType_InitVertexBuffer:{
                RenderEntry_InitVertexBuffer bufferData = *(RenderEntry_InitVertexBuffer*)currentRenderBufferEntry;
                
                ScopedMemory scope{platformMemoryPart};
                
                u32 vertexArrayID{};
                glGenVertexArrays(1, &vertexArrayID);
                glBindVertexArray(vertexArrayID);
                
                GLuint bufferID;
                glGenBuffers(1, &bufferID);
                glBindBuffer(GL_ARRAY_BUFFER, bufferID);
                glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * bufferData.vertAttribs.length, bufferData.vertAttribs.elements, GL_DYNAMIC_DRAW);
                glEnableVertexAttribArray(0);
                glEnableVertexAttribArray(1);
                glEnableVertexAttribArray(2);
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 8, 0);//position
                glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 8, (char*)(sizeof(GLfloat)*3));//tex coords
                glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 8, (char*)(sizeof(GLfloat)*5));//normals
                
                GLuint indexBufferID;
                glGenBuffers(1, &indexBufferID);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(s16) * bufferData.indicies.length, bufferData.indicies.elements, GL_DYNAMIC_DRAW);
                
                currentRenderBufferEntry += sizeof(RenderEntry_InitVertexBuffer);
            }break;
            
            case EntryType_LoadTexture: {
                RenderEntry_LoadTexture loadTexEntry = *(RenderEntry_LoadTexture*)currentRenderBufferEntry;
                
                u32 textureID {};
                glEnable(GL_TEXTURE_2D);
                glGenTextures(1, &textureID);
                glBindTexture(GL_TEXTURE_2D, textureID);
                
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, loadTexEntry.texture.width_pxls, loadTexEntry.texture.height_pxls, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, loadTexEntry.texture.data);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                
                //Enable alpha channel for transparency
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glBindTexture(GL_TEXTURE_2D, 0);
                
                currentRenderBufferEntry += sizeof(RenderEntry_LoadTexture);
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
                
                //camera transform setup
                Mat4x4 xRotMatrix = XRotation(camera3d.rotation.x);
                Mat4x4 yRotMatrix = YRotation(camera3d.rotation.y);
                Mat4x4 zRotMatrix = ZRotation(camera3d.rotation.z);
                Mat4x4 fullRotMatrix = xRotMatrix * yRotMatrix * zRotMatrix;
                v3 xAxis = GetColumn(fullRotMatrix, 0);
                v3 yAxis = GetColumn(fullRotMatrix, 1);
                v3 zAxis = GetColumn(fullRotMatrix, 2);
                
                //Setup full transform matrix
                Mat4x4 camTransform = ProduceCameraTransformMatrix(xAxis, yAxis, zAxis, camera3d.worldPos);
                Mat4x4 projectionTransform = ProduceProjectionTransformMatrix_UsingFOV(renderingInfo.fov, renderingInfo.aspectRatio, renderingInfo.nearPlane, renderingInfo.farPlane);
                Mat4x4 fullTransformMatrix = projectionTransform * camTransform * geometryEntry.worldTransform;
                
                //Send transform matrix to vertex shader
                GLint transformMatrixUniformLocation = glGetUniformLocation(3, "transformationMatrix");
                glUniformMatrix4fv(transformMatrixUniformLocation, 1, GL_FALSE, &fullTransformMatrix.elem[0][0]);
                
                Draw(platformMemoryPart, geometryEntry.meshID, geometryEntry.textureID, geometryEntry.indicies);
                
                currentRenderBufferEntry += sizeof(RenderEntry_Geometry);
            }break;
            
            InvalidDefaultCase;
        };
    }
    
    renderingInfo.cmdBuffer.entryCount = 0;
};

#endif //OPENGL_INCLUDE_H

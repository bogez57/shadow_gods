#ifndef OPENGL_INCLUDE_H
#define OPENGL_INCLUDE_H

#include "renderer_stuff.h"

const char* vertexShaderCode =
"#version 430 \r\n"
"in layout(location=0) vec4 position;\n"
"in layout(location=1) vec3 color;\n"
"out vec3 fragColor;\n"
"\n"
"void main()\n"
"{\n"
"    gl_Position = position; \n"
"    vec3 changedColors;\n"
"    changedColors.r += color.r + .01;\n"
"    changedColors.g += color.g + .04;\n"
"    changedColors.b += color.b + .02;\n"
"    fragColor = changedColors;\n"
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

v2f ParentTransform_1Vector(v2f localCoords, Transform parentTransform)
{
    ConvertToCorrectPositiveRadian($(parentTransform.rotation));
    
    Coordinate_Space parentSpace {};
    parentSpace.origin = parentTransform.translation;
    parentSpace.xBasis = v2f { CosR(parentTransform.rotation), SinR(parentTransform.rotation) };
    parentSpace.yBasis = parentTransform.scale.y * PerpendicularOp(parentSpace.xBasis);
    parentSpace.xBasis *= parentTransform.scale.x;
    
    v2f transformedCoords {};
    
    //This equation rotates first then moves to correct world position
    transformedCoords = parentSpace.origin + (localCoords.x * parentSpace.xBasis) + (localCoords.y * parentSpace.yBasis);
    
    return transformedCoords;
};

struct Transform_v4
{
    v4f translation{};
    v3f rotation{};
};

v4f ParentTransform_1Vec(v4f localCoords, Transform_v4 parentTransform);

void ProjectionTestUsingFocalLength_InMeters(f32 windowWidth, f32 windowHeight)
{
#if 0
    Array<v4f, 6> squareVerts_camera =
    {
        v4f{1.6f, 1.4f, 4.0f, 1.0f},
        v4f{2.6f, 1.4f, 3.0f, 1.0f},
        v4f{4.6f, 1.4f, 4.0f, 1.0f},
        
        v4f{1.6f, -0.6f, 4.0f, 1.0f},
        v4f{2.6f, -0.6f, 3.0f, 1.0f},
        v4f{4.6f, -0.6f, 4.0f, 1.0f}
    };
#endif
    
    Array<v4f, 6> squareVerts_object =
    {
        v4f{8.0f, -1.0f, -0.5f, 1.0f},
        v4f{9.0f, -1.0f, 0.5f, 1.0f},
        v4f{11.0f, -1.0f, -0.5f, 1.0f},
        
        v4f{8.0f, 1.0f, -0.5f, 1.0f},
        v4f{9.0f, 1.0f, 0.5f, 1.0f},
        v4f{11.0f, 1.0f, -0.5f, 1.0f}
    };
    
    local_persist v3f rotation { 0.0f, 0.0f, 0.0f };
    Transform_v4 world { v4f{ 0.0f, 3.0f, 6.0f, 1.0f}, rotation};
    
    Array<v4f, 6> squareVerts_world{};
    {//World Transform
        for(i32 i{}; i < 6; ++i)
        {
            squareVerts_world[i] = ParentTransform_1Vec(squareVerts_object[i], world);
        };
    };
    
    Array<v4f, 6> squareVerts_camera{};
    {//Camera Transform
        for(i32 i{}; i < 6; ++i)
        {
            squareVerts_camera[i].x = squareVerts_world[i].x - 6.4f;
            squareVerts_camera[i].y = squareVerts_world[i].y - 3.6f;
            squareVerts_camera[i].z = squareVerts_world[i].z;
            squareVerts_camera[i].w = squareVerts_world[i].w;
        };
    };
    
    Array<v4f, 6> squareVerts_openGLClipSpace;
    {//Projection transform
        f32 focalLength = 1.8f;
        f32 windowWidth_meters = windowWidth;
        f32 windowHeight_meters = windowHeight;
        
        for(i32 vertI{}; vertI < 6; ++vertI)
        {
            squareVerts_openGLClipSpace[vertI].x = (squareVerts_camera[vertI].x * focalLength) / (windowWidth_meters / 2.0f);
            squareVerts_openGLClipSpace[vertI].y = (squareVerts_camera[vertI].y * focalLength) / (windowHeight_meters / 2.0f);
            squareVerts_openGLClipSpace[vertI].z = 1.0f;
            squareVerts_openGLClipSpace[vertI].w = squareVerts_camera[vertI].z;
        };
    };
    
    GLfloat verts[] =
    {
        squareVerts_openGLClipSpace[0].x, squareVerts_openGLClipSpace[0].y, squareVerts_openGLClipSpace[0].z, squareVerts_openGLClipSpace[0].w,
        1.0f, 0.0f, 0.0f,
        squareVerts_openGLClipSpace[1].x, squareVerts_openGLClipSpace[1].y, squareVerts_openGLClipSpace[1].z, squareVerts_openGLClipSpace[1].w,
        0.0f, 1.0f, 0.0f,
        squareVerts_openGLClipSpace[2].x, squareVerts_openGLClipSpace[2].y, squareVerts_openGLClipSpace[2].z, squareVerts_openGLClipSpace[2].w,
        1.0f, 0.0f, 0.0f,
        squareVerts_openGLClipSpace[3].x, squareVerts_openGLClipSpace[3].y, squareVerts_openGLClipSpace[3].z, squareVerts_openGLClipSpace[3].w,
        1.0f, 0.0f, 0.0f,
        squareVerts_openGLClipSpace[4].x, squareVerts_openGLClipSpace[4].y, squareVerts_openGLClipSpace[4].z, squareVerts_openGLClipSpace[4].w,
        0.0f, 1.0f, 0.0f,
        squareVerts_openGLClipSpace[5].x, squareVerts_openGLClipSpace[5].y, squareVerts_openGLClipSpace[5].z, squareVerts_openGLClipSpace[5].w,
        1.0f, 0.0f, 0.0f
    };
    
    GLuint bufferID;
    glGenBuffers(1, &bufferID);
    glBindBuffer(GL_ARRAY_BUFFER, bufferID);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 7, 0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 7, (char*)(sizeof(GLfloat)*3));
    
    GLushort indicies[] =
    {
        0, 1, 3,  3, 4, 1,  1, 4, 2,  2, 5, 4
    };
    
    GLuint indexBufferID;
    glGenBuffers(1, &indexBufferID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indicies), indicies, GL_STATIC_DRAW);
    
    glDisable(GL_TEXTURE_2D);
    glDrawElements(GL_TRIANGLES, 12, GL_UNSIGNED_SHORT, 0);
    glEnable(GL_TEXTURE_2D);
};

void ProjectionTestUsingFOV_InMeters(f32 windowWidth, f32 windowHeight)
{
#if 0
    Array<v4f, 6> squareVerts_camera =
    {
        v4f{1.6f, 1.4f, 4.0f, 1.0f},
        v4f{2.6f, 1.4f, 3.0f, 1.0f},
        v4f{4.6f, 1.4f, 4.0f, 1.0f},
        
        v4f{1.6f, -0.6f, 4.0f, 1.0f},
        v4f{2.6f, -0.6f, 3.0f, 1.0f},
        v4f{4.6f, -0.6f, 4.0f, 1.0f}
    };
#endif
    
    Array<v4f, 6> squareVerts_object =
    {
        v4f{8.0f, 2.0f, 1.0f, 1.0f},
        v4f{9.0f, 2.0f, 0.0f, 1.0f},
        v4f{11.0f, 2.0f, 1.0f, 1.0f},
        
        v4f{8.0f, 0.0f, 1.0f, 1.0f},
        v4f{9.0f, 0.0f, 0.0f, 1.0f},
        v4f{11.0f, 0.0f, 1.0f, 1.0f}
    };
    
    local_persist v3f rotation = { 0.0f, 0.0f, 0.0f };
    Transform_v4 world { v4f{ 0.0f, 3.0f, 6.0f, 1.0f}, rotation};
    
    Array<v4f, 6> squareVerts_world{};
    {//World Transform
        for(i32 i{}; i < 6; ++i)
        {
            squareVerts_world[i] = ParentTransform_1Vec(squareVerts_object[i], world);
        };
    };
    
    Array<v4f, 6> squareVerts_camera{};
    {//Camera Transform
        for(i32 i{}; i < 6; ++i)
        {
            squareVerts_camera[i].x = squareVerts_world[i].x - 6.4f;
            squareVerts_camera[i].y = squareVerts_world[i].y - 3.6f;
            squareVerts_camera[i].z = squareVerts_world[i].z;
            squareVerts_camera[i].w = squareVerts_world[i].w;
        };
    };
    
    BGZ_CONSOLE("z: %f\n", squareVerts_camera[0].z);
    
    Array<glm::vec4, 6> squareVerts_openGLClipSpace;
#if 1
    {//Projection transform
        f32 fov = glm::radians(140.0f);
        f32 aspectRatio = 16.0f/9.0f;
        f32 tanHalfFov = TanR(fov / 2.0f);
        f32 xScale = 1.0f / (tanHalfFov * aspectRatio);
        f32 yScale = 1.0f / tanHalfFov;
        
        f32 farClip = 100.0f;
        f32 nearClip = 1.0f;
        
        f32 a = (-farClip - nearClip) / (nearClip - farClip);
        f32 b = (2.0f * farClip * nearClip) / (nearClip - farClip);
        
        for(i32 vertI{}; vertI < 6; ++vertI)
        {
            squareVerts_openGLClipSpace[vertI].x = squareVerts_camera[vertI].x * xScale;
            squareVerts_openGLClipSpace[vertI].y = squareVerts_camera[vertI].y * yScale;
            squareVerts_openGLClipSpace[vertI].z = squareVerts_camera[vertI].z * a + b;
            squareVerts_openGLClipSpace[vertI].w = squareVerts_camera[vertI].z;
        };
    };
    
#else
    {//Do openGL clip space transform on verts
        glm::mat4 proj = glm::perspective(glm::radians(80.0f), 16.0f/9.0f, .1f, 100.0f);
        
        for(i32 vertI{}; vertI < 6; ++vertI)
        {
            squareVerts_openGLClipSpace[vertI] = proj * squareVerts_camera[vertI];
            squareVerts_openGLClipSpace[vertI].z = 1.0f;
            squareVerts_openGLClipSpace[vertI].w *= -1.0f;
        };
    };
#endif
    
    GLfloat verts[] =
    {
        squareVerts_openGLClipSpace[0].x, squareVerts_openGLClipSpace[0].y, squareVerts_openGLClipSpace[0].z, squareVerts_openGLClipSpace[0].w,
        1.0f, 0.0f, 0.0f,
        squareVerts_openGLClipSpace[1].x, squareVerts_openGLClipSpace[1].y, squareVerts_openGLClipSpace[1].z, squareVerts_openGLClipSpace[1].w,
        0.0f, 1.0f, 0.0f,
        squareVerts_openGLClipSpace[2].x, squareVerts_openGLClipSpace[2].y, squareVerts_openGLClipSpace[2].z, squareVerts_openGLClipSpace[2].w,
        1.0f, 0.0f, 0.0f,
        squareVerts_openGLClipSpace[3].x, squareVerts_openGLClipSpace[3].y, squareVerts_openGLClipSpace[3].z, squareVerts_openGLClipSpace[3].w,
        1.0f, 0.0f, 0.0f,
        squareVerts_openGLClipSpace[4].x, squareVerts_openGLClipSpace[4].y, squareVerts_openGLClipSpace[4].z, squareVerts_openGLClipSpace[4].w,
        0.0f, 1.0f, 0.0f,
        squareVerts_openGLClipSpace[5].x, squareVerts_openGLClipSpace[5].y, squareVerts_openGLClipSpace[5].z, squareVerts_openGLClipSpace[5].w,
        1.0f, 0.0f, 0.0f
    };
    
    GLuint bufferID;
    glGenBuffers(1, &bufferID);
    glBindBuffer(GL_ARRAY_BUFFER, bufferID);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 7, 0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 7, (char*)(sizeof(GLfloat)*3));
    
    GLushort indicies[] =
    {
        0, 1, 3,  3, 1, 4,  1, 2, 4,  2, 5, 4
    };
    
    GLuint indexBufferID;
    glGenBuffers(1, &indexBufferID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indicies), indicies, GL_STATIC_DRAW);
    
    glDisable(GL_TEXTURE_2D);
    glDrawElements(GL_TRIANGLES, 12, GL_UNSIGNED_SHORT, 0);
    glEnable(GL_TEXTURE_2D);
};

v4f ParentTransform_1Vec(v4f localCoords, Transform_v4 parentTransform)
{
    BGZ_ASSERT(parentTransform.translation.w == 1.0f, "Function expects w to be 1.0");
    
    ConvertToCorrectPositiveRadian($(parentTransform.rotation.x));
    ConvertToCorrectPositiveRadian($(parentTransform.rotation.y));
    ConvertToCorrectPositiveRadian($(parentTransform.rotation.z));
    
    v4f origin_inParentSpace = parentTransform.translation;
    v3f newRotatedPoint{};
    
    //X axis rotation
    v3f zBasis_xAxisRotation = v3f { 0.0f, SinR(parentTransform.rotation.x), CosR(parentTransform.rotation.x) };
    v3f yBasis_xAxisRotation = v3f { 0.0f, CosR(parentTransform.rotation.x), -SinR(parentTransform.rotation.x) };
    newRotatedPoint.yz = (localCoords.z * zBasis_xAxisRotation.yz) + (localCoords.y * yBasis_xAxisRotation.yz);
    
    //Z axis rotation
    v3f xBasis_zAxisRotation = v3f { CosR(parentTransform.rotation.z), SinR(parentTransform.rotation.z), 0.0f };
    v3f yBasis_zAxisRotation = v3f{ -SinR(parentTransform.rotation.z), CosR(parentTransform.rotation.z), 0.0f };
    newRotatedPoint.xy = (localCoords.x * xBasis_zAxisRotation.xy) + (newRotatedPoint.y * yBasis_zAxisRotation.xy);
    
    //Y axis rotation
    f32 rotatedY = newRotatedPoint.y;
    v3f xBasis_yAxisRotation = v3f { CosR(parentTransform.rotation.y), 0.0f, SinR(parentTransform.rotation.y) };
    v3f zBasis_yAxisRotation = v3f { -SinR(parentTransform.rotation.y), 0.0f, CosR(parentTransform.rotation.y) };
    newRotatedPoint = (newRotatedPoint.x * xBasis_yAxisRotation) + (newRotatedPoint.z * zBasis_yAxisRotation);
    newRotatedPoint.y = rotatedY;
    
    v4f transformedCoord { newRotatedPoint.x, newRotatedPoint.y, newRotatedPoint.z, 0.0f };
    transformedCoord = origin_inParentSpace + transformedCoord;
    
    return transformedCoord;
};

#define NUM_VERTS 8
Array<v4f, NUM_VERTS> squareVerts_object =
{
    v4f{-0.5f, 0.5f, -0.5f, 1.0f}, //0
    v4f{+0.5f, 0.5f, -0.5f, 1.0f}, //1
    v4f{-0.5f, -0.5f, -0.5f, 1.0f},//2
    v4f{+0.5f, -0.5f, -0.5f, 1.0f},//3
    v4f{+0.5f, -0.5f, +0.5f, 1.0f},//4
    v4f{+0.5f, +0.5f, +0.5f, 1.0f},//5
    v4f{-0.5f, +0.5f, +0.5f, 1.0f},//6
    v4f{-0.5f, -0.5f, +0.5f, 1.0f},//7
    
};

GLushort indicies[] =
{
    1, 0, 2,  1, 2, 3,//Front
    4, 1, 3,  5, 1, 4, //Right side
    4, 5, 7,  7, 5, 6, //Back side
    6, 7, 0,  0, 7, 2 //Left side
};

f32 epsilon = 0.00001f; //TODO: Remove????

void ProjectionTestUsingFullSquare(f32 windowWidth, f32 windowHeight)
{
    local_persist v3f rotation = { 0.0f, 0.0f, 0.0f } ;
    Transform_v4 world { v4f{ 6.4f, 3.0f, 6.0f, 1.0f}, rotation};
    
    Array<v4f, NUM_VERTS> squareVerts_world{};
    {//World Transform
        for(i32 i{}; i < NUM_VERTS; ++i)
        {
            squareVerts_world[i] = ParentTransform_1Vec(squareVerts_object[i], world);
        };
    };
    
    Array<v4f, NUM_VERTS> squareVerts_camera{};
    {//Camera Transform
        for(i32 i{}; i < NUM_VERTS; ++i)
        {
            squareVerts_camera[i].x = squareVerts_world[i].x - 6.4f;
            squareVerts_camera[i].y = squareVerts_world[i].y - 3.6f;
            squareVerts_camera[i].z = squareVerts_world[i].z;
            squareVerts_camera[i].w = squareVerts_world[i].w;
        };
    };
    
    BGZ_CONSOLE("z: %f\n", squareVerts_camera[0].z);
    
    Array<glm::vec4, NUM_VERTS> squareVerts_openGLClipSpace;
#if 1
    {//Projection transform
        f32 fov = glm::radians(80.0f);
        f32 aspectRatio = 16.0f/9.0f;
        f32 tanHalfFov = TanR(fov / 2.0f);
        f32 xScale = 1.0f / (tanHalfFov * aspectRatio);
        f32 yScale = 1.0f / tanHalfFov;
        
        f32 farClip = 100.0f;
        f32 nearClip = 1.0f;
        
        f32 a = (-farClip - nearClip) / (nearClip - farClip);
        f32 b = (2.0f * farClip * nearClip) / (nearClip - farClip);
        
        for(i32 vertI{}; vertI < NUM_VERTS; ++vertI)
        {
            squareVerts_openGLClipSpace[vertI].x = squareVerts_camera[vertI].x * xScale;
            squareVerts_openGLClipSpace[vertI].y = squareVerts_camera[vertI].y * yScale;
            squareVerts_openGLClipSpace[vertI].z = squareVerts_camera[vertI].z * a + b;
            squareVerts_openGLClipSpace[vertI].w = squareVerts_camera[vertI].z;
            
            f32 test = squareVerts_openGLClipSpace[vertI].z/squareVerts_openGLClipSpace[vertI].w;
        };
    };
    
#else
    {//Do openGL clip space transform on verts
        glm::mat4 proj = glm::perspective(glm::radians(120.0f), 16.0f/9.0f, 1.0f, 100.0f);
        
        for(i32 vertI{}; vertI < NUM_VERTS; ++vertI)
        {
            squareVerts_openGLClipSpace[vertI] = proj * squareVerts_meters[vertI];
            squareVerts_openGLClipSpace[vertI].z = 1.0f;
            squareVerts_openGLClipSpace[vertI].w *= -1.0f;
        };
    };
#endif
    
    GLfloat verts[NUM_VERTS * 7] = {};
    i32 i{};
    f32 colorR{}, colorG{}, colorB{};
    for(i32 j{}; j < NUM_VERTS; ++j)
    {
        verts[i++] = squareVerts_openGLClipSpace[j].x;
        verts[i++] = squareVerts_openGLClipSpace[j].y;
        verts[i++] = squareVerts_openGLClipSpace[j].z;
        verts[i++] = squareVerts_openGLClipSpace[j].w;
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
                GLfloat verts[] =
                {
                    result[0].x, result[0].y, result[0].z,
                    1.0f, 0.0f, 0.0f,
                    result[1].x, result[1].y, result[1].z,
                    0.0f, 1.0f, 0.0f,
                    result[2].x, result[2].y, result[2].z,
                    1.0f, 0.0f, 0.0f,
                    result[3].x, result[3].y, result[3].z,
                    1.0f, 0.0f, 0.0f,
                    
                    result[4].x, result[4].y, result[4].z,
                    1.0f, 0.0f, 0.0f,
                    result[5].x, result[5].y, result[5].z,
                    0.0f, 1.0f, 0.0f,
                    result[6].x, result[6].y, result[6].z,
                    1.0f, 0.0f, 0.0f,
                    result[7].x, result[7].y, result[7].z,
                    1.0f, 0.0f, 0.0f,
                    
                    result[8].x, result[8].y, result[8].z,
                    1.0f, 0.0f, 0.0f,
                    result[9].x, result[9].y, result[9].z,
                    0.0f, 1.0f, 0.0f,
                    result[10].x, result[10].y, result[10].z,
                    1.0f, 0.0f, 0.0f,
                    result[11].x, result[11].y, result[11].z,
                    1.0f, 0.0f, 0.0f,
                    
                    result[12].x, result[12].y, result[12].z,
                    1.0f, 0.0f, 0.0f,
                    result[13].x, result[13].y, result[13].z,
                    0.0f, 1.0f, 0.0f,
                    result[14].x, result[14].y, result[14].z,
                    1.0f, 0.0f, 0.0f,
                    result[15].x, result[15].y, result[15].z,
                    1.0f, 0.0f, 0.0f,
                    
                    result[16].x, result[16].y, result[16].z,
                    1.0f, 0.0f, 0.0f,
                    result[17].x, result[17].y, result[17].z,
                    0.0f, 1.0f, 0.0f,
                    result[18].x, result[18].y, result[18].z,
                    1.0f, 0.0f, 0.0f,
                    result[19].x, result[19].y, result[18].z,
                    1.0f, 0.0f, 0.0f,
                    
                    result[20].x, result[20].y, result[20].z,
                    1.0f, 0.0f, 0.0f,
                    result[21].x, result[21].y, result[21].z,
                    0.0f, 1.0f, 0.0f,
                    result[22].x, result[22].y, result[22].z,
                    1.0f, 0.0f, 0.0f,
                    result[23].x, result[23].y, result[23].z,
                    1.0f, 0.0f, 0.0f,
                };
#endif
                
                ProjectionTestUsingFullSquare((f32)windowWidth, (f32)windowHeight);
                f32 windowWidth_meters = windowWidth / 100.0f;
                f32 windowHeight_meters = windowHeight / 100.0f;
                //ProjectionTestUsingFocalLength_InMeters((f32)windowWidth_meters, (f32)windowHeight_meters);
                //ProjectionTestUsingFOV_InMeters((f32)windowWidth_meters, (f32)windowHeight_meters);
                
                currentRenderBufferEntry += sizeof(RenderEntry_Test);
            }break;
            
            InvalidDefaultCase;
        };
    }
    
    renderingInfo.cmdBuffer.entryCount = 0;
};

#endif //OPENGL_INCLUDE_H
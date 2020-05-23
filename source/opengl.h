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

#define GLM_FORCE_LEFT_HANDED

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/ext/matrix_clip_space.hpp> // glm::perspective
#include <glm/gtc/matrix_transform.hpp> // glm::translate, rotate, etc.

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
    v3f scale{};
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

//#define USE_GLM_PATH

#define NUM_VERTS 8
struct Cube
{
#ifndef USE_GLM_PATH
    Array<v3f, NUM_VERTS> verts{};
#else
    Array<glm::vec4, NUM_VERTS> verts{};
#endif
    v3f centerPoint{};
};

GLushort indicies[] =
{
    1, 0, 2,  1, 2, 3,//Front
    4, 1, 3,  5, 1, 4, //Right side
    4, 5, 7,  7, 5, 6, //Back side
    6, 7, 0,  0, 7, 2 //Left side
};

f32 epsilon = 0.00001f; //TODO: Remove????

v3f CenterOfCube(Array<v3f, NUM_VERTS> cubeVerts)//Find center of cube in order to rotate in camera space next
{
    f32 sumXs{}, sumYs{}, sumZs{};
    for(i32 i{}; i < NUM_VERTS; ++i)
        sumXs += cubeVerts[i].x;
    for(i32 i{}; i < NUM_VERTS; ++i)
        sumYs += cubeVerts[i].y;
    for(i32 i{}; i < NUM_VERTS; ++i)
        sumZs += cubeVerts[i].z;
    v3f centerOfCube = v3f { sumXs/8.0f, sumYs/8.0f, sumZs/8.0f };
    
    return centerOfCube;
}


Array<v4f, NUM_VERTS> ProjectionTransform_UsingFocalLength(Array<v3f, NUM_VERTS> squareVerts_camera, f32 windowWidth_pxls, f32 windowHeight_pxls)
{
    Array<v4f, NUM_VERTS> squareVerts_openGLClipSpace{};
    
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

Array<v4f, NUM_VERTS> ProjectionTransform_UsingFOV(Array<v3f, NUM_VERTS> squareVerts_camera)
{
    Array<v4f, NUM_VERTS> squareVerts_openGLClipSpace{};
    
    f32 fov = glm::radians(80.0f);
    f32 aspectRatio = 16.0f/9.0f;
    f32 tanHalfFov = TanR(fov / 2.0f);
    f32 xScale = 1.0f / (tanHalfFov * aspectRatio);
    f32 yScale = 1.0f / tanHalfFov;
    
    f32 farClip = 100.0f;
    f32 nearClip = 0.1f;
    
    f32 a = (-farClip - nearClip) / (nearClip - farClip);
    f32 b = (2.0f * farClip * nearClip) / (nearClip - farClip);
    
    for(i32 vertI{}; vertI < NUM_VERTS; ++vertI)
    {
        squareVerts_openGLClipSpace[vertI].x = squareVerts_camera[vertI].x * xScale;
        squareVerts_openGLClipSpace[vertI].y = squareVerts_camera[vertI].y * yScale;
        squareVerts_openGLClipSpace[vertI].z = squareVerts_camera[vertI].z * a + b;
        squareVerts_openGLClipSpace[vertI].w = squareVerts_camera[vertI].z;
    };
    
    return squareVerts_openGLClipSpace;
};

struct Basis
{
    v3f origin{};//Universal space
    v3f xAxis{1.0f, 0.0f, 0.0f};
    v3f yAxis{0.0f, 1.0f, 0.0f};
    v3f zAxis{0.0f, 0.0f, 1.0f};
    v3f translation{};
};

#ifndef USE_GLM_PATH
v3f RotateVector(v3f vecToRotate, v3f rotation)
{
    //Left-handed rotation equations
#if 1
    v3f newRotatedVector{};
    
    //X axis rotation
    v3f yBasis_xAxisRotation = v3f { 0.0f, CosR(rotation.x), SinR(rotation.x) };
    v3f zBasis_xAxisRotation = v3f { 0.0f, -SinR(rotation.x), CosR(rotation.x) };
    newRotatedVector.yz = (vecToRotate.z * zBasis_xAxisRotation.yz) + (vecToRotate.y * yBasis_xAxisRotation.yz);
    
    //Z axis rotation
    v3f xBasis_zAxisRotation = v3f { CosR(rotation.z), SinR(rotation.z), 0.0f };
    v3f yBasis_zAxisRotation = v3f{ -SinR(rotation.z), CosR(rotation.z), 0.0f };
    newRotatedVector.xy = (vecToRotate.x * xBasis_zAxisRotation.xy) + (newRotatedVector.y * yBasis_zAxisRotation.xy);
    
    //Y axis rotation
    f32 rotatedY = newRotatedVector.y;
    v3f xBasis_yAxisRotation = v3f { CosR(rotation.y), 0.0f, -SinR(rotation.y) };
    v3f zBasis_yAxisRotation = v3f { SinR(rotation.y), 0.0f, CosR(rotation.y) };
    newRotatedVector = (newRotatedVector.x * xBasis_yAxisRotation) + (newRotatedVector.z * zBasis_yAxisRotation);
    newRotatedVector.y = rotatedY;
    
    return newRotatedVector;
#else
    
    //Right-handed rotation equations
    v3f newRotatedVector{};
    
    //X axis rotation
    v3f zBasis_xAxisRotation = v3f { 0.0f, SinR(rotation.x), CosR(rotation.x) };
    v3f yBasis_xAxisRotation = v3f { 0.0f, CosR(rotation.x), -SinR(rotation.x) };
    newRotatedVector.yz = (vecToRotate.z * zBasis_xAxisRotation.yz) + (vecToRotate.y * yBasis_xAxisRotation.yz);
    
    //Z axis rotation
    v3f xBasis_zAxisRotation = v3f { CosR(rotation.z), SinR(rotation.z), 0.0f };
    v3f yBasis_zAxisRotation = v3f{ -SinR(rotation.z), CosR(rotation.z), 0.0f };
    newRotatedVector.xy = (vecToRotate.x * xBasis_zAxisRotation.xy) + (newRotatedVector.y * yBasis_zAxisRotation.xy);
    
    //Y axis rotation
    f32 rotatedY = newRotatedVector.y;
    v3f xBasis_yAxisRotation = v3f { CosR(rotation.y), 0.0f, SinR(rotation.y) };
    v3f zBasis_yAxisRotation = v3f { -SinR(rotation.y), 0.0f, CosR(rotation.y) };
    newRotatedVector = (newRotatedVector.x * xBasis_yAxisRotation) + (newRotatedVector.z * zBasis_yAxisRotation);
    newRotatedVector.y = rotatedY;
    
    return newRotatedVector;
#endif
};

#else
glm::vec3 RotateVector(glm::vec3 vecToRotate, glm::vec3 rotation)
{
    v3f newRotatedVector{};
    
    //X axis rotation
    v3f yBasis_xAxisRotation = v3f { 0.0f, CosR(rotation.x), SinR(rotation.x) };
    v3f zBasis_xAxisRotation = v3f { 0.0f, -SinR(rotation.x), CosR(rotation.x) };
    newRotatedVector.yz = (vecToRotate.z * zBasis_xAxisRotation.yz) + (vecToRotate.y * yBasis_xAxisRotation.yz);
    
    //Z axis rotation
    v3f xBasis_zAxisRotation = v3f { CosR(rotation.z), -SinR(rotation.z), 0.0f };
    v3f yBasis_zAxisRotation = v3f{ SinR(rotation.z), CosR(rotation.z), 0.0f };
    newRotatedVector.xy = (vecToRotate.x * xBasis_zAxisRotation.xy) + (newRotatedVector.y * yBasis_zAxisRotation.xy);
    
    //Y axis rotation
    f32 rotatedY = newRotatedVector.y;
    v3f xBasis_yAxisRotation = v3f { CosR(rotation.y), 0.0f, -SinR(rotation.y) };
    v3f zBasis_yAxisRotation = v3f { SinR(rotation.y), 0.0f, CosR(rotation.y) };
    newRotatedVector = (newRotatedVector.x * xBasis_yAxisRotation) + (newRotatedVector.z * zBasis_yAxisRotation);
    newRotatedVector.y = rotatedY;
    
    glm::vec3 result = {newRotatedVector.x, newRotatedVector.y, newRotatedVector.z};
    
    return result;
};
#endif

Basis ProduceWorldBasis(v3f translation, v3f rotation, v3f scale)
{
    Basis resultBasis{};
    
    ConvertToCorrectPositiveRadian($(rotation.x));
    ConvertToCorrectPositiveRadian($(rotation.y));
    ConvertToCorrectPositiveRadian($(rotation.z));
    
    resultBasis.translation = translation;
    resultBasis.xAxis = {1.0f, 0.0f, 0.0f};
    resultBasis.yAxis = {0.0f, 1.0f, 0.0f};
    resultBasis.zAxis = {0.0f, 0.0f, 1.0f};
    
    resultBasis.xAxis = scale.x * RotateVector(resultBasis.xAxis, rotation);
    resultBasis.yAxis = scale.y * RotateVector(resultBasis.yAxis, rotation);
    resultBasis.zAxis = scale.z * RotateVector(resultBasis.zAxis, rotation);
    
    return resultBasis;
};

Array<v3f, NUM_VERTS> TransformVerts(Array<v3f, NUM_VERTS> cubeVerts_childSpace, Basis parentBasis)
{
    Array<v3f, NUM_VERTS> cubeVerts_parentSpace{};
    
    v3f centerOfCube = CenterOfCube(cubeVerts_childSpace);
    
    for(i32 i{}; i < NUM_VERTS; ++i)
    {
        v3f cubeVert_parentOriginSpace = cubeVerts_childSpace[i] - centerOfCube;
        cubeVerts_parentSpace[i] = parentBasis.translation + (cubeVert_parentOriginSpace.x * parentBasis.xAxis) + (cubeVert_parentOriginSpace.y * parentBasis.yAxis) + (cubeVert_parentOriginSpace.z * parentBasis.zAxis);
    }
    
    return cubeVerts_parentSpace;
};

Basis ProduceCameraBasis(Array<v3f, NUM_VERTS> cubeVerts_world, v3f cameraPostion_world, v3f rotation, v3f scale)
{
    Basis resultBasis{};
    
    ConvertToCorrectPositiveRadian($(rotation.x));
    ConvertToCorrectPositiveRadian($(rotation.y));
    ConvertToCorrectPositiveRadian($(rotation.z));
    
    v3f cubeCenter_world = CenterOfCube(cubeVerts_world);
    
    resultBasis.translation = cubeCenter_world - cameraPostion_world;
    
    resultBasis.xAxis = {1.0f, 0.0f, 0.0f};
    resultBasis.yAxis = {0.0f, 1.0f, 0.0f};
    resultBasis.zAxis = {0.0f, 0.0f, 1.0f};
    
    resultBasis.xAxis = scale.x * RotateVector(resultBasis.xAxis, rotation);
    resultBasis.yAxis = scale.y * RotateVector(resultBasis.yAxis, rotation);
    resultBasis.zAxis = scale.z * RotateVector(resultBasis.zAxis, rotation);
    
    return resultBasis;
};

Array<v3f, NUM_VERTS> CameraTransform(Array<v3f, NUM_VERTS> cubeVerts_world, v3f cameraPostion_world, v3f cameraLookAt, v3f rotation, v3f scale)
{
    Array<v3f, NUM_VERTS> cubeVerts_camera{};
    
    ConvertToCorrectPositiveRadian($(rotation.x));
    ConvertToCorrectPositiveRadian($(rotation.y));
    ConvertToCorrectPositiveRadian($(rotation.z));
    
    Basis cameraBasis{};
    
    v3f diff = cameraLookAt - cameraPostion_world;
    v3f diffX = v3f{ diff.x, 0.0f, diff.z};
    v3f diffY = v3f{0.0f, diff.y, diff.z};
    
    f32 theta_yRot{};
    if(cameraPostion_world.x != 0.0f)
    {
        f32 dotProduct = DotProduct(cameraBasis.zAxis, diffX);
        f32 combinedMagnitudes = (Magnitude(cameraBasis.zAxis) * Magnitude(diffX));
        theta_yRot = InvCosR(dotProduct / combinedMagnitudes);
    };
    
    f32 theta_xRot{};
    if(cameraPostion_world.y != 0.0f)
    {
        theta_xRot = InvCosR(DotProduct(cameraBasis.zAxis, diffY) / (Magnitude(cameraBasis.zAxis) * Magnitude(diffY)));
    }
    
    rotation = {theta_xRot, theta_yRot, 0.0f};
    cameraBasis.xAxis = RotateVector(cameraBasis.xAxis, rotation);
    cameraBasis.yAxis = RotateVector(cameraBasis.yAxis, rotation);
    cameraBasis.zAxis = RotateVector(cameraBasis.zAxis, rotation);
    
    for(i32 i{}; i < NUM_VERTS; ++i)
    {
        cubeVerts_world[i] = cubeVerts_world[i] + -cameraPostion_world;
        
        cubeVerts_camera[i].x = DotProduct(cubeVerts_world[i], cameraBasis.xAxis);
        cubeVerts_camera[i].y = DotProduct(cubeVerts_world[i], cameraBasis.yAxis);
        cubeVerts_camera[i].z = DotProduct(cubeVerts_world[i], cameraBasis.zAxis);
    };
    
    return cubeVerts_camera;
};

#ifndef USE_GLM_PATH
void ProjectionTestUsingFullSquare(Cube cube, f32 windowWidth, f32 windowHeight)
{
    local_persist v3f worldRotation = {0.0f, 0.0f, 0.0f} ;
    local_persist v3f worldTranslation = {0.0f, 0.0f, 1.0f};
    local_persist v3f worldScale = {1.0f, 1.0f, 1.0f};
    worldRotation.y += .01f;
    
    Basis worldBasis = ProduceWorldBasis(worldTranslation, worldRotation, worldScale);
    
    //World Transform
    Array<v3f, NUM_VERTS> squareVerts_world = TransformVerts(cube.verts, worldBasis);
    
    Array<v3f, NUM_VERTS> squareVerts_camera{};
    {//Camera Transform
        local_persist v3f cameraPostion_world{0.0f, 0.0f, -2.0f};
        local_persist v3f rotation_camera{0.0f, 0.0f, 0.0f};
        
        Basis camera{};
        local_persist v3f camRotation{0.0f, 0.0f, 0.0f};
        camera.zAxis = RotateVector(camera.zAxis, camRotation);
        camera.yAxis = RotateVector(camera.yAxis, camRotation);
        camera.xAxis = RotateVector(camera.xAxis, camRotation);
        mat4x4 camTransform = CamTransform(camera.xAxis, camera.yAxis, camera.zAxis, cameraPostion_world);
        
        for(i32 i{}; i < NUM_VERTS; ++i)
        {
            v4f squareVert_world4d = v4f {squareVerts_world[i].x, squareVerts_world[i].y, squareVerts_world[i].z, 1.0f};
            squareVerts_camera[i] = (camTransform * squareVert_world4d).xyz;
        }
    }
    
    //ProjectionTransform
    Array<v4f, NUM_VERTS> squareVerts_openGLClipSpace = ProjectionTransform_UsingFOV(squareVerts_camera);
    //Array<v4f, NUM_VERTS> squareVerts_openGLClipSpace = ProjectionTransform_UsingFocalLength(squareVerts_camera, windowWidth, windowHeight);
    
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

#else
void ProjectionTestUsingFullSquare_GLM(Cube cube, f32 windowWidth, f32 windowHeight)
{
    local_persist f32 rotation = 0.0f;
    glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), glm::vec3{0.0f, 0.0f, 1.0f});
    glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), rotation, glm::vec3{1.0f, 0.0f, 0.0f});
    
    //World transform
    glm::mat4 worldTransformMatrix = translationMatrix * rotationMatrix;
    Array<glm::vec4, NUM_VERTS> cubeVerts_world{};
    for(i32 i{}; i < NUM_VERTS; ++i)
        cubeVerts_world[i] = worldTransformMatrix * cube.verts[i];
    
    //Camera transform
    Array<glm::vec4, NUM_VERTS> cubeVerts_camera{};
    local_persist glm::vec3 camPos = {0.0f, 0.0f, -2.0f}, upVec{0.0f, 1.0f, 0.0f};
    glm::vec3 viewDirection{0.0f, 0.0f, 1.0f};
    local_persist f32 cameraRotation = 0.0f;
    cameraRotation += .0004f;
    //viewDirection = RotateVector(viewDirection, cameraRotation);
    glm::mat4 rotationMatrix_cam = glm::rotate(glm::mat4(1.0f), cameraRotation, glm::vec3{0.0f, 0.0f, 1.0f});
    rotationMatrix_cam = glm::rotate(rotationMatrix_cam, cameraRotation, glm::vec3{0.0f, 1.0f, 0.0f});
    rotationMatrix_cam = glm::rotate(rotationMatrix_cam, cameraRotation, glm::vec3{1.0f, 0.0f, 0.0f});
    glm::vec4 viewDirection_4d = rotationMatrix_cam * glm::vec4{viewDirection, 1.0f};
    viewDirection = glm::vec3{viewDirection_4d};
    glm::mat4 cameraTransformMatrix = glm::lookAtLH(camPos, camPos + viewDirection, upVec);
    for(i32 i{}; i < NUM_VERTS; ++i)
        cubeVerts_camera[i] = cameraTransformMatrix * cubeVerts_world[i];
    
    //Projection transform
    glm::mat4 projectionTransform = glm::perspective(glm::radians(80.0f), windowWidth/windowHeight, 0.1f, 100.0f);
    Array<glm::vec4, NUM_VERTS> cubeVerts_openGLClipSpace{};
    for(i32 i{}; i < NUM_VERTS; ++i)
        cubeVerts_openGLClipSpace[i] = projectionTransform * cubeVerts_camera[i];
    
    GLfloat verts[NUM_VERTS * 7] = {};
    i32 i{};
    f32 colorR{}, colorG{}, colorB{};
    for(i32 j{}; j < NUM_VERTS; ++j)
    {
        verts[i++] = cubeVerts_openGLClipSpace[j].x;
        verts[i++] = cubeVerts_openGLClipSpace[j].y;
        verts[i++] = cubeVerts_openGLClipSpace[j].z;
        verts[i++] = cubeVerts_openGLClipSpace[j].w;
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
#endif

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
                Cube cube{};
                
#ifndef USE_GLM_PATH
                cube.verts = {
                    v3f{-0.5f, 0.5f, -0.5f }, //0
                    v3f{+0.5f, 0.5f, -0.5f }, //1
                    v3f{-0.5f, -0.5f, -0.5f },//2
                    v3f{+0.5f, -0.5f, -0.5f },//3
                    v3f{+0.5f, -0.5f, +0.5f },//4
                    v3f{+0.5f, +0.5f, +0.5f },//5
                    v3f{-0.5f, +0.5f, +0.5f },//6
                    v3f{-0.5f, -0.5f, +0.5f },//7
                };
                
                ProjectionTestUsingFullSquare(cube, (f32)windowWidth, (f32)windowHeight);
#else
                cube.verts = {
                    glm::vec4{-0.5f, 0.5f, -0.5f, 1.0f }, //0
                    glm::vec4{+0.5f, 0.5f, -0.5f, 1.0f }, //1
                    glm::vec4{-0.5f, -0.5f, -0.5f, 1.0f },//2
                    glm::vec4{+0.5f, -0.5f, -0.5f, 1.0f },//3
                    glm::vec4{+0.5f, -0.5f, +0.5f, 1.0f },//4
                    glm::vec4{+0.5f, +0.5f, +0.5f, 1.0f },//5
                    glm::vec4{-0.5f, +0.5f, +0.5f, 1.0f },//6
                    glm::vec4{-0.5f, -0.5f, +0.5f, 1.0f },//7
                };
                
                ProjectionTestUsingFullSquare_GLM(cube, (f32)windowWidth, (f32)windowHeight);
#endif
                
                currentRenderBufferEntry += sizeof(RenderEntry_Test);
            }break;
            
            InvalidDefaultCase;
        };
    }
    
    renderingInfo.cmdBuffer.entryCount = 0;
};

#endif //OPENGL_INCLUDE_H
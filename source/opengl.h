#ifndef OPENGL_INCLUDE_H
#define OPENGL_INCLUDE_H

#include "renderer_stuff.h"

local_func void
GLInit(int windowWidth, int windowHeight)
{
    //If this is set to GL_MODULATE instead then you might get unwanted texture coloring.
    //In order to avoid that in GL_MODULATE mode you need to constantly set glcolor to white after drawing.
    //For more info: https://stackoverflow.com/questions/53180760/all-texture-colors-affected-by-colored-rectangle-opengl
    glViewport(0, 0, windowWidth, windowHeight);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glMatrixMode(GL_MODELVIEW); //For fixed function pipeline modelview matrix is pretty much non-essential "handmade hero ep: 237 26:52". So just load identity matrix for this thing and be done with it (identiy matrix is basically a noop for matrices)
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, (f32)windowWidth, 0.0, (f32)windowHeight, -1.0, 1.0); //Sets the projection matrix in openGL which will take our screen coordinates and tramsform them to openGL's clip space (-1 to 1)
}

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

void RenderViaHardware(Rendering_Info&& renderingInfo, int windowWidth, int windowHeight)
{
    local_persist bool glIsInitialized { false };
    if (NOT glIsInitialized)
    {
        GLInit(windowWidth, windowHeight);
        glClearColor(1.0f, 0.0f, 1.0f, 0.0f);
        glIsInitialized = true;
    };
    
    u8* currentRenderBufferEntry = renderingInfo.cmdBuffer.baseAddress;
    Camera2D* camera = &renderingInfo.camera;
    
    f32 pixelsPerMeter = renderingInfo._pixelsPerMeter;
    v2i screenSize = { windowWidth, windowHeight };
    v2 screenSize_meters = CastV2IToV2F(screenSize) / pixelsPerMeter;
    camera->dilatePoint_inScreenCoords = (screenSize_meters / 2.0f) + (Hadamard(screenSize_meters, camera->dilatePointOffset_normalized));
    
    camera->viewCenter = screenSize_meters / 2.0f;
    
    glClear(GL_COLOR_BUFFER_BIT);
    
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
                
                Quadf imageTargetRect_camera = CameraTransform(textureEntry.targetRect_worldCoords, *camera);
                Quadf imageTargetRect_screen = ProjectionTransform_Ortho(imageTargetRect_camera, pixelsPerMeter);
                
                DrawTexture(textureID, imageTargetRect_screen, textureEntry.uvBounds[0], textureEntry.uvBounds[1]);
                
                currentRenderBufferEntry += sizeof(RenderEntry_Texture);
            }break;
            
            case EntryType_Rect: {
                RenderEntry_Rect rectEntry = *(RenderEntry_Rect*)currentRenderBufferEntry;
                
                Quadf targetRect_camera = CameraTransform(rectEntry.worldCoords, *camera);
                Quadf targetRect_screen = ProjectionTransform_Ortho(targetRect_camera, pixelsPerMeter);
                
                v4 color = { rectEntry.color, 1.0f };
                
                glDisable(GL_TEXTURE_2D);
                DrawQuad(targetRect_screen, color);
                glEnable(GL_TEXTURE_2D);
                
                currentRenderBufferEntry += sizeof(RenderEntry_Rect);
            }break;
            
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
            }break;
            
            case EntryType_Geometry: {
                RenderEntry_Geometry goemetryEntry = *(RenderEntry_Geometry*)currentRenderBufferEntry;
                
            }break;
            
            InvalidDefaultCase;
        };
    }
    
    renderingInfo.cmdBuffer.entryCount = 0;
};

#endif //OPENGL_INCLUDE_H
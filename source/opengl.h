#ifndef OPENGL_INCLUDE_H
#define OPENGL_INCLUDE_H

#include "renderer_stuff.h"

    local_func auto
    GLInit(f32 windowWidth, f32 windowHeight) -> void
    {
        //If this is set to GL_MODULATE instead then you might get unwanted texture coloring.
        //In order to avoid that in GL_MODULATE mode you need to constantly set glcolor to white after drawing.
        //For more info: https://stackoverflow.com/questions/53180760/all-texture-colors-affected-by-colored-rectangle-opengl
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0.0, windowWidth, 0.0, windowHeight, -1.0, 1.0);
    }

    local_func auto
    LoadTexture(Image ImageToSendToGPU) -> Texture
    {
        Texture ResultingTexture {};
        ResultingTexture.size.width = ImageToSendToGPU.size.width;
        ResultingTexture.size.height = ImageToSendToGPU.size.height;

        ui8* ImageData = ImageToSendToGPU.data;

        glEnable(GL_TEXTURE_2D);
        glGenTextures(1, &ResultingTexture.ID);
        glBindTexture(GL_TEXTURE_2D, ResultingTexture.ID);

        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_RGBA, ResultingTexture.size.width, ResultingTexture.size.height,
            0, GL_RGBA, GL_UNSIGNED_BYTE, ImageData);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

        //Enable alpha channel for transparency
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_TEXTURE_2D);

        return ResultingTexture;
    }

    local_func auto
    DrawBackground(ui32 TextureID, Rectf BackgroundImage, v2f MinUV, v2f MaxUV) -> void
    {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, TextureID);

        glBegin(GL_QUADS);
        glTexCoord2f(MinUV.x, MinUV.y);
        glVertex2f(BackgroundImage.min.x, BackgroundImage.min.y);

        glTexCoord2f(MaxUV.x, MinUV.y);
        glVertex2f(BackgroundImage.max.x, BackgroundImage.min.y);

        glTexCoord2f(MaxUV.x, MaxUV.y);
        glVertex2f(BackgroundImage.max.x, BackgroundImage.max.y);

        glTexCoord2f(MinUV.x, MaxUV.y);
        glVertex2f(BackgroundImage.min.x, BackgroundImage.max.y);

        glEnd();
        glFlush();

        glDisable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    local_func auto
    DrawRect(v2f MinPoint, v2f MaxPoint, v4f color) -> void
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
    DrawTexture(ui32 TextureID, Rectf Destination, v2f* UVs)
    {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, TextureID);

        glBegin(GL_QUADS);
        glTexCoord2f(UVs[0].x, UVs[0].y);
        glVertex2f(Destination.min.x, Destination.min.y);

        glTexCoord2f(UVs[1].x, UVs[1].y);
        glVertex2f(Destination.max.x, Destination.min.y);

        glTexCoord2f(UVs[2].x, UVs[2].y);
        glVertex2f(Destination.max.x, Destination.max.y);

        glTexCoord2f(UVs[3].x, UVs[3].y);
        glVertex2f(Destination.min.x, Destination.max.y);

        glEnd();
        glFlush();

        glDisable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    local_func void
    DrawLine(v2f minPoint, v2f maxPoint)
    {
        glLineWidth(9.0f);
        glBegin(GL_LINES);
        glColor3f(0.0f, 0.0f, 1.0f);
        glVertex2f(minPoint.x, minPoint.y);
        glColor3f(0.0f, 0.0f, 1.0f);
        glVertex2f(maxPoint.x, maxPoint.y);
        glEnd();
        glFlush();
    };

    local_func auto
    ClearScreen() -> void
    {
        glClear(GL_COLOR_BUFFER_BIT);
    };

    void RenderViaHardware()
    {

    };

#endif //OPENGL_INCLUDE_H
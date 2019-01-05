/******************************************************************************
 * Spine Runtimes Software License v2.5
 *
 * Copyright (c) 2013-2016, Esoteric Software
 * All rights reserved.
 *
 * You are granted a perpetual, non-exclusive, non-sublicensable, and
 * non-transferable license to use, install, execute, and perform the Spine
 * Runtimes software and derivative works solely for personal or internal
 * use. Without the written permission of Esoteric Software (see Section 2 of
 * the Spine Software License Agreement), you may not (a) modify, translate,
 * adapt, or develop new applications using the Spine Runtimes or otherwise
 * create derivative works or improvements of the Spine Runtimes or (b) remove,
 * delete, alter, or obscure any trademarks or any copyright, trademark, patent,
 * or other intellectual property or proprietary rights notices on or in the
 * Software, including any copy thereof. Redistributions in binary or source
 * form must include this license and terms.
 *
 * THIS SOFTWARE IS PROVIDED BY ESOTERIC SOFTWARE "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL ESOTERIC SOFTWARE BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES, BUSINESS INTERRUPTION, OR LOSS OF
 * USE, DATA, OR PROFITS) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************************/

#ifndef ATLAS_H_
#define ATLAS_H_

typedef enum
{
    ATLAS_UNKNOWN_FORMAT,
    ATLAS_ALPHA,
    ATLAS_INTENSITY,
    ATLAS_LUMINANCE_ALPHA,
    ATLAS_RGB565,
    ATLAS_RGBA4444,
    ATLAS_RGB888,
    ATLAS_RGBA8888
} AtlasFormat;

typedef enum
{
    ATLAS_UNKNOWN_FILTER,
    ATLAS_NEAREST,
    ATLAS_LINEAR,
    ATLAS_MIPMAP,
    ATLAS_MIPMAP_NEAREST_NEAREST,
    ATLAS_MIPMAP_LINEAR_NEAREST,
    ATLAS_MIPMAP_NEAREST_LINEAR,
    ATLAS_MIPMAP_LINEAR_LINEAR
} AtlasFilter;

typedef enum
{
    ATLAS_MIRROREDREPEAT,
    ATLAS_CLAMPTOEDGE,
    ATLAS_REPEAT
} AtlasWrap;

struct Atlas;
struct AtlasPage
{
    const Atlas* atlas;
    const char* name;
    AtlasFormat format;
    AtlasFilter minFilter, magFilter;
    AtlasWrap uWrap, vWrap;

    void* rendererObject;
    int width, height;

    AtlasPage* next;
};

AtlasPage* AtlasPage_create(Atlas* atlas, const char* name);
void AtlasPage_dispose(AtlasPage* self);

typedef struct AtlasRegion AtlasRegion;
struct AtlasRegion
{
    const char* name;
    int x, y, width, height;
    float u, v, u2, v2;
    int offsetX, offsetY;
    int originalWidth, originalHeight;
    int index;
    int /*bool*/ rotate;
    int /*bool*/ flip;
    int* splits;
    int* pads;

    AtlasPage* page;

    AtlasRegion* next;
};

AtlasRegion* AtlasRegion_create();
void AtlasRegion_dispose(AtlasRegion* self);

struct Atlas
{
    AtlasPage* pages;
    AtlasRegion* regions;

    void* rendererObject;
};

/* Image files referenced in the atlas file will be prefixed with the directory containing the atlas file. */
Atlas* CreateAtlasFromFile(const char* path, void* rendererObject);
void Atlas_dispose(Atlas* atlas);

/* Returns 0 if the region was not found. */
AtlasRegion* Atlas_findRegion(const Atlas* self, const char* name);

#endif /* ATLAS_H_ */

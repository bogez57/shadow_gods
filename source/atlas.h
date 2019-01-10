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

#ifndef ATLAS_INCLUDE_H
#define ATLAS_INCLUDE_H

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

Atlas* CreateAtlas(const char* begin, i64 length, const char* dir, void* rendererObject);
void Atlas_dispose(Atlas* atlas);

/* Returns 0 if the region was not found. */
AtlasRegion* Atlas_findRegion(const Atlas* self, const char* name);

#endif // ATLAS_INCLUDE_H

#ifdef ATLAS_IMPL

#include <string.h>
#include <ctype.h>

#define MALLOC_STR(TO, FROM) strcpy(CONST_CAST(char*, TO) = (char*)MallocType(0, char, strlen(FROM) + 1), FROM)
#define CONST_CAST(TYPE, VALUE) (*(TYPE*)&VALUE)

void _AtlasPage_createTexture(AtlasPage* self, const char* path)
{
    Image image;
    image.Data = globalPlatformServices->LoadRGBAImage(path, &image.size.width, &image.size.height);

    Texture texture = globalRenderCmds.LoadTexture(image);
    Texture* textureptr = MallocType(0, Texture, 1);
    *textureptr = texture;

    self->rendererObject = textureptr;
    self->width = image.size.width;
    self->height = image.size.height;
};

void _AtlasPage_disposeTexture(AtlasPage* self)
{
    BGZ_ERRCTXT1("When disposing of texture in spine's atlaspage");
    BGZ_ASSERT(self->rendererObject, "Texture does not exist!");

    DeAlloc(0, self->rendererObject);
    self->width = 0;
    self->height = 0;
};

AtlasPage* AtlasPage_create(Atlas* atlas, const char* name)
{
    AtlasPage* self = CallocType(0, AtlasPage, 1);
    CONST_CAST(Atlas*, self->atlas) = atlas;
    MALLOC_STR(self->name, name);
    return self;
}

void AtlasPage_dispose(AtlasPage* self)
{
    _AtlasPage_disposeTexture(self);
    DeAlloc(0, self->name);
    DeAlloc(0, self);
}

/**/

typedef struct
{
    const char* begin;
    const char* end;
} Str;

static void trim(Str* str)
{
    while (isspace((unsigned char)*str->begin) && str->begin < str->end)
        (str->begin)++;
    if (str->begin == str->end)
        return;
    str->end--;
    while (isspace((unsigned char)*str->end) && str->end >= str->begin)
        str->end--;
    str->end++;
}

/* Tokenize string without modification. Returns 0 on failure. */
static int readLine(const char** begin, const char* end, Str* str)
{
    if (*begin == end)
        return 0;
    str->begin = *begin;

    /* Find next delimiter. */
    while (*begin != end && **begin != '\n')
        (*begin)++;

    str->end = *begin;
    trim(str);

    if (*begin != end)
        (*begin)++;
    return 1;
}

/* Moves str->begin past the first occurence of c. Returns 0 on failure. */
static int beginPast(Str* str, char c)
{
    const char* begin = str->begin;
    while (1)
    {
        char lastSkippedChar = *begin;
        if (begin == str->end)
            return 0;
        begin++;
        if (lastSkippedChar == c)
            break;
    }
    str->begin = begin;
    return 1;
}

/* Returns 0 on failure. */
static int readValue(const char** begin, const char* end, Str* str)
{
    readLine(begin, end, str);
    if (!beginPast(str, ':'))
        return 0;
    trim(str);
    return 1;
}

/* Returns the number of tuple values read (1, 2, 4, or 0 for failure). */
static int readTuple(const char** begin, const char* end, Str tuple[])
{
    int i;
    Str str = { NULL, NULL };
    readLine(begin, end, &str);
    if (!beginPast(&str, ':'))
        return 0;

    for (i = 0; i < 3; ++i)
    {
        tuple[i].begin = str.begin;
        if (!beginPast(&str, ','))
            break;
        tuple[i].end = str.begin - 2;
        trim(&tuple[i]);
    }
    tuple[i].begin = str.begin;
    tuple[i].end = str.end;
    trim(&tuple[i]);
    return i + 1;
}

static char* mallocString(Str* str)
{
    int length = (int)(str->end - str->begin);
    char* string = MallocType(0, char, length + 1);
    memcpy(string, str->begin, length);
    string[length] = '\0';
    return string;
}

static int indexOf(const char** array, int count, Str* str)
{
    int length = (int)(str->end - str->begin);
    int i;
    for (i = count - 1; i >= 0; i--)
        if (strncmp(array[i], str->begin, length) == 0)
            return i;
    return 0;
}

static int equals(Str* str, const char* other)
{
    return strncmp(other, str->begin, str->end - str->begin) == 0;
}

static int toInt(Str* str)
{
    return (int)strtol(str->begin, (char**)&str->end, 10);
}

static Atlas* abortAtlas(Atlas* self)
{
    Atlas_dispose(self);
    return 0;
}

/**/

AtlasRegion* AtlasRegion_create()
{
    return CallocType(0, AtlasRegion, 1);
}

void AtlasRegion_dispose(AtlasRegion* self)
{
    DeAlloc(0, self->name);
    DeAlloc(0, self->splits);
    DeAlloc(0, self->pads);
    DeAlloc(0, self);
}

static const char* formatNames[] = { "", "Alpha", "Intensity", "LuminanceAlpha", "RGB565", "RGBA4444", "RGB888", "RGBA8888" };
static const char* textureFilterNames[] = { "", "Nearest", "Linear", "MipMap", "MipMapNearestNearest", "MipMapLinearNearest",
    "MipMapNearestLinear", "MipMapLinearLinear" };

Atlas* CreateAtlas(const char* begin, i64 length, const char* dir, void* rendererObject)
{
    Atlas* self;

    i64 count;
    const char* end = begin + length;
    i64 dirLength = (i64)strlen(dir);
    i64 needsSlash = dirLength > 0 && dir[dirLength - 1] != '/' && dir[dirLength - 1] != '\\';

    AtlasPage* page = 0;
    AtlasPage* lastPage = 0;
    AtlasRegion* lastRegion = 0;
    Str str;
    Str tuple[4];

    self = CallocType(0, Atlas, 1);
    self->rendererObject = rendererObject;

    while (readLine(&begin, end, &str))
    {
        if (str.end - str.begin == 0)
        {
            page = 0;
        }
        else if (!page)
        {
            char* name = mallocString(&str);
            char* path = MallocType(0, char, dirLength + needsSlash + strlen(name) + 1);
            memcpy(path, dir, dirLength);
            if (needsSlash)
                path[dirLength] = '/';
            strcpy(path + dirLength + needsSlash, name);

            page = AtlasPage_create(self, name);
            DeAlloc(0, name);
            if (lastPage)
                lastPage->next = page;
            else
                self->pages = page;
            lastPage = page;

            switch (readTuple(&begin, end, tuple))
            {
            case 0:
                return abortAtlas(self);
            case 2: /* size is only optional for an atlas packed with an old TexturePacker. */
                page->width = toInt(tuple);
                page->height = toInt(tuple + 1);
                if (!readTuple(&begin, end, tuple))
                    return abortAtlas(self);
            }
            page->format = (AtlasFormat)indexOf(formatNames, 8, tuple);

            if (!readTuple(&begin, end, tuple))
                return abortAtlas(self);
            page->minFilter = (AtlasFilter)indexOf(textureFilterNames, 8, tuple);
            page->magFilter = (AtlasFilter)indexOf(textureFilterNames, 8, tuple + 1);

            if (!readValue(&begin, end, &str))
                return abortAtlas(self);

            page->uWrap = ATLAS_CLAMPTOEDGE;
            page->vWrap = ATLAS_CLAMPTOEDGE;
            if (!equals(&str, "none"))
            {
                if (str.end - str.begin == 1)
                {
                    if (*str.begin == 'x')
                        page->uWrap = ATLAS_REPEAT;
                    else if (*str.begin == 'y')
                        page->vWrap = ATLAS_REPEAT;
                }
                else if (equals(&str, "xy"))
                {
                    page->uWrap = ATLAS_REPEAT;
                    page->vWrap = ATLAS_REPEAT;
                }
            }

            _AtlasPage_createTexture(page, path);
            DeAlloc(0, path);
        }
        else
        {
            AtlasRegion* region = AtlasRegion_create();
            if (lastRegion)
                lastRegion->next = region;
            else
                self->regions = region;
            lastRegion = region;

            region->page = page;
            region->name = mallocString(&str);

            if (!readValue(&begin, end, &str))
                return abortAtlas(self);
            region->rotate = equals(&str, "true");

            if (readTuple(&begin, end, tuple) != 2)
                return abortAtlas(self);
            region->x = toInt(tuple);
            region->y = toInt(tuple + 1);

            if (readTuple(&begin, end, tuple) != 2)
                return abortAtlas(self);
            region->width = toInt(tuple);
            region->height = toInt(tuple + 1);

            region->u = region->x / (float)page->width;
            region->v = region->y / (float)page->height;
            if (region->rotate)
            {
                region->u2 = (region->x + region->height) / (float)page->width;
                region->v2 = (region->y + region->width) / (float)page->height;
            }
            else
            {
                region->u2 = (region->x + region->width) / (float)page->width;
                region->v2 = (region->y + region->height) / (float)page->height;
            }

            count = readTuple(&begin, end, tuple);
            if (!count)
                return abortAtlas(self);
            if (count == 4)
            { /* split is optional */
                region->splits = MallocType(0, i32, 4);
                region->splits[0] = toInt(tuple);
                region->splits[1] = toInt(tuple + 1);
                region->splits[2] = toInt(tuple + 2);
                region->splits[3] = toInt(tuple + 3);

                count = readTuple(&begin, end, tuple);
                if (!count)
                    return abortAtlas(self);
                if (count == 4)
                { /* pad is optional, but only present with splits */
                    region->pads = MallocType(0, i32, 4);
                    region->pads[0] = toInt(tuple);
                    region->pads[1] = toInt(tuple + 1);
                    region->pads[2] = toInt(tuple + 2);
                    region->pads[3] = toInt(tuple + 3);

                    if (!readTuple(&begin, end, tuple))
                        return abortAtlas(self);
                }
            }

            region->originalWidth = toInt(tuple);
            region->originalHeight = toInt(tuple + 1);

            readTuple(&begin, end, tuple);
            region->offsetX = toInt(tuple);
            region->offsetY = toInt(tuple + 1);

            if (!readValue(&begin, end, &str))
                return abortAtlas(self);
            region->index = toInt(&str);
        }
    }

    return self;
};

Atlas* CreateAtlasFromFile(const char* path, void* rendererObject)
{
    i32 dirLength;
    char* dir;
    i32 length;

    Atlas* atlas = 0;

    /* Get directory from atlas path. */
    const char* lastForwardSlash = strrchr(path, '/');
    const char* lastBackwardSlash = strrchr(path, '\\');
    const char* lastSlash = lastForwardSlash > lastBackwardSlash ? lastForwardSlash : lastBackwardSlash;
    if (lastSlash == path)
        lastSlash++; /* Never drop starting slash. */
    dirLength = (i32)(lastSlash ? lastSlash - path : 0);
    dir = MallocType(0, char, dirLength + 1);
    memcpy(dir, path, dirLength);
    dir[dirLength] = '\0';

    const char* fileData = globalPlatformServices->ReadEntireFile(&length, path);

    if (fileData)
        atlas = CreateAtlas(fileData, length, dir, rendererObject);
    else
        InvalidCodePath;

    globalPlatformServices->Free((void*)fileData);
    DeAlloc(0, dir);

    return atlas;
}

void Atlas_dispose(Atlas* self)
{
    AtlasRegion *region, *nextRegion;
    AtlasPage* page = self->pages;
    while (page)
    {
        AtlasPage* nextPage = page->next;
        AtlasPage_dispose(page);
        page = nextPage;
    }

    region = self->regions;
    while (region)
    {
        nextRegion = region->next;
        AtlasRegion_dispose(region);
        region = nextRegion;
    }

    DeAlloc(0, self);
}

AtlasRegion* Atlas_findRegion(const Atlas* self, const char* name)
{
    AtlasRegion* region = self->regions;
    while (region)
    {
        if (strcmp(region->name, name) == 0)
            return region;
        region = region->next;
    }
    return 0;
}

#endif //ATLAS_IMPL
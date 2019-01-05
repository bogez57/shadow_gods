#include "atlas.h"
#include <string.h>

Atlas* CreateAtlasFromFile(const char* path, void* rendererObject)
{
    i32 dirLength;
    char* dir;
    i64 length;
    const char* data;

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

    char* FileData = globalPlatformServices->ReadEntireFile(&length, path);

    /*
    if (data)
        atlas = spAtlas_create(data, length, dir, rendererObject);

    FREE(data);
    FREE(dir);
   */
    return atlas;
}
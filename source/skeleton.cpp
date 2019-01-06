/*
 Copyright (c) 2009, Dave Gamble
 Copyright (c) 2013, Esoteric Software

 Permission is hereby granted, dispose of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 */

#include "skeleton.h"
#include "json.h"

Skeleton CreateSkeleton(Atlas* atlas, const char* skeletonJson)
{
    Skeleton newSkeleton {};

    Json* root {};
    root = Json_create(skeletonJson);

    Json* skeletonJsonItem {};
    skeletonJsonItem = Json_getItem(root, "skeleton");

    if (skeletonJsonItem)
    {
        newSkeleton.width = Json_getFloat(skeletonJsonItem, "width", 0);
        newSkeleton.height = Json_getFloat(skeletonJsonItem, "height", 0);
    };

    return newSkeleton;
};

Skeleton CreateSkeletonUsingJsonFile(Atlas* atlas, const char* skeletonJsonFilePath)
{
    i32 length;
    Skeleton newSkeleton;

    const char* skeletonJson = globalPlatformServices->ReadEntireFile(&length, skeletonJsonFilePath);

    if (skeletonJson)
        newSkeleton = CreateSkeleton(atlas, skeletonJson);
    else
        InvalidCodePath;

    globalPlatformServices->Free((void*)skeletonJson);

    return newSkeleton;
};

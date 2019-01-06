#pragma once

#include "atlas.h"

struct Bone
{
    f32 x, y, rotation, scaleX, scaleY;
};

typedef enum
{
    ATTACHMENT_REGION,
    ATTACHMENT_BOUNDING_BOX,
    ATTACHMENT_MESH,
    ATTACHMENT_LINKED_MESH,
    ATTACHMENT_PATH,
    ATTACHMENT_POINT,
    ATTACHMENT_CLIPPING
} AttachmentType;

class Attachment
{
    const char* const name;
    const AttachmentType type;

protected:
    Attachment() = default;
};

struct Slot
{
    Attachment* const attachment;
};

struct Skeleton
{
    i32 boneCount;
    Array<Bone*> bones;
    Bone* const root;

    i32 slotCount;
    Array<Slot*> slots;
    Array<Slot*> drawOrder;
};
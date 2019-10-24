#ifndef COLLISION_DETECTION_INCLUDE
#define COLLISION_DETECTION_INCLUDE

struct AABB
{
    v2f minCorner{};
    v2f maxCorner{};
};

struct Collision_Box
{
    Collision_Box() = default;
    Collision_Box(v2f worldPos, v2f size);

    AABB bounds;
    v2f worldPos{};
    v2f size{};
};

void UpdateCollisionBoxWorldPos_BasedOnCenterPoint(Collision_Box&& oldCollisionBox, v2f newWorldPos);

#endif //COLLISION_DETECTION_INCLUDE

#ifdef COLLISION_DETECTION_IMPL

Collision_Box::Collision_Box(v2f worldPos, v2f size) :
    worldPos(worldPos),
    size(size)
{
    UpdateCollisionBoxWorldPos_BasedOnCenterPoint($(*this), worldPos);
}

void UpdateCollisionBoxWorldPos_BasedOnCenterPoint(Collision_Box&& collisionBox, v2f newWorldPos)
{
    collisionBox.bounds.minCorner.x = newWorldPos.x - collisionBox.size.x;
    collisionBox.bounds.minCorner.y = newWorldPos.y;
    collisionBox.bounds.maxCorner.x = newWorldPos.x + collisionBox.size.x;
    collisionBox.bounds.maxCorner.y = newWorldPos.y + collisionBox.size.y;

    collisionBox.worldPos = newWorldPos;
};

local_func b CheckForFighterCollisions_AxisAligned(Collision_Box fighter1Box, Collision_Box fighter2Box)
{
    // Exit returning NO intersection between bounding boxes
    if (fighter1Box.bounds.maxCorner.x < fighter2Box.bounds.minCorner.x || fighter1Box.bounds.minCorner.x > fighter2Box.bounds.maxCorner.x)
    {
        return false;
    };

    // Exit returning NO intersection between bounding boxes
    if (fighter1Box.bounds.maxCorner.y < fighter2Box.bounds.minCorner.y || fighter1Box.bounds.minCorner.y > fighter2Box.bounds.maxCorner.y)
    {
        return false;
    };

    // Else intersection and thus collision has occured!
    return true;
};

local_func v2f FindCenterOfRectangle(AABB rectangle)
{
    v2f centerPoint {};

    centerPoint.x = ((rectangle.minCorner.x + rectangle.maxCorner.x) / 2);
    centerPoint.y = ((rectangle.maxCorner.y + rectangle.maxCorner.y) / 2);

    return centerPoint;
};

#endif //COLLISION_DECTECTION_IMPL 
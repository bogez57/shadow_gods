Collision_Box UpdateCollisionBoxBasedOnCenterPoint(Collision_Box oldCollisionBox, v2f newCenterPosition)
{
    Collision_Box newCollisionBox { oldCollisionBox };

    newCollisionBox.bounds.minCorner.x = newCenterPosition.x - oldCollisionBox.size.x;
    newCollisionBox.bounds.minCorner.y = newCenterPosition.y;
    newCollisionBox.bounds.maxCorner.x = newCenterPosition.x + oldCollisionBox.size.x;
    newCollisionBox.bounds.maxCorner.y = newCenterPosition.y + oldCollisionBox.size.y;

    newCollisionBox.centerPoint = newCenterPosition;

    return newCollisionBox;
}

void InitCollisionBox_1(Collision_Box* collisionBox, v2f centerPoint, v2f size)
{
    collisionBox->size = size;

    *collisionBox = UpdateCollisionBoxBasedOnCenterPoint(*collisionBox, centerPoint);
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
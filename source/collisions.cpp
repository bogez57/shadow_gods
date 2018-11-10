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

local_func Collision_Box SetupCollisionBox(Collision_Box CollisionBox, v2f referencePosition, v2f BoxSize)
{
    CollisionBox.bounds.minCorner.x = referencePosition.x - BoxSize.x;
    CollisionBox.bounds.minCorner.y = referencePosition.y;
    CollisionBox.bounds.maxCorner.x = referencePosition.x + BoxSize.x;
    CollisionBox.bounds.maxCorner.y = referencePosition.y + BoxSize.y;

    CollisionBox.centerPoint = FindCenterOfRectangle(CollisionBox.bounds);

    CollisionBox.size = BoxSize;

    return CollisionBox;
};

local_func b CheckIfVertsInClockwiseOrder(const Dynam_Array<v2f>* vertsToCheck)
{
    f32 area {};
    for (i32 vecIndex { 0 }; vecIndex < vertsToCheck->size; ++vecIndex)
    {
        i32 nextVector = (vecIndex + 1) % vertsToCheck->size;
        area += vertsToCheck->At(vecIndex).x * vertsToCheck->At(nextVector).y;
        area -= vertsToCheck->At(nextVector).x * vertsToCheck->At(vecIndex).y;
    }

    if (area < 0)
    {
        //clockwise
        return true;
    }

    return false;
};

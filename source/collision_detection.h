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
    Collision_Box(v2f worldPos, v2f worldPosOffset,v2f size);

    AABB bounds;
    v2f worldPosOffset{};
    v2f worldPos{};
    v2f size{};
};

struct HitBox : public Collision_Box
{
    HitBox() = default;
    HitBox(v2f worldPos, v2f worldPosOffset,v2f size); 

    b isActive{false};
    char* boneName;
    f32 endTime{};
    f32 duration{};
    f32 timeUntilHitBoxIsActivated{};
    b timerStarted{false};
};

struct HurtBox : public Collision_Box
{
    HurtBox() = default;
    HurtBox(v2f worldPos, v2f worldPosOffset,v2f size); 
};

void UpdateCollisionBoxWorldPos_BasedOnCenterPoint(Collision_Box&& oldCollisionBox, v2f newWorldPos);
void UpdateHitBoxStatus(HitBox&& hitBox, f32 currentAnimRunTime);
b CheckForFighterCollisions_AxisAligned(Collision_Box& fighter1Box, Collision_Box fighter2Box);

#endif //COLLISION_DETECTION_INCLUDE


#ifdef COLLISION_DETECTION_IMPL

Collision_Box::Collision_Box(v2f worldPos, v2f worldPosOffset, v2f size) :
    worldPosOffset(worldPosOffset),
    size(size)
{
    this->worldPos = worldPos;
    UpdateCollisionBoxWorldPos_BasedOnCenterPoint($(*this), worldPos);
}

HurtBox::HurtBox(v2f worldPos, v2f worldPosOffset,v2f size) :
        Collision_Box(worldPos, worldPosOffset, size)
{}

HitBox::HitBox(v2f worldPos, v2f worldPosOffset,v2f size) :
        Collision_Box(worldPos, worldPosOffset, size)
{}

void UpdateCollisionBoxWorldPos_BasedOnCenterPoint(Collision_Box&& collisionBox, v2f newWorldPos)
{
    collisionBox.worldPos = newWorldPos + collisionBox.worldPosOffset;

    collisionBox.bounds.minCorner.x = collisionBox.worldPos.x - (collisionBox.size.x/2.0f);
    collisionBox.bounds.minCorner.y = collisionBox.worldPos.y - (collisionBox.size.y/2.0f);
    collisionBox.bounds.maxCorner.x = collisionBox.worldPos.x + (collisionBox.size.x/2.0f);
    collisionBox.bounds.maxCorner.y = collisionBox.worldPos.y + (collisionBox.size.y/2.0f);
};

local_func b CheckForFighterCollisions_AxisAligned(Collision_Box& fighter1Box, Collision_Box fighter2Box)
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

void UpdateHitBoxStatus(HitBox&& hitBox, f32 currentAnimRunTime)
{
    if(currentAnimRunTime > hitBox.timeUntilHitBoxIsActivated && currentAnimRunTime < hitBox.timeUntilHitBoxIsActivated + hitBox.duration)
            hitBox.endTime = globalPlatformServices->realLifeTimeInSecs + hitBox.duration;

    if(globalPlatformServices->realLifeTimeInSecs <= hitBox.endTime)
        hitBox.isActive = true;
    else
        hitBox.isActive = false;
}

local_func v2f FindCenterOfRectangle(AABB rectangle)
{
    v2f centerPoint {};

    centerPoint.x = ((rectangle.minCorner.x + rectangle.maxCorner.x) / 2);
    centerPoint.y = ((rectangle.maxCorner.y + rectangle.maxCorner.y) / 2);

    return centerPoint;
};

#endif //COLLISION_DECTECTION_IMPL 
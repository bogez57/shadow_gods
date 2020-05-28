#ifndef COLLISION_DETECTION_INCLUDE
#define COLLISION_DETECTION_INCLUDE

struct AABB
{
    v2 minCorner {};
    v2 maxCorner {};
};

struct Collision_Box
{
    Collision_Box() = default;
    Collision_Box(v2 worldPos, v2 worldPosOffset, v2 size);
    
    AABB bounds;
    v2 worldPosOffset {};
    v2 pos_worldSpace {};
    v2 size {};
};

struct HitBox : public Collision_Box
{
    HitBox() = default;
    HitBox(v2 worldPos, v2 worldPosOffset, v2 size);
    
    bool isActive { false };
    char* boneName;
    f32 endTime {};
    f32 duration {};
    f32 timeUntilHitBoxIsActivated {};
    bool timerStarted { false };
};

struct HurtBox : public Collision_Box
{
    HurtBox() = default;
    HurtBox(v2 worldPos, v2 worldPosOffset, v2 size);
};

void UpdateCollisionBoxWorldPos_BasedOnCenterPoint(Collision_Box&& oldCollisionBox, v2 newWorldPos);
void UpdateHitBoxStatus(HitBox&& hitBox, f32 currentAnimRunTime);
b CheckForFighterCollisions_AxisAligned(Collision_Box& fighter1Box, Collision_Box fighter2Box);

#endif //COLLISION_DETECTION_INCLUDE

#ifdef COLLISION_DETECTION_IMPL

Collision_Box::Collision_Box(v2 worldPos, v2 worldPosOffset, v2 size)
: worldPosOffset(worldPosOffset)
, size(size)
{
    this->pos_worldSpace = worldPos;
    UpdateCollisionBoxWorldPos_BasedOnCenterPoint($(*this), worldPos);
}

HurtBox::HurtBox(v2 worldPos, v2 worldPosOffset, v2 size)
: Collision_Box(worldPos, worldPosOffset, size)
{}

HitBox::HitBox(v2 worldPos, v2 worldPosOffset, v2 size)
: Collision_Box(worldPos, worldPosOffset, size)
{}

void UpdateCollisionBoxWorldPos_BasedOnCenterPoint(Collision_Box&& collisionBox, v2 newWorldPos)
{
    collisionBox.pos_worldSpace = newWorldPos + collisionBox.worldPosOffset;
    
    collisionBox.bounds.minCorner.x = collisionBox.pos_worldSpace.x - (collisionBox.size.x / 2.0f);
    collisionBox.bounds.minCorner.y = collisionBox.pos_worldSpace.y - (collisionBox.size.y / 2.0f);
    collisionBox.bounds.maxCorner.x = collisionBox.pos_worldSpace.x + (collisionBox.size.x / 2.0f);
    collisionBox.bounds.maxCorner.y = collisionBox.pos_worldSpace.y + (collisionBox.size.y / 2.0f);
};

local_func bool CheckForFighterCollisions_AxisAligned(Collision_Box& fighter1Box, Collision_Box fighter2Box)
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
    if (currentAnimRunTime > hitBox.timeUntilHitBoxIsActivated && currentAnimRunTime < hitBox.timeUntilHitBoxIsActivated + hitBox.duration)
        hitBox.endTime = globalPlatformServices->realLifeTimeInSecs + hitBox.duration;
    
    if (globalPlatformServices->realLifeTimeInSecs <= hitBox.endTime)
        hitBox.isActive = true;
    else
        hitBox.isActive = false;
}

local_func v2 FindCenterOfRectangle(AABB rectangle)
{
    v2 centerPoint {};
    
    centerPoint.x = ((rectangle.minCorner.x + rectangle.maxCorner.x) / 2);
    centerPoint.y = ((rectangle.maxCorner.y + rectangle.maxCorner.y) / 2);
    
    return centerPoint;
};

#endif //COLLISION_DECTECTION_IMPL
#pragma once

struct AABB
{
    v2f minCorner;
    v2f maxCorner;
};

struct Collision_Box
{
    AABB bounds;
    v2f centerPoint;
    v2f size;
};

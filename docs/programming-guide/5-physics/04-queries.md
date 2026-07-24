# Physics Queries

## Class Design: `PhysicsQueries2D`

```cpp
struct PhysicsRaycastHit2D {
    GameObject* gameObject = nullptr;
    float hitX = 0, hitY = 0;
    float normalX = 0, normalY = 1;
    float distance = 0;
};

struct PhysicsQueryFilter2D {
    std::uint16_t queryCategoryBits = 0xFFFF;
    std::uint16_t queryMaskBits = 0xFFFF;
    bool hitSolids = true;
    bool hitTriggers = true;
};
```

## Raycast World Convenience

```cpp
#include "spark/physics/PhysicsQueries2D.hpp"

PhysicsRaycastHit2D hit{};
if (PhysicsQueries2D::RaycastWorld2D(
        world, originX, originY, dirX, dirY, maxDist, filter, hit)) {
    GameObject* struck = hit.gameObject;
    (void)struck;
}
```

## Static Broadphase (Manual)

```cpp
StaticBroadPhase2D broad;
broad.Rebuild(world, 4.0F);  // cellWorldSize

PhysicsRaycastHit2D hit{};
PhysicsQueries2D::RaycastStatics2D(broad, ox, oy, dx, dy, maxDist, filter, hit);
```

## Overlap Queries

```cpp
Array<GameObject*> hits;
PhysicsQueries2D::QueryOverlapCircleDynamics2D(world, cx, cy, radius, filter, hits);
PhysicsQueries2D::QueryOverlapArcStatics2D(broad, cx, cy, radius,
    startAngleRad, sweepRad, filter, hits);
```

Arc queries support cone attacks and vision checks.

## Ground Check Pattern

```cpp
bool grounded = PhysicsQueries2D::RaycastWorld2D(
    world, footX, footY, 0.0F, -1.0F, 0.08F, filter, hit);
```

Or use `Rigidbody2DComponent::IsGrounded()` after simulation.

Next: [Physics 3D](05-physics-3d.md).

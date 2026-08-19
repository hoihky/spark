# Physics Queries

## Service: `PhysicsQueryWorld2D`

Owned by `PhysicsSubsystem::GetQueries2D()`. Rebuilds a static broad-phase once, then runs overlap and raycast queries without re-walking ECS statics on every call.

```cpp
#include "spark/physics/PhysicsSubsystem.hpp"

PhysicsSubsystem physics;

PhysicsQueryFilter2D filter{};
filter.queryCategoryBits = 1u << 0;
filter.hitSolids = true;
filter.hitTriggers = false;

Array<PhysicsQueryHit2D> hits;
physics.GetQueries2D().RebuildStatics(world);
physics.GetQueries2D().OverlapCircleStatics(footX, footY, 0.5F, filter, hits);
```

When using `PhysicsSubsystem`, `Simulate2D` keeps query cell size aligned with simulation. For standalone `PhysicsQueryWorld2D`, set `SetCellWorldSize` to match your `PhysicsWorld2D::GetBroadPhaseCellSize()`.

## Hit Types

```cpp
struct PhysicsRaycastHit2D {
    float distanceAlongRay = 0.0F;
    float hitX = 0.0F;
    float hitY = 0.0F;
    std::uint32_t staticColliderIndex = 0;
    GameObject* owner = nullptr;
};

struct PhysicsQueryFilter2D {
    std::uint16_t queryCategoryBits = 0xFFFF;
    std::uint16_t queryMaskBits = 0xFFFF;
    bool hitSolids = true;
    bool hitTriggers = true;
};
```

## Raycast

```cpp
PhysicsRaycastHit2D hit{};
physics.GetQueries2D().RebuildStatics(world);
if (physics.GetQueries2D().RaycastStatics(ox, oy, dx, dy, maxDist, filter, hit)) {
    GameObject* struck = hit.owner;
    (void)struck;
}
```

## One-Shot World Raycast (Legacy)

Free functions rebuild broad-phase internally each call — fine for tools, prefer `PhysicsQueryWorld2D` in hot paths:

```cpp
#include "spark/physics/PhysicsQueries2D.hpp"

PhysicsRaycastHit2D hit{};
RaycastWorld2D(world, ox, oy, dx, dy, maxDist, filter, hit);
```

## Static Broad-Phase (Manual)

```cpp
BroadPhase2D broad;
broad.Rebuild(world, 4.0F);

PhysicsRaycastHit2D hit{};
RaycastStatics2D(broad, ox, oy, dx, dy, maxDist, filter, hit);
```

## Dynamic Overlaps

```cpp
Array<PhysicsQueryHitDynamic2D> dynamics;
physics.GetQueries2D().OverlapCircleDynamics(world, cx, cy, radius, filter, nullptr, dynamics);
physics.GetQueries2D().OverlapArcDynamics(
        world, cx, cy, radius, dirX, dirY, halfAngleRad, filter, nullptr, dynamics);
```

Arc queries support cone attacks and vision checks.

## Ground Check Pattern

```cpp
bool grounded = RaycastWorld2D(world, footX, footY, 0.0F, -1.0F, 0.08F, filter, hit);
```

Or use `Rigidbody2DComponent::IsGrounded()` after simulation.

## 3D Raycasts (Scene / Mesh)

3D physics queries for triggers and overlaps are handled via `TriggerVolume3DComponent` signals and `PhysicsSubsystem::SimulateAll3D`. For **picking** and **hitscan weapons**, use scene raycast helpers:

```cpp
#include "spark/scene/MeshRaycast.hpp"
#include "spark/scene/Scene.hpp"

// Sphere hit test (FPS targets, simple pickups)
float hitT = 0.0F;
float bestT = maxDistance;
if (TryRaycastSphereWorld(rayOrigin, rayDir, centerWorld, radius, 1.0e-4F, bestT, hitT)) {
    // hit at rayOrigin + rayDir * hitT
}

// Mesh-accurate pick (scene editor, mesh targets)
SceneRaycastHit hit{};
SceneRaycastOptions opts{};
if (scene.RaycastPick(ray, hit, opts) && hit.object) {
    GameObject* picked = hit.object;
}

// Ray vs single mesh in world space
if (TryRaycastMeshWorld(rayOrigin, rayDir, *mesh, worldMatrix, tMin, tMax, outT)) { ... }
```

See `SceneEditor3DDemo` for editor picking and `8-3d-game/04-shooting.md` for FPS hitscan.

Next: [Physics 3D](05-physics-3d.md).

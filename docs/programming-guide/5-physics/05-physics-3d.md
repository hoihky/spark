# Physics 3D

Use `PhysicsSubsystem` and call each frame from `OnUpdate` (order matters):

```cpp
#include "spark/physics/PhysicsSubsystem.hpp"

PhysicsSubsystem physics;

void OnUpdate(const FrameTiming& timing) {
    physics.SimulateAll3D(world, timing);
}
```

`SimulateAll3D` runs, in order: rigidbody simulation, character controllers, trigger volumes. It also keeps `GetQueries3D()` broad-phase cell size aligned with `GetWorld3D()` (same as `Simulate3D`).

Or step individually:

```cpp
physics.Simulate3D(world, timing);
physics.SimulateCharacterControllers3D(world, timing);
physics.SimulateTriggerVolumes3D(world, timing);
```

## Class Design: `Rigidbody3DComponent`

```cpp
Vector3& GetVelocity();
void SetVelocity(const Vector3& v);
Vector3& GetAngularVelocity();
void AddImpulse(const Vector3& impulse);
```

Pair with `SphereCollider3DComponent` or `CapsuleCollider3DComponent` for dynamic props. If both sphere and capsule exist, **sphere is preferred** by the solver. `BoxCollider3DComponent` on a dynamic body is **not** simulated — use sphere/capsule proxies for boxes and crates.

## Throw Demo Pattern

```cpp
auto* ball = world.CreateGameObject();
ball->AddComponent<TransformComponent>()->SetTranslation(spawnPos);
ball->AddComponent<SphereCollider3DComponent>(0.25F);
auto* rb = ball->AddComponent<Rigidbody3DComponent>();
rb->SetVelocity(camera.Forward() * 18.0F);

physics.Simulate3D(world, timing);
```

See `PhysicsBallThrow3DDemo`. For carry/throw, layered triggers, queries, CCD, fixed step, and replay, see **`PhysicsShowcase3DDemo`** (SparkDemo **#29**, **P**).

## Character Controller (kinematic)

For FPS / third-person walkers, use `CharacterController3DComponent` instead of a dynamic rigidbody on the player:

```cpp
auto* cc = player->AddComponent<CharacterController3DComponent>(0.4F, Vector3{0,0.9F,0});
cc->SetMoveInput({inputX, 0.0F, inputZ});
physics.SimulateCharacterControllers3D(world, timing);
```

Objects with `CharacterController3DComponent` are **excluded** from dynamic rigidbody simulation.

Third-person movement should use the same horizontal forward as `FlyCamera` (yaw only): `{sin(yaw), 0, -cos(yaw)}` and `right = cross(forward, up)`. `PhysicsShowcase3DDemo` drives `SetMoveInput` from `camera.yaw` after mouse look.

## Carry and throw (handoff)

```cpp
#include "spark/physics/PhysicsBodyHandoff3D.hpp"

// Before PhysicsWorld3D::Simulate (SimulateAll3D does this internally at step start):
PhysicsBodyHandoff3D::SynchronizeAttachedBodies(world);

PhysicsBodyHandoff3D::AttachRigidbodyToCarrier(world, *crate, *carrierAnchor, localOffset);
physics.GetQueries3D().RebuildDynamics(world);

const Vector3 throwImpulse = forward * 7.0F + Vector3::UnitY * 2.8F;
PhysicsBodyHandoff3D::ReleaseRigidbody(*crate, throwImpulse);
physics.GetQueries3D().RebuildDynamics(world);
```

`ReleaseRigidbody` sets the body dynamic and applies `worldImpulse` as velocity (plus a small positional nudge). Pair with `PhysicsAttachedBody3DComponent` on the carried object.

## Trigger Volumes

```cpp
zone->AddComponent<TriggerVolume3DComponent>(TriggerVolume3DShape::Box, Vector3{3,2,3});
zone->GetComponent<TriggerVolume3DComponent>()->SetCategoryBits(
        CollisionFilter3D::LayerBit(PhysicsLayers3D::Trigger));
zone->GetComponent<TriggerVolume3DComponent>()->SetMaskBits(
        CollisionFilter3D::LayerBit(PhysicsLayers3D::Player));
physics.SimulateTriggerVolumes3D(world, timing);
// After simulation:
if (zone->GetComponent<TriggerVolume3DComponent>()->IsOverlapping(*player)) { ... }
```

Prefer `SimulateAll3D` so trigger overlap lists update before gameplay reads `IsOverlapping`.

## Fixed timestep

```cpp
auto& settings = physics.GetWorld3D().GetSettings();
settings.fixedSimulationTimestep = 1.0F / 60.0F;
settings.maxFixedSimulationStepsPerFrame = 8;
```

When `fixedSimulationTimestep` is zero, each frame uses the real `deltaTime` once. See [Physics 3D contract](07-physics-3d-contract.md).

## 3D queries

```cpp
physics.GetQueries3D().RebuildAll(world);

PhysicsQueryFilter3D filter{};
Array<PhysicsQueryHitDynamic3D> dynamics;
physics.GetQueries3D().OverlapSphereDynamics(center, radius, filter, nullptr, dynamics);

PhysicsRaycastHit3D hit{};
physics.GetQueries3D().Raycast(world, origin, direction, maxDistance, filter, nullptr, hit);
```

## 3D Simulation Pipeline

Per substep, `PhysicsWorld3D` runs:

1. `RigidbodyIntegrator3D` — gravity, damping, swept static + dynamic CCD, orientation
2. `ContactResolver3D` — static impulses, penetration separation, dynamic pair velocities
3. `JointSolver3D` — distance, hinge, spring constraints

Next: [Physics 3D contract](07-physics-3d-contract.md) · [Tips](06-tips.md).

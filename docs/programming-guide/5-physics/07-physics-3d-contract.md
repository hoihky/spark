# Physics 3D contract

This page describes what Spark’s 3D physics layer guarantees today: simulation order, query services, fixed timestep, and continuous collision detection (CCD). It is the reference for gameplay code that depends on balls, players, and environment interaction.

## Simulation entry points

| API | Responsibility |
| --- | --- |
| `PhysicsSubsystem::Simulate3D` | Dynamic rigidbodies (sphere/capsule) vs static box/capsule geometry, joints |
| `PhysicsSubsystem::SimulateCharacterControllers3D` | Kinematic character motors (sphere vs statics) |
| `PhysicsSubsystem::SimulateAll3D` | Rigidbodies, then characters, then trigger volumes (also syncs 3D query broad-phase cell size) |

Objects with `CharacterController3DComponent` are **not** integrated as rigidbodies. Dynamic rigidbodies require `Rigidbody3DComponent` + `SphereCollider3DComponent` or `CapsuleCollider3DComponent`.

`BoxCollider3DComponent` and `MeshCollider3DComponent` participate in **static** baking and triggers, but are **not** integrated as dynamic rigidbodies today. For crate-like props, use a **cube mesh** with a **sphere or capsule** collider sized to fit the visual (see `PhysicsShowcase3DDemo`).

## Fixed timestep (A4b)

`PhysicsWorld3DSettings::fixedSimulationTimestep` (seconds) enables an internal accumulator on `PhysicsWorld3D`. When greater than zero, each frame runs up to `maxFixedSimulationStepsPerFrame` steps at that fixed delta. When zero, the frame uses `FrameTiming::deltaTimeSeconds` once (legacy behavior).

`substeps` still subdivide **each** simulation step for internal stability (`stepDelta / substeps`).

```cpp
auto& settings = physics.GetWorld3D().GetSettings();
settings.fixedSimulationTimestep = 1.0F / 60.0F;
settings.maxFixedSimulationStepsPerFrame = 6;
```

Call `PhysicsWorld3D::ResetFixedStepAccumulator()` after teleporting the whole scene to avoid a large catch-up burst.

## Queries (E1)

`PhysicsSubsystem::GetQueries3D()` owns `PhysicsQueryWorld3D`:

1. `RebuildStatics(world)` — baked static colliders (boxes/capsules from the ECS bake pipeline).
2. `RebuildDynamics(world)` — dynamic rigidbodies plus character controllers (sphere proxy).
3. `RebuildAll(world)` — both.

| Method | Targets |
| --- | --- |
| `OverlapSphereStatics` | Static geometry |
| `OverlapSphereDynamics` | Dynamic rigidbodies + character controllers |
| `OverlapSphere` | Rebuilds all, then fills static and dynamic hit arrays |
| `RaycastStatics` / `RaycastDynamics` | Narrow-phase ray vs one category |
| `Raycast` | Closest hit among statics and dynamics |

Filtering uses the same bitmask rules as simulation (`PhysicsQueryFilter3D` + per-collider category/mask). Pass `GameObject* ignore` on dynamic queries to skip an instigator (e.g. the projectile owner).

`PhysicsRaycastHit3D::isDynamicHit` is true when `staticColliderIndex == kNoStaticColliderIndex`.

`Simulate3D` and `SimulateAll3D` align `PhysicsQueryWorld3D` broad-phase cell size with `PhysicsWorld3D::GetBroadPhaseCellSize()`. After changing cell size or spawning/destroying dynamics, call `RebuildDynamics` (or `RebuildAll`) before raycasts/overlaps.

Bodies with an active `PhysicsAttachedBody3DComponent` binding are omitted from the dynamic query catalog until released.

## Continuous collision detection

| Setting | Effect |
| --- | --- |
| `sweptStaticCcdBinaryIterations` | Binary search along translation vs **static** colliders (broad-phase along segment) |
| `sweptDynamicCcdBinaryIterations` | Binary search vs **other dynamic** colliders at interpolated poses |

Set either counter to `0` to disable that pass. CCD runs inside `RigidbodyIntegrator3D` before contact resolution.

## Layer indices

Use `PhysicsLayers3D` and `CollisionFilter3D::LayerBit` on colliders, character controllers, and query filters. See [Colliders](03-colliders.md).

## Tier 2 — Trust and handoff

| Feature | API |
| --- | --- |
| **E9** Trigger stay + layers | `TriggerVolume3DComponent` category/mask; `Physics3DTriggerStay` signal |
| **E8** Unified raycast | `ScenePhysicsRaycast3D` + `PhysicsQueryWorld3D` |
| **E6** Contact hooks | `PhysicsContactBroadcaster3D`, `PhysicsContactListener3DComponent`, `Physics3DContact` |
| **E7** Carry / throw | `PhysicsBodyHandoff3D`, `PhysicsAttachedBody3DComponent` |
| **E10 / A4c** Replay domain | `PhysicsSimulationDomain3D`, `PhysicsReplayRingBuffer3D`, `PhysicsSubsystem::PlaybackPhysics3DFrame` |

`PhysicsSimulationDomain3D::AdvanceMode::Paused` skips live rigidbody integration inside `PhysicsWorld3D::Simulate`. `SimulateAll3D` still runs character controllers and trigger volumes unless you step APIs individually.

### Trigger overlap state

`TriggerVolume3DComponent::IsOverlapping` reads the overlap list updated by `SimulateTriggerVolumes3D`. Query it **after** `SimulateAll3D` (or `SimulateTriggerVolumes3D`) in the same frame.

### Character controller vs fixed step

`CharacterControllerWorld3D` uses `FrameTiming::deltaTimeSeconds` each frame. `PhysicsWorld3D` can use `fixedSimulationTimestep` independently. Toggle both in **`PhysicsShowcase3DDemo`** (**F2**) to compare behavior.

## Hands-on reference

SparkDemo **#29** / hotkey **P** — `PhysicsShowcase3DDemo` (`include/spark/demo/PhysicsShowcase3DDemo.hpp`):

| Input | Feature |
| --- | --- |
| WASD | `CharacterController3DComponent` + camera-yaw-relative velocity |
| **E** | `PhysicsBodyHandoff3D` carry / throw |
| **F** | Fast sphere projectile (swept CCD vs thin wall) |
| LMB | `ScenePhysicsRaycast3D` unified mesh + physics pick |
| **F2** | Fixed vs variable physics timestep |
| **F3** | Pause rigidbody integration (`PhysicsSimulationDomain3D`) |
| **F4** / **F5** | Record / playback replay buffer |
| Goal zone | Layered `TriggerVolume3DComponent` stay counter |

Regression: `PhysicsShowcaseRegressionTest.CharacterControllerWalksOnFloor` in `tests/physics/PhysicsWorld3DIntegrationTest.cpp`.

## Related

- [Physics 3D overview](05-physics-3d.md)
- [Queries (2D patterns)](04-queries.md)

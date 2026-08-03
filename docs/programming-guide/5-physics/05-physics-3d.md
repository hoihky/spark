# Physics 3D

Use `PhysicsSubsystem` and call each frame from `OnUpdate` (order matters):

```cpp
#include "spark/physics/PhysicsSubsystem.hpp"

PhysicsSubsystem physics;

void OnUpdate(const FrameTiming& timing) {
    physics.SimulateAll3D(world, timing);
}
```

`SimulateAll3D` runs, in order: rigidbody simulation, character controllers, trigger volumes.

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

Pair with `SphereCollider3DComponent` or `CapsuleCollider3DComponent` for dynamic props. If both sphere and capsule exist, **sphere is preferred** by the solver.

## Throw Demo Pattern

```cpp
auto* ball = world.CreateGameObject();
ball->AddComponent<TransformComponent>()->SetTranslation(spawnPos);
ball->AddComponent<SphereCollider3DComponent>(0.25F);
auto* rb = ball->AddComponent<Rigidbody3DComponent>();
rb->SetVelocity(camera.Forward() * 18.0F);

physics.Simulate3D(world, timing);
```

See `PhysicsBallThrow3DDemo`.

## Character Controller (kinematic)

For FPS / third-person walkers, use `CharacterController3DComponent` instead of a dynamic rigidbody on the player:

```cpp
auto* cc = player->AddComponent<CharacterController3DComponent>(0.4F, Vector3{0,0.9F,0});
cc->SetMoveInput({inputX, 0.0F, inputZ});
physics.SimulateCharacterControllers3D(world, timing);
```

Objects with `CharacterController3DComponent` are **excluded** from dynamic rigidbody simulation.

## Trigger Volumes

```cpp
zone->AddComponent<TriggerVolume3DComponent>(TriggerVolume3DShape::Box, Vector3{3,2,3});
physics.SimulateTriggerVolumes3D(world, timing);
```

## 3D Simulation Pipeline

Per substep, `PhysicsWorld3D` runs:

1. `RigidbodyIntegrator3D` — gravity, damping, swept CCD, orientation
2. `ContactResolver3D` — static impulses, penetration separation, dynamic pair velocities
3. `JointSolver3D` — distance, hinge, spring constraints

Next: [Tips](06-tips.md).

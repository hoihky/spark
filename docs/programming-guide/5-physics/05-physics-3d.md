# Physics 3D

Call each frame from `OnUpdate` (order matters):

```cpp
SimulatePhysics3D(world, timing, settings);
SimulateCharacterControllers3D(world, timing);
SimulateTriggerVolumes3D(world, timing);
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

SimulatePhysics3D(world, timing, settings);
```

See `PhysicsBallThrow3DDemo`.

## Character Controller (kinematic)

For FPS / third-person walkers, use `CharacterController3DComponent` instead of a dynamic rigidbody on the player:

```cpp
auto* cc = player->AddComponent<CharacterController3DComponent>(0.4F, Vector3{0,0.9F,0});
cc->SetMoveInput({inputX, 0.0F, inputZ});
SimulateCharacterControllers3D(world, timing);
```

Objects with `CharacterController3DComponent` are **excluded** from `SimulatePhysics3D` dynamic sphere resolution.

## Trigger Volumes

```cpp
auto* zone = world.CreateGameObject();
zone->AddComponent<TriggerVolume3DComponent>(TriggerVolume3DShape::Box, Vector3{4,2,4});
zone->GetComponent<TriggerVolume3DComponent>()->SetOnEnter(
    [](GameObject& other) { (void)other; });
```

Also emits `SignalId::Physics3DTriggerEnter` / `Exit` to sibling components.

## Joints and springs

```cpp
PhysicsWorld3DSettings settings{};
settings.jointIterations = 8;
SimulatePhysics3D(world, timing, settings);

// Distance + hinge (position constraints):
ballA->AddComponent<DistanceJoint3DComponent>(ballB, 2.0F);
crateA->AddComponent<HingeJoint3DComponent>(crateB);

// Spring (velocity-based, needs sphere colliders on both ends):
bob->AddComponent<SpringJoint3DComponent>(anchor, 4.0F);
```

## Spatial Hash

`SpatialHashGrid3D` accelerates static queries — rebuilt during simulation internally.

## ECS vs demo cameras

The FPS sample may use **FlyCamera** without rigidbody. For shipped games prefer `CameraComponent` + `SpringArm3DComponent` + `CameraFollow3DComponent` (see [Cameras in 3D](../3-3d-graphics/02-cameras-3d.md)).

See [Game Component Reference](../1-overview-architecture/07-game-component-reference.md#physics-3d).

Next: [Tips and Patterns](06-tips.md).

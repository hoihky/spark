# Physics 3D

3D simulation handles **rigidbodies**, **character controllers**, **joints**, **CCD**, and **trigger volumes**. Entry: `PhysicsSubsystem::SimulateAll3D` or `Simulate3D`.

**Demo:** SparkDemo **#29** (`PhysicsShowcase3DDemo`) — replay key **P**.

## Concept: rigidbody stack

### C++

```cpp
#include "spark/ecs/components/physics/3d/Rigidbody3DComponent.hpp"

crate->AddComponent<Rigidbody3DComponent>()->SetMass(2.5F);
crate->AddComponent<BoxCollider3DComponent>(Vector3{0.5F, 0.5F, 0.5F});
```

### C\#

```csharp
crate.AddRigidbody3D().SetMass(2.5f);
crate.AddBoxCollider3D(new Vector3(0.5f, 0.5f, 0.5f));
```

### Python

```python
crate.get_or_add_default_component(ComponentKind.Rigidbody3D)
```

## Concept: character controller (FPS)

### C++

```cpp
player->AddComponent<CharacterController3DComponent>();
player->AddComponent<CapsuleCollider3DComponent>(0.4F, 1.8F);

physics.GetCharacterController3D().Simulate(world, timing);
// or SimulateAll3D includes CC when configured
```

### C\#

```csharp
player.GetOrAddComponent<CharacterController3DComponent>();
```

### Python

```python
player.get_or_add_default_component(ComponentKind.CharacterController3D)
```

## Concept: 3D triggers

### C++

```cpp
zone->AddComponent<TriggerVolume3DComponent>();
// sibling component OnSignal Physics3DTriggerEnter
```

Signals are **sibling-only** — put listener on same `GameObject`.

### C\#

```csharp
zone.AddTriggerVolume3D();
```

### Python

```python
zone.get_or_add_default_component(ComponentKind.TriggerVolume3D)
```

## Swept CCD

Fast bullets: configure `PhysicsWorld3DSettings::sweptStaticCcdBinaryIterations` and throw with velocity — showcase demo.

### C++

```cpp
physics.GetWorld3D().GetSettings().substeps = 2;
```

### C\#

```csharp
_physics.GetWorld3D().GetSettings().Substeps = 2;
```

### Python

```python
pass
```

Next: [Joints and Tips](06-tips.md).

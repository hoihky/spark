# Joints and Tips

2D/3D distance and hinge joints attach to `Rigidbody` bodies. Tune iteration counts when chains jitter.

## Concept: distance joint 2D

### C++

```cpp
#include "spark/ecs/components/physics/2d/DistanceJoint2DComponent.hpp"

bridge->AddComponent<DistanceJoint2DComponent>()
    ->SetConnectedBody(otherRb)
    ->SetRestLength(3.0F);
```

### C\#

```csharp
bridge.GetOrAddComponent<DistanceJoint2DComponent>();
```

### Python

```python
bridge.get_or_add_default_component(ComponentKind.DistanceJoint2D)
```

## Concept: hinge joint 3D (door)

### C++

```cpp
door->AddComponent<HingeJoint3DComponent>()->SetAxisLocal({0, 1, 0});
```

### C\#

```csharp
door.AddHingeJoint3D();
```

### Python

```python
door.get_or_add_default_component(ComponentKind.HingeJoint3D)
```

## Tips

| Issue | Fix |
|-------|-----|
| Tunneling at high speed | 3D substeps + CCD |
| Jitter on stacks | More `resolveIterations`, sleep thresholds |
| Missed raycasts | `RebuildStatics` after spawning level |
| Triggers not firing | Component on trigger object; sibling `OnSignal` |
| Character snags | Tweak capsule radius, controller step height |

## Determinism / replay

`PhysicsShowcase3DDemo` records input + physics state — pattern for QA replays.

### C++

```cpp
// See demo Recording subsystem — fixed timestep optional via settings.
```

### C\#

```csharp
// Mirror demo FixedStep in template projects.
```

### Python

```python
pass
```

Next: [Physics 3D contract](07-physics-3d-contract.md) → [Layers & filters](08-collision-layers-and-filters.md) → [Triggers & showcase](09-triggers-contacts-and-showcase.md).

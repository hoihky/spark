# Triggers, contacts, and showcase patterns

SparkDemo **#29 / P** exercises **3D triggers (enter/stay/exit)**, **contact reports**, **joints**, **CCD**, and **replay** hooks. SparkDemo **#7** adds **2D joints** and broad-phase stress.

**Contract summary:** [Physics 3D contract](07-physics-3d-contract.md) · **Layers:** [Collision layers](08-collision-layers-and-filters.md).

## Trigger volumes (3D)

`TriggerVolume3DComponent` is non-blocking. `PhysicsSubsystem::SimulateAll3D` runs `SimulateTriggerVolumes3D`, which fires **callbacks** and **signals** when probe bodies overlap (layer-filtered).

| Event | Signal | Callback |
|-------|--------|----------|
| Enter | `Physics3DTriggerEnter` | `SetOnEnter` |
| Stay | `Physics3DTriggerStay` | `SetOnStay` |
| Exit | `Physics3DTriggerExit` | `SetOnExit` |

Payload: `payload.ptr` = other `GameObject*`.

---

## Concept: goal zone (showcase)

### C++

```cpp
#include "spark/ecs/components/physics/3d/TriggerVolume3DComponent.hpp"

auto* goalTrigger = goal->AddComponent<TriggerVolume3DComponent>(
    TriggerVolume3DShape::Box, Vector3{1.5F, 2.0F, 1.5F});
goalTrigger->SetCategoryBits(CollisionFilter3D::LayerBit(PhysicsLayers3D::Trigger));
goalTrigger->SetMaskBits(CollisionFilter3D::LayerBit(PhysicsLayers3D::Player));

goalTrigger->SetOnEnter([](GameObject& other) {
    if (other.GetName() == Utf8String("Player")) {
        // level complete
    }
});
```

Signals are delivered to **sibling** `GameComponent` instances on the **same** `GameObject` as the trigger.

### C\#

```csharp
goal.AddTriggerVolume3D(TriggerVolume3DShape.Box, halfExtents);
```

### Python

```python
goal.get_or_add_default_component(ComponentKind.TriggerVolume3D)
```

---

## Concept: contact listener (3D)

`PhysicsContactListener3DComponent` receives `PhysicsContactReport3D` when the broad-phase resolver generates contacts. Useful for impact SFX, damage, or breakables without polling overlap every frame.

### C++

```cpp
#include "spark/ecs/components/physics/3d/PhysicsContactListener3DComponent.hpp"

crate->AddComponent<PhysicsContactListener3DComponent>()
    ->SetContactHandler([](const PhysicsContactReport3D& report) {
        if (report.penetration > 0.05F) {
            // play clang
        }
    });
```

Contacts fire during `Simulate3D` / `SimulateAll3D`, not from queries.

### C\#

```csharp
crate.AddPhysicsContactListener3D();
```

### Python

```python
crate.get_or_add_default_component(ComponentKind.PhysicsContactListener3D)
```

---

## Concept: carry / attach handoff

`PhysicsAttachedBody3DComponent` + `PhysicsBodyHandoff3D` move a dynamic body between **simulated** and **kinematic carry** states (showcase **P**). Release applies impulse for throws.

### C++

```cpp
#include "spark/ecs/components/physics/3d/PhysicsAttachedBody3DComponent.hpp"

carriedCrate->AddComponent<PhysicsAttachedBody3DComponent>();
// Handoff API on PhysicsSubsystem — see PhysicsShowcase3DDemo input bindings.
```

Prefer `SimulateAll3D` so character motor + triggers + query sync run together.

### C\#

```csharp
// Mirror showcase carry keys in template game.
```

### Python

```python
pass
```

---

## Concept: 2D joints (**#7**)

`BroadPhase2DDemo` / platformer bridges use **`DistanceJoint2DComponent`** and **`HingeJoint2DComponent`** between `Rigidbody2D` bodies.

### C++

```cpp
#include "spark/ecs/components/physics/2d/DistanceJoint2DComponent.hpp"

rope->AddComponent<DistanceJoint2DComponent>()
    ->SetConnectedBody(otherBody)
    ->SetRestLength(3.0F);
```

Tune `PhysicsWorld2D` iteration counts when chains jitter — [Joints and tips](06-tips.md).

### C\#

```csharp
rope.AddDistanceJoint2D().SetRestLength(3f);
```

### Python

```python
rope.get_or_add_default_component(ComponentKind.DistanceJoint2D)
```

---

## Concept: CCD and replay (showcase)

Fast thin props: raise **`PhysicsWorld3DSettings::substeps`** and swept static CCD iterations before enabling max throw speed.

### C++

```cpp
PhysicsWorld3DSettings& s = physics.GetWorld3D().GetSettings();
s.substeps = 2;
s.sweptStaticCcdBinaryIterations = 6;
```

Showcase records input + state for deterministic QA replays — copy the demo’s fixed-step block when building regression tests.

### C\#

```csharp
_physics.GetWorld3D().GetSettings().Substeps = 2;
```

### Python

```python
pass
```

---

## Integration checklist (3D sandbox)

| Step | Call / component |
|------|------------------|
| 1 | Own `PhysicsSubsystem`, configure gravity + substeps |
| 2 | Assign **layers** on every collider + controller |
| 3 | `SimulateAll3D` after gameplay input |
| 4 | `RebuildStatics` once per level load; dynamics after bulk spawn |
| 5 | Triggers on goal/pickup objects with mask = `Player` |
| 6 | Optional contact listeners on destructibles |
| 7 | Queries for interaction ray separate from render pick |

---

## Debugging

| Symptom | Check |
|---------|--------|
| Stay never fires | `SetOnStay` registered? probe still inside volume |
| Double enter | Destroy/recreate trigger without exit |
| Contact spam | Filter by `penetration` or debounce time |
| Joint explosion | Rest length vs initial distance; more solver iterations |

Part 5 complete → **Part 6**: [Sound Engine](../6-sound/01-sound-engine.md).

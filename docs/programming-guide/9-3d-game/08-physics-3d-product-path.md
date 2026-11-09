# Physics 3D product path

SparkDemo **#29 / hotkey P** (`PhysicsShowcase3DDemo`) is the blessed reference for **3D physics integration**: layer masks, triggers, character controller handoff, queries, CCD, and optional replay.

**Part 5:** [Physics 3D](../5-physics/05-physics-3d.md) · **Part 7:** [Physics 3D components](../7-game-components-usage/07-physics-3d.md).

## Goals the showcase demonstrates

| Topic | Behavior |
|-------|----------|
| Fixed step vs variable | Configurable substeps / display sync |
| Layers & filters | Category/mask on colliders and queries |
| Triggers | `TriggerVolume3DComponent` enter/stay/exit |
| Character vs rigidbody | CC3D riding / pushing dynamic crates |
| Queries | Raycast, overlap, scene editor parity |
| Carry / interact | Gameplay-specific joint or parenting patterns in demo |
| CCD | Fast thin objects without tunneling |

Treat the demo source as the checklist when porting patterns into your `Game::OnUpdate`.

## Concept: subsystem ownership

### C++

```cpp
PhysicsSubsystem physics;

void OnAttach(IEngineContext& ctx) override {
    physics.GetWorld3D().GetSettings().gravity = Vector3{0.0F, -9.81F, 0.0F};
    physics.GetWorld3D().GetSettings().Substeps = 2;
}

void OnUpdate(const FrameTiming& timing, IEngineContext& ctx) override {
    TickGameplayInput(timing, ctx);
    Game::OnUpdate(timing, ctx);
    physics.SimulateAll3D(GetWorld(), timing);
}
```

### C\#

```csharp
_physics.GetWorld3D().GetSettings().Gravity = new Vector3(0, -9.81f, 0);
_physics.SimulateAll3D(World, timing);
```

### Python

```python
context.physics.simulate_all_3d(self.world, timing)
```

`SimulateAll3D` is **not** called automatically by `Game::OnUpdate` — same rule as 2D ([Physics overview](../5-physics/01-physics-overview.md)).

## Concept: trigger volumes

### C++

```cpp
#include "spark/ecs/components/physics/3d/TriggerVolume3DComponent.hpp"

zone->AddComponent<TriggerVolume3DComponent>(
    TriggerVolume3DShape::Box, Vector3{2.0F, 2.0F, 2.0F});
```

Listen via contact signals or gameplay components that poll overlap state — see showcase for wiring.

### C\#

```csharp
zone.AddTriggerVolume3D(TriggerVolume3DShape.Box, new Vector3(2f, 2f, 2f));
```

### Python

```python
zone.get_or_add_default_component(ComponentKind.TriggerVolume3D)
```

## Concept: queries before sim

For aim assist / foot IK / editor picking, run **`PhysicsQueries3D`** (or mesh raycasts) with the same layer masks as gameplay:

### C++

```cpp
if (physics.GetQueries3D().Raycast(world, origin, dir, maxDist, out hit)) {
    // react before applying impulses
}
```

### C\#

```csharp
if (_physics.GetQueries3D().Raycast(World, origin, dir, maxDist, out var hit)) { }
```

## Handoff with character controller

When the player rides elevators or stands on dynamic platforms:

1. Update CC3D intent in gameplay.  
2. `SimulateCharacterControllers3D`.  
3. `SimulateAll3D`.  
4. Reconcile camera attach targets.

Details: [Character camera](07-character-and-third-person.md).

## When to stay on hitscan (FPS template)

Part 9 ch. **04** uses analytic rays without rigidbody bullets. Adopt physics projectiles from [Extending](06-extending.md) once layers and CCD are understood from this chapter.

## Testing mindset

Use showcase replay / fixed-step modes to reproduce bug reports. Add your game's critical interactions (stairs, moving platforms, thrown objects) as small scenes before shipping open worlds.

Next: [Maze AI and perception](09-maze-ai-and-perception.md).

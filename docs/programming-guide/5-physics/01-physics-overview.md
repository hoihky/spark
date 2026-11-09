# Physics Overview

Physics is **explicit**: neither `Game::OnUpdate` nor `Engine` calls `Simulate2D` for you. Own a `PhysicsSubsystem`, configure gravity once, and step every frame after applying input.

**Guides:** [Physics 2D components](../7-game-components-usage/06-physics-2d.md), [Physics 3D](../7-game-components-usage/07-physics-3d.md) · **Demo #29** Physics 3D showcase.

## Concept: own and step `PhysicsSubsystem`

### C++

```cpp
#include "spark/physics/PhysicsSubsystem.hpp"

PhysicsSubsystem physics;

void OnAttach(IEngineContext&) override {
    physics.GetWorld2D().GetSettings().gravityY = -30.0F;
    physics.GetWorld3D().GetSettings().substeps = 2;
}

void OnUpdate(const FrameTiming& t, IEngineContext&) override {
    Game::OnUpdate(t, context);
    physics.Simulate2D(GetWorld(), t);
    physics.SimulateAll3D(GetWorld(), t);
}
```

### C\#

```csharp
private readonly PhysicsSubsystem _physics = new();

public override void OnUpdate(FrameTiming t, IEngineContext ctx)
{
    base.OnUpdate(t, ctx);
    _physics.Simulate2D(World, t);
    _physics.SimulateAll3D(World, t);
}
```

### Python

```python
def on_update(self, timing, context):
    super().on_update(timing, context)
    context.physics.simulate_2d(self.world, timing)
    context.physics.simulate_all_3d(self.world, timing)
```

## Services on `PhysicsSubsystem`

| Accessor | Role |
|----------|------|
| `GetWorld2D()` | 2D integrate + contacts |
| `GetWorld3D()` | 3D rigid bodies + CCD |
| `GetQueries2D()` | Overlap / raycast 2D |
| `GetQueries3D()` | Overlap / raycast 3D |
| `GetCharacterController3D()` | FPS motor |
| `GetTriggerVolumes3D()` | 3D trigger dispatch |

Include umbrella: `#include "spark/physics/Physics.hpp"`.

## Broad-phase cell size

### C++

```cpp
physics.SetBroadPhaseCellSize2D(8.0F);
// 3D default cell 2 — PhysicsWorld3D::DefaultBroadPhaseCellSize
```

After large static edits, `GetQueries3D().RebuildStatics(world)` before raycasts.

### C\#

```csharp
_physics.SetBroadPhaseCellSize2D(8f);
```

### Python

```python
context.physics.set_broadphase_cell_size_2d(8.0)
```

## 2D pipeline (one step)

Broad-phase bake → integrate → contact resolve → triggers → joints.

## Legacy APIs

`SimulatePhysics2D` / `SimulatePhysics3D` are **deprecated** — recreate settings each call.

## Settings snapshot

| 2D field | Default |
|----------|---------|
| `gravityY` | -32 |
| `maxFallSpeed` | 46 |
| `jointIterations` | 4 |

| 3D field | Default |
|----------|---------|
| `gravityY` | -24 |
| `substeps` | 1 |
| `resolveIterations` | 8 |

3D contract for triggers/CCD/replay: [07-physics-3d-contract.md](07-physics-3d-contract.md).

Next: [Rigidbody 2D](02-rigidbody-2d.md).

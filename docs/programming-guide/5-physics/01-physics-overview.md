---
title: Physics Overview
order: 1
---

# Physics Overview

## Design: Explicit Simulation

Spark physics is **not** auto-ticked in `Game::OnUpdate`. Your game calls simulation explicitly:

```cpp
#include "spark/physics/PhysicsWorld2D.hpp"
#include "spark/physics/PhysicsWorld3D.hpp"

PhysicsWorld2DSettings settings2d{};
settings2d.gravityY = -30.0F;
settings2d.maxFallSpeed = 42.0F;
SimulatePhysics2D(world, timing, settings2d);

PhysicsWorld3DSettings settings3d{};
settings3d.gravityY = -24.0F;
settings3d.substeps = 1;
settings3d.resolveIterations = 8;
SimulatePhysics3D(world, timing, settings3d);
```

## Settings Structs

**2D** (`PhysicsWorld2DSettings`):

| Field | Default | Meaning |
|-------|---------|---------|
| `gravityY` | `-32` | Vertical acceleration |
| `maxFallSpeed` | `46` | Terminal velocity clamp |
| `resolveDynamicVsDynamic` | `false` | Dynamic-dynamic resolution |

**3D** (`PhysicsWorld3DSettings`):

| Field | Default |
|-------|---------|
| `gravityY` | `-24` |
| `substeps` | `1` |
| `resolveIterations` | `8` |
| `sweptStaticCcdBinaryIterations` | `10` |

## Recommended Update Order

```cpp
void OnUpdate(const FrameTiming& t, IEngineContext& ctx) override {
    ReadInputAndSetForces();
    SimulatePhysics2D(GetWorld(), t, physSettings);
    PostPhysicsGameplay();
    Game::OnUpdate(t, ctx);
}
```

Next: [Rigidbody 2D](5-physics/02-rigidbody-2d.html).

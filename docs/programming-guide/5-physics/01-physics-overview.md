# Physics Overview

## Recommended API: `PhysicsSubsystem`

Spark physics is **not** auto-ticked in `Game::OnUpdate`. Your game owns a `PhysicsSubsystem`, configures it once, and steps explicitly each frame:

```cpp
#include "spark/physics/Physics.hpp"

PhysicsSubsystem physics;

void OnLoad() {
    physics.GetWorld2D().GetSettings().gravityY = -30.0F;
    physics.GetWorld2D().GetSettings().maxFallSpeed = 42.0F;
    physics.GetWorld3D().GetSettings().substeps = 2;
}

void OnUpdate(const FrameTiming& timing) override {
    ApplyPlayerForces();
    physics.Simulate2D(GetWorld(), timing);
    physics.SimulateAll3D(GetWorld(), timing);  // rigidbodies + character + triggers
    PostPhysicsGameplay();
}
```

`PhysicsSubsystem` owns:

| Service | Role |
|---------|------|
| `GetWorld2D()` | 2D rigidbody simulation |
| `GetWorld3D()` | 3D rigidbody simulation |
| `GetQueries2D()` | Static/dynamic overlap and raycasts |
| `GetCharacterController3D()` | Kinematic character motors |
| `GetTriggerVolumes3D()` | 3D trigger enter/exit |

Include everything with `#include "spark/physics/Physics.hpp"`.

### Broad-phase cell size (2D)

Simulation and queries share the same uniform-cell grid when using `PhysicsSubsystem`:

```cpp
physics.SetBroadPhaseCellSize2D(8.0F);  // sets world + query cell size
// or: physics.GetWorld2D().SetBroadPhaseCellSize(8.0F) before Simulate2D (queries sync each step)
```

Default cell size is **4** world units (`PhysicsWorld2D::DefaultBroadPhaseCellSize`).

## Legacy Free Functions

`SimulatePhysics2D` / `SimulatePhysics3D` still compile but are **deprecated** — they recreate settings every call. Prefer `PhysicsSubsystem` or holding a `PhysicsWorld2D` / `PhysicsWorld3D` member.

## Settings Structs

**2D** (`PhysicsWorld2DSettings`):

| Field | Default | Meaning |
|-------|---------|---------|
| `gravityY` | `-32` | Vertical acceleration |
| `maxFallSpeed` | `46` | Terminal velocity clamp |
| `resolveDynamicVsDynamic` | `false` | Dynamic-dynamic positional separation |
| `jointIterations` | `4` | Distance/hinge joint soft iterations |

**3D** (`PhysicsWorld3DSettings`):

| Field | Default |
|-------|---------|
| `gravityY` | `-24` |
| `substeps` | `1` |
| `resolveIterations` | `8` |
| `sweptStaticCcdBinaryIterations` | `10` |
| `jointIterations` | `0` |

## Internal Pipeline (2D)

Each `PhysicsWorld2D::Simulate` step runs:

1. **Broad-phase** — `ColliderBakePipeline2D` bakes static `Collider2D` snapshots
2. **Integrator** — gravity + velocity integration (`RigidbodyIntegrator2D`)
3. **Contact resolver** — static and dynamic–dynamic contacts (`ContactResolver2D`)
4. **Triggers** — `Physics2DTriggerOverlap` signals (`TriggerDispatcher2D`)
5. **Joints** — distance/hinge (`JointSolver2D`)

3D mirrors this with swept CCD (`SweptCcd3D`), impulse resolution, and substepping.

## Collider Model

Static and dynamic colliders are **object-oriented**:

- `Collider2D` / `Collider3D` — baked static shapes (`UniquePtr<IShape>`)
- `DynamicCollider2D` / `DynamicCollider3D` — runtime dynamic shapes
- `DynamicBody2D` / `DynamicBody3D` — ECS handle bundles for queries and 3D simulation

See `spark/physics/colliders/PhysicsColliders.hpp` and `spark/physics/simulation/PhysicsSimulation.hpp`.

## Recommended Update Order

```cpp
void OnUpdate(const FrameTiming& t, IEngineContext& ctx) override {
    ReadInputAndSetForces();
    physics.Simulate2D(GetWorld(), t);
    PostPhysicsGameplay();
    Game::OnUpdate(t, ctx);
}
```

Next: [Rigidbody 2D](02-rigidbody-2d.md).

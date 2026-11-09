# FPS Introduction

Part 9 overview: [3D game systems map](00-3d-game-systems-map.md). SparkDemo ships several **first-person / third-person 3D** references — there is no separate `samples/` tree. Use these demos as templates for movement, shooting, physics, and AI:

| Demo | Launcher | Highlights |
|------|----------|------------|
| `CharacterCameraDemo` | **#5** | 1st/3rd person, `CharacterController3D`, `SpringArm3D`, cursor capture |
| `PhysicsBallThrow3DDemo` | **#11** | 3D rigidbodies, throw impulse, captured mouse |
| `PhysicsShowcase3DDemo` | **#29 / P** | Tier‑2 tour: CC handoff, layers, queries, triggers, CCD, fixed step |
| `Maze3DDemo` | **#9** | Nav mesh guard, perception, gems, `FlyCamera` fallback |
| `SceneEditor3DDemo` | (shell) | Mesh ray pick, `TryRaycastSphereWorld`, gizmo camera |

This part walks a **minimal FPS arena**: fly camera, lit cubes, LMB hitscan + emissive tracers (no bullet rigidbodies required).

**Rendering:** [Part 3](../3-3d-graphics/01-meshes-and-materials.md) · **3D components:** [Part 7](../7-game-components-usage/02-3d-rendering.md) · **Physics 3D:** [Part 5](../5-physics/05-physics-3d.md).

## What you will build

| System | API / components |
|--------|------------------|
| Camera | `FlyCamera` + cursor capture |
| Arena | `MeshComponent`, `MaterialComponent`, registered unit meshes |
| Shooting | `TryRaycastSphereWorld` vs target transforms |
| VFX | Emissive materials + short-lived tracer entities |
| Submit | `SubmitStandardLitSceneFromWorld` (PBR + sun) |

## Class design: `FpsGame`

### C++

```cpp
class FpsGame final : public Game {
    FlyCamera camera{};
    SharedPtr<Mesh> unitCube{};
    SharedPtr<Mesh> groundMesh{};
    Array<GameObject*> targets{};
    Array<TracerBullet> tracers{};
    std::uint32_t shotsFired = 0, hits = 0;
};

struct TracerBullet {
    GameObject* go = nullptr;
    Vector3 velocity{};
    float timeLeft = 0.0F;
};
```

### C\#

```csharp
public sealed class FpsGame : Game
{
    private FlyCamera _camera = new();
    // Meshes: world.RegisterMesh after MakeShared equivalent in bindings
    private readonly List<GameObject> _targets = new();
}
```

### Python

```python
class FpsGame(Game):
    def __init__(self):
        super().__init__()
        self._shots = 0
        self._hits = 0
```

## Frame contract

Same as other `Game` subclasses:

1. `OnAttach` — meshes, arena, input capture, physics settings
2. `OnUpdate` — camera, shooting, tracer motion, then `Game::OnUpdate`
3. Optional `physics.SimulateAll3D` if you add rigidbodies later
4. `OnRender` — build `viewProj`, `SubmitStandardLitSceneFromWorld`

Reference: `CharacterCameraDemo.cpp` (character + camera), `SceneEditor3DDemo.cpp` (world ray pick).

Next: [Project Setup](02-project-setup.md).

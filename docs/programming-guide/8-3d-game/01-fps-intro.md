# FPS Introduction

SparkDemo includes several 3D first-person references — there is no separate `samples/` tree in the repository. Use these as templates:

| Demo | Launcher # | Highlights |
|------|------------|------------|
| `CharacterCameraDemo` | 5 | 3rd/1st person, `CharacterController3DComponent`, spring arm |
| `PhysicsBallThrow3DDemo` | 11 | 3D physics, throwing, cursor capture |
| `Maze3DDemo` | 9 | Nav mesh guard, perception, gems, fly camera |

This walkthrough describes a **minimal first-person arena** you can build by combining patterns from those demos:

- `FlyCamera` with cursor capture
- Lit PBR cubes and ground plane
- LMB spawns emissive tracer bullets
- `TryRaycastSphereWorld` hitscan (no physics middleware for shooting)

## Class Design: `FpsGame`

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

Reference implementations: `CharacterCameraDemo.cpp` (camera + character), `SceneEditor3DDemo.cpp` (ray picking with `TryRaycastSphereWorld`).

Next: [Project Setup](02-project-setup.md).

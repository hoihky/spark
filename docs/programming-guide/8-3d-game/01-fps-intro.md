# FPS Introduction

`samples/fps_game_template/` demonstrates a minimal **first-person arena**:

- `FlyCamera` with cursor capture
- Lit PBR cubes and ground plane
- LMB spawns emissive tracer bullets
- Custom ray-sphere hitscan (no physics middleware for shooting)

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

Next: [Project Setup](02-project-setup.md).

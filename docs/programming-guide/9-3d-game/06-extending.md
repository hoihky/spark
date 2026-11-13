---
title: Extending the FPS
order: 6
---

# Extending the FPS

Grow the template into a full shooter: weapons, AI guards, physics projectiles, character-controller movement, and viewmodels.

## Concept: weapons system

### C++

```cpp
struct Weapon {
    float fireRateHz = 8.0F;
    float spreadRadians = 0.02F;
    float cooldownLeft = 0.0F;
};

void TickWeapon(Weapon& w, float dt, const Vector3& origin, const Vector3& baseDir) {
    w.cooldownLeft = std::max(0.0F, w.cooldownLeft - dt);
    if (w.cooldownLeft > 0.0F) return;
    w.cooldownLeft = 1.0F / w.fireRateHz;
    Vector3 dir = baseDir;
    // optional: apply random yaw/pitch offset up to spreadRadians
    TryShootAlongRay(origin, dir);
}
```

### C\#

```csharp
void TickWeapon(ref Weapon w, float dt, Vector3 origin, Vector3 baseDir)
{
    w.CooldownLeft = MathF.Max(0f, w.CooldownLeft - dt);
    if (w.CooldownLeft > 0f) return;
    w.CooldownLeft = 1f / w.FireRateHz;
    TryShootAlongRay(origin, baseDir);
}
```

### Python

```python
def tick_weapon(w, dt, origin, direction):
    w.cooldown = max(0.0, w.cooldown - dt)
    if w.cooldown > 0:
        return
    w.cooldown = 1.0 / w.fire_rate_hz
```

## Concept: enemy AI (`Maze3DDemo`)

### C++

```cpp
#include "spark/ai/GameAiSubsystem.hpp"

GameObject* guard = SpawnPatrolGuard(world, patrolPathGo);

void OnUpdate(const FrameTiming& t, IEngineContext& ctx) override {
    TickFpsPlayer(t, ctx);
    Game::OnUpdate(t, ctx);
    physics.SimulateAll3D(GetWorld(), t);
    SimulateGameAi(GetWorld(), t, ctx);
}
```

### C\#

```csharp
SimulateGameAi(World, timing, context);
```

`PerceptionSensorComponent` + `NavMeshAgentComponent` on the guard — [AI overview](../4-ai/01-ai-overview.md).

## Concept: physics projectiles

Replace tracers with dynamic spheres (`PhysicsBallThrow3DDemo`):

### C++

```cpp
GameObject* ball = world.CreateGameObject();
ball->AddComponent<TransformComponent>()->SetTranslation(origin);
ball->AddComponent<SphereCollider3DComponent>(0.15F);
ball->AddComponent<Rigidbody3DComponent>()->SetVelocity(dir * 30.0F);
ball->AddComponent<MeshComponent>(unitSphere, SceneMeshSlot::Custom, Vector3{0.9F, 0.5F, 0.2F});

physics.SimulateAll3D(world, timing);
```

### C\#

```csharp
ball.AddSphereCollider3D(0.15f);
ball.AddRigidbody3D().SetVelocity(dir * 30f);
```

## Concept: character controller (walk FPS)

### C++

```cpp
go->AddComponent<CharacterController3DComponent>();
physics.GetCharacterController3D().SimulateCharacterControllers3D(world, timing);
```

### C\#

```csharp
player.GetOrAddComponent<CharacterController3DComponent>();
_physics.GetCharacterController3D().SimulateCharacterControllers3D(World, timing);
```

See `CharacterCameraDemo` for `SpringArm3DComponent` + `CameraFollow3DComponent`.

## Concept: glTF viewmodels

### C++

```cpp
world.Assets().RequestGltf("assets/models/Fox.glb");
world.Assets().Pump();
if (world.Assets().IsGltfReady("assets/models/Fox.glb")) {
    SkinnedGltfAsset arms = world.Assets().LoadSkinnedGltf("assets/models/Fox.glb");
    viewModel->AddComponent<SkinnedMeshComponent>(arms.mesh);
    viewModel->AddComponent<AnimatorComponent>(arms.skeleton, 0, 1.0F);
}
```

### C\#

```csharp
var fox = world.Assets.LoadSkinnedGltf("assets/models/Fox.glb");
viewModel.AddSkinnedMesh(fox.Mesh);
```

[Skinned characters](../3-3d-graphics/05-skinned-characters.md).

## C\# scripting host

Managed games use **`Spark.Bindings`** + optional **`SparkScriptHost`** ([C# scripting](../../CSHARP_SCRIPTING.md), [codegen](../../COMPONENT_SCRIPT_CODEGEN.md)). Regenerate mirrors after `component-script-catalog.json` changes.

---

Continue Part 9:

- [Character & third person](07-character-and-third-person.md) — demo **#5**  
- [Physics 3D product path](08-physics-3d-product-path.md) — demo **#29 / P**  
- [Maze AI & perception](09-maze-ai-and-perception.md) — demo **#8**  
- [3D components handbook](10-3d-components-handbook.md) — FPS / character / environment stacks

**Congratulations!** You finished the core **FPS** track and the extended 3D chapters. Next: [3D systems map](00-3d-game-systems-map.md), [Roadmap hub](../1-overview-architecture/16-glossary-and-roadmap-hub.md), or [Architecture & developer guide](../../ARCHITECTURE_AND_DEVELOPER_GUIDE.md).

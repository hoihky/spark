---
title: Extending the FPS
order: 6
---

# Extending the FPS

## Weapons System

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

## Enemy AI (from `Maze3DDemo`)

```cpp
#include "spark/ai/GameAiSubsystem.hpp"

// Spawn patrol path + guard (see AI Overview chapter)
GameObject* guard = SpawnPatrolGuard(world, patrolPathGo);

void OnUpdate(const FrameTiming& t, IEngineContext& ctx) override {
    UpdatePlayer(t, ctx);
    Game::OnUpdate(t, ctx);
    physics.SimulateAll3D(GetWorld(), t);
    SimulateGameAi(GetWorld(), t, ctx);
}
```

`PerceptionSensorComponent` on the guard detects the player; `NavMeshAgentComponent` follows the patrol path.

## Physics Projectiles

Replace tracers with dynamic rigidbodies — pattern from `PhysicsBallThrow3DDemo`:

```cpp
GameObject* ball = world.CreateGameObject();
ball->AddComponent<TransformComponent>()->SetTranslation(origin);
ball->AddComponent<SphereCollider3DComponent>(0.15F);
ball->AddComponent<Rigidbody3DComponent>()->SetVelocity(dir * 30.0F);
ball->AddComponent<MeshComponent>(unitSphere, SceneMeshSlot::Custom, Vector3{0.9F, 0.5F, 0.2F});

// Each frame:
physics.SimulateAll3D(world, timing);
```

## Character Controller (FPS Movement)

For walk-on-ground FPS without full rigidbody simulation:

```cpp
go->AddComponent<CharacterController3DComponent>();
// Each frame, after input:
physics.GetCharacterController3D().SimulateCharacterControllers3D(world, timing);
```

See `CharacterCameraDemo` for first/third-person camera rigs with `SpringArm3DComponent` and `CameraFollow3DComponent`.

## glTF Weapons + Skinned Hands

```cpp
world.RequestGltf("assets/models/Fox.glb");
world.PumpAssets();
if (world.IsGltfReady("assets/models/Fox.glb")) {
    SkinnedGltfAsset arms = world.LoadSkinnedGltf("assets/models/Fox.glb");
    viewModel->AddComponent<SkinnedMeshComponent>(arms.mesh);
    viewModel->AddComponent<AnimatorComponent>(arms.skeleton, 0, 1.0F);
    // Parent viewmodel to camera entity with local offset transform
}
```

## C# Scripting

Spark supports CoreCLR scripting (`SPARK_BUILD_SCRIPT_HOST`) — see `docs/CSHARP_SCRIPTING.md`.

---

**Congratulations!** You have completed all eight parts. Explore `SparkDemo` demos and `docs/ARCHITECTURE_AND_DEVELOPER_GUIDE.md` for advanced topics: scene editor, serialization, tilemaps, and the open-world roadmap.

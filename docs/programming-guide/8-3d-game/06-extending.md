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
```

Tick cooldown in `OnUpdate`; apply random yaw/pitch offset to `dir` before hitscan.

## Enemy AI

```cpp
auto* enemy = world.CreateGameObject();
enemy->AddComponent<TransformComponent>();
auto* agent = enemy->AddComponent<AiAgentComponent>();
agent->SetMaxSpeed(5.0F);
agent->SetSteeringPlane(AiSteeringPlane::XzWorld);
// + FSM chase/attack states
```

Call `SimulateGameAi` after player movement.

## Physics Projectiles

Replace tracers with:

```cpp
go->AddComponent<SphereCollider3DComponent>(0.1F);
go->AddComponent<Rigidbody3DComponent>()->SetVelocity(dir * 30.0F);
physics.Simulate3D(world, timing);
```

## glTF Weapons + Skinned Hands

```cpp
SkinnedGltfAsset arms = world.LoadSkinnedGltf("assets/models/arms.glb");
// SkinnedMeshComponent + AnimatorComponent on viewmodel entity
// Parent to camera with offset transform
```

## C# Scripting

Spark supports CoreCLR scripting (`SPARK_BUILD_SCRIPT_HOST`) — see `docs/CSHARP_SCRIPTING.md`.

---

**Congratulations!** You have completed all eight parts. Explore `SparkDemo` demos and `docs/ARCHITECTURE_AND_DEVELOPER_GUIDE.md` for advanced topics: scene editor, deferred paths, open-world roadmap.

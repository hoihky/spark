---
title: Arena and Targets
order: 3
---

# Arena and Targets

## Ground

```cpp
GameObject* ground = world.CreateGameObject();
ground->AddComponent<TransformComponent>();
ground->AddComponent<MeshComponent>(groundMesh, SceneMeshSlot::Custom,
    Vector3{0.35F, 0.38F, 0.42F});
auto* gmat = ground->AddComponent<MaterialComponent>(nullptr);
gmat->SetRoughness(0.85F);
gmat->SetMetallic(0.0F);
```

## Targets

```cpp
GameObject* target = world.CreateGameObject();
auto* tr = target->AddComponent<TransformComponent>();
tr->SetTranslation({x, 1.0F, z});
tr->SetScale({1.2F, 1.2F, 1.2F});

target->AddComponent<MeshComponent>(unitCube, SceneMeshSlot::Custom, Vector3{0.9F, 0.25F, 0.2F});
auto* mat = target->AddComponent<MaterialComponent>(nullptr);
mat->SetMetallic(0.1F);
mat->SetRoughness(0.4F);
mat->SetEmissive({1.0F, 0.3F, 0.1F});
mat->SetEmissiveStrength(1.5F);

targets.PushBack(target);
```

## Spatial Culling

```cpp
GetScene().SetSpatialPartitionKind(ScenePartitionKind::BoundingVolumeHierarchy);
```

Improves frustum culling for many static meshes.

Next: [Shooting and Tracers](8-3d-game/04-shooting.html).

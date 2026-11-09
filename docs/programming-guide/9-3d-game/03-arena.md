# Arena and Targets

Build a static floor plus a handful of emissive target cubes. The FPS template uses **mesh raycast against spheres** fitted to targets (not mesh triangles).

[Part 7 — Mesh / Material](../7-game-components-usage/02-3d-rendering.md#meshcomponent)

## Concept: ground plane

### C++

```cpp
GameObject* ground = world.CreateGameObject();
ground->AddComponent<TransformComponent>();
ground->AddComponent<MeshComponent>(groundMesh, SceneMeshSlot::Custom,
    Vector3{0.35F, 0.38F, 0.42F});
auto* gmat = ground->AddComponent<MaterialComponent>(nullptr);
gmat->SetRoughness(0.85F);
gmat->SetMetallic(0.0F);
```

### C\#

```csharp
var ground = world.CreateGameObject();
ground.AddMesh(SceneMeshSlot.Custom, new Vector3(0.35f, 0.38f, 0.42f), "fps/ground");
ground.AddMaterialLit(null).SetRoughness(0.85f).SetMetallic(0f);
```

### Python

```python
from spark._generated.component_kind import ComponentKind
ground = world.create_game_object("Ground")
ground.get_or_add_default_component(ComponentKind.Mesh)
ground.get_or_add_default_component(ComponentKind.Material)
```

## Concept: target cubes

### C++

```cpp
GameObject* target = world.CreateGameObject();
auto* tr = target->AddComponent<TransformComponent>();
tr->SetTranslation({x, 1.0F, z});
tr->SetScale({1.2F, 1.2F, 1.2F});

target->AddComponent<MeshComponent>(unitCube, SceneMeshSlot::Custom, Vector3{0.9F, 0.25F, 0.2F});
auto* mat = target->AddComponent<MaterialComponent>(nullptr);
mat->SetMetallic(0.1F);
mat->SetRoughness(0.4F);
mat->SetEmissive({1.0F, 0.3F, 0.1F}, 1.5F);

targets.PushBack(target);
```

### C\#

```csharp
var target = world.CreateGameObject();
target.GetOrAddTransform().SetTranslation(new Vector3(x, 1f, z));
target.AddMesh(SceneMeshSlot.Custom, new Vector3(0.9f, 0.25f, 0.2f), "fps/unit_cube");
target.AddMaterialLit(null).SetEmissive(new Vector3(1f, 0.3f, 0.1f), 1.5f);
_targets.Add(target);
```

### Python

```python
target = world.create_game_object()
target.get_or_add_transform().translation = Vector3(x, 1.0, z)
target.get_or_add_default_component(ComponentKind.Mesh)
```

Lay out targets in a grid (e.g. `for (int i = -2; i <= 2; ++i)`) so hitscan has clear depth ordering along the camera forward axis.

## Concept: spatial culling (many static meshes)

### C++

```cpp
GetScene().SetSpatialPartitionKind(ScenePartitionKind::BoundingVolumeHierarchy);
```

### C\#

```csharp
Scene.SetSpatialPartitionKind(ScenePartitionKind.BoundingVolumeHierarchy);
```

### Python

```python
# Scene partition kind usually set from C++ game bootstrap today
```

BVH helps when the arena grows beyond a few dozen static drawables.

Next: [Shooting and Tracers](04-shooting.md).

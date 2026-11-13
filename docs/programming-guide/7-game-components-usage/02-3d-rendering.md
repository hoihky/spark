# Component Guide: 3D Rendering

Components that populate `SceneRenderParams::draws`, `transparentDraws`, `waterDraws`, foliage, and sky. All lit meshes need `TransformComponent` + `MeshComponent` or `SkinnedMeshComponent` and usually `MaterialComponent`.

**Related:** [Meshes and materials](../3-3d-graphics/01-meshes-and-materials.md), [Field reference](../1-overview-architecture/07-game-component-reference.md).

| Component | Role |
|-----------|------|
| `MeshComponent` | Rigid mesh draw |
| `MaterialComponent` | PBR factors + textures |
| `MultiMaterialComponent` | glTF multi-slot |
| `SkinnedMeshComponent` | GPU skinned draw |
| `SkyComponent` | Procedural / textured sky |
| `TerrainComponent` | Heightfield mesh |
| `WaterBodyComponent` | Water pass |
| `BillboardComponent` | Camera-facing transform |
| `DecalProjectorComponent` | Projected decals |
| `FogVolumeComponent` | Local fog |
| `PostProcessVolumeComponent` | Exposure / grade |
| `GrassFieldComponent` | Instanced grass cards |

---

## MeshComponent

**Header:** `spark/ecs/components/rendering/MeshComponent.hpp`

**When to use:** Static props, floors, procedural cubes. Slot `UnitCube` / `GroundPlane` or `Custom` with registered mesh.

#### C++

```cpp
auto mesh = MakeShared<Mesh>(Mesh::CreateUnitCube());
world.Assets().RegisterMesh(mesh, "props/crate");
go->AddComponent<MeshComponent>(mesh, SceneMeshSlot::Custom, Vector3{0.8F, 0.6F, 0.4F});
```

#### C\#

```csharp
go.AddMesh(SceneMeshSlot.Custom, new Vector3(0.8f, 0.6f, 0.4f));
```

#### Python

```python
go.add_mesh(native.SceneMeshSlot.Custom, Vector3(0.8, 0.6, 0.4))
```

---

## MaterialComponent

**Header:** `spark/ecs/components/rendering/MaterialComponent.hpp`

**When to use:** Every lit mesh; sets metallic, roughness, emissive, normal map, `SceneShadingModel`.

#### C++

```cpp
auto* mat = go->AddComponent<MaterialComponent>(baseColorTex);
mat->SetMetallic(0.1F);
mat->SetRoughness(0.45F);
mat->SetMaterialAsset(world, "materials/trim.sparkmat");
```

#### C\#

```csharp
go.AddMaterialLit("assets/models/Crate_albedo.png", Vector3.One)
    .SetRoughness(0.45f);
```

#### Python

```python
go.add_material_lit("assets/models/Crate_albedo.png", Vector3(1, 1, 1))
```

---

## MultiMaterialComponent

**Header:** `spark/ecs/components/rendering/MultiMaterialComponent.hpp`

**When to use:** glTF with multiple primitives / material variants (`KHR_materials_variants`).

#### C++

```cpp
#include "spark/ecs/components/rendering/MultiMaterialComponent.hpp"

auto* multi = go->AddComponent<MultiMaterialComponent>();
multi->SetSlotMaterialAsset(world, 0, "models/Building.glb#material/0");
multi->SetVariantIndex(1);
```

#### C\#

```csharp
go.GetOrAddComponent<MultiMaterialComponent>()?.SetVariantIndex(1);
```

#### Python

```python
go.get_or_add_default_component(ComponentKind.MultiMaterial)
```

---

## SkinnedMeshComponent

**Header:** `spark/ecs/components/rendering/SkinnedMeshComponent.hpp`

**When to use:** Animated characters; requires `AnimatorComponent` + skeleton palette.

#### C++

```cpp
SkinnedGltfAsset fox = world.LoadSkinnedGltf("assets/models/Fox.glb");
go->AddComponent<SkinnedMeshComponent>(fox.mesh);
go->AddComponent<AnimatorComponent>(fox.skeleton, fox.walkClipIndex);
```

#### C\#

```csharp
var fox = world.LoadSkinnedGltf("assets/models/Fox.glb");
go.GetOrAddComponent<SkinnedMeshComponent>()?.SetMesh(fox.Mesh);
go.AddAnimator(fox.Skeleton, fox.WalkClipIndex);
```

#### Python

```python
# bind_skinned_gltf_from_path when host exposes it
go.get_or_add_default_component(ComponentKind.SkinnedMesh)
```

---

## SkyComponent

**Header:** `spark/ecs/components/rendering/SkyComponent.hpp`

**When to use:** Gradient sky, sun disk, optional cubemap background instead of flat `worldClearColor`.

#### C++

```cpp
#include "spark/ecs/components/rendering/SkyComponent.hpp"

auto* sky = world.CreateGameObject();
sky->AddComponent<SkyComponent>()->SetSunDirection(Vector3{0.3F, -0.8F, 0.2F}.Normalized());
```

#### C\#

```csharp
sky.GetOrAddComponent<SkyComponent>()?.SetSunDirection(sunDir);
```

#### Python

```python
sky.get_or_add_default_component(ComponentKind.Sky)
```

---

## TerrainComponent

**Header:** `spark/ecs/components/rendering/TerrainComponent.hpp`

**When to use:** Large heightfields; sculpt brushes regenerate mesh. Always casts CSM shadows.

#### C++

```cpp
TerrainGeneratorSettings ts{};
ts.halfExtentX = 120.0F;
ts.heightScale = 20.0F;
terrainGo->AddComponent<TerrainComponent>(ts);
```

#### C\#

```csharp
go.AddTerrain(ts);
```

#### Python

```python
go.get_or_add_default_component(ComponentKind.Terrain)
```

---

## WaterBodyComponent

**Header:** `spark/ecs/components/rendering/WaterBodyComponent.hpp`

**When to use:** Lakes and rivers; separate `waterDraws` pass with Gerstner waves.

#### C++

```cpp
#include "spark/ecs/components/rendering/WaterBodyComponent.hpp"

lake->AddComponent<WaterBodyComponent>()->SetWaveAmplitude(0.35F);
```

#### C\#

```csharp
lake.GetOrAddComponent<WaterBodyComponent>()?.SetWaveAmplitude(0.35f);
```

#### Python

```python
lake.get_or_add_default_component(ComponentKind.WaterBody)
```

---

## BillboardComponent

**Header:** `spark/ecs/components/rendering/BillboardComponent.hpp`  
**Priority 50** — rotates owner to face main camera before gameplay update.

#### C++

```cpp
pickup->AddComponent<BillboardComponent>();
```

#### C\#

```csharp
pickup.GetOrAddBillboard();
```

#### Python

```python
pickup.get_or_add_default_component(ComponentKind.Billboard)
```

---

## DecalProjectorComponent

**Header:** `spark/ecs/components/rendering/DecalProjectorComponent.hpp`

**When to use:** Bullet holes, blood splats projected onto opaque geometry.

#### C++

```cpp
#include "spark/ecs/components/rendering/DecalProjectorComponent.hpp"

auto* decal = impact->AddComponent<DecalProjectorComponent>();
decal->SetSize({0.4F, 0.4F, 0.2F});
decal->SetAlbedoTexture(splatTex);
```

#### C\#

```csharp
impact.GetOrAddComponent<DecalProjectorComponent>()?.SetSize(new Vector3(0.4f, 0.4f, 0.2f));
```

#### Python

```python
impact.get_or_add_default_component(ComponentKind.DecalProjector)
```

---

## FogVolumeComponent

**Header:** `spark/ecs/components/rendering/FogVolumeComponent.hpp`

**When to use:** Localized fog tint/density boxes in 3D interiors.

#### C++

```cpp
cave->AddComponent<FogVolumeComponent>()->SetDensity(0.12F);
```

#### C\#

```csharp
cave.GetOrAddComponent<FogVolumeComponent>()?.SetDensity(0.12f);
```

#### Python

```python
cave.get_or_add_default_component(ComponentKind.FogVolume)
```

---

## PostProcessVolumeComponent

**Header:** `spark/ecs/components/rendering/PostProcessVolumeComponent.hpp`

**When to use:** Per-region exposure, bloom, color grade (`ApplyRegionalRenderVolumes` at submit).

#### C++

```cpp
volume->AddComponent<PostProcessVolumeComponent>()->SetExposure(1.15F);
```

#### C\#

```csharp
volume.GetOrAddComponent<PostProcessVolumeComponent>()?.SetExposure(1.15f);
```

#### Python

```python
volume.get_or_add_default_component(ComponentKind.PostProcessVolume)
```

---

## GrassFieldComponent

**Header:** `spark/ecs/components/rendering/GrassFieldComponent.hpp`

**When to use:** Instanced grass/foliage cards on terrain (`FoliageWindDemo`, `GrassFieldTerrainDemo`).

#### C++

```cpp
field->AddComponent<GrassFieldComponent>()->SetPatchRadius(48.0F);
```

#### C\#

```csharp
go.AddGrassField();
```

#### Python

```python
go.get_or_add_default_component(ComponentKind.GrassField)
```

---

## Typical prop stack

```cpp
GltfAssetBinder::BindFromPath(*go, "assets/models/Lantern.glb");
// adds Transform + Mesh + Material (+ variants)
```

Next: [Tilemap components](03-tilemap.md).

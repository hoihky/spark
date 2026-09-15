# Terrain and Sky

## Class Design: `TerrainComponent`

Heightfield mesh with brush editing API:

```cpp
explicit TerrainComponent(TerrainGeneratorSettings settings, Vector3 meshAlbedo = ...);

void ResetHeightsToProcedural(GameObject& owner);
void RegenerateMesh(GameObject& owner);
bool TryRaycastWorld(const GameObject& owner, Vector3 rayOriginWorld,
                     Vector3 rayDirWorld, float maxDistance, Vector3& outHitWorld) const;
void ApplyHeightBrushWorld(GameObject& owner, Vector3 centerWorld,
                           float radiusWorld, float deltaY);
```

## Procedural Terrain (from `TerrainDemo`)

`TerrainDemo` (launcher **#4**) builds a large heightfield with fly camera sculpting (LMB/RMB), point lights, and a **procedural blue sky** via world clear color — no sky mesh required.

```cpp
#include "spark/ecs/components/rendering/TerrainComponent.hpp"
#include "spark/scene/mesh/TerrainGeneratorSettings.hpp"

TerrainGeneratorSettings ts{};
ts.subdivX = 288;
ts.subdivZ = 288;
ts.halfExtentX = 220.0F;
ts.halfExtentZ = 220.0F;
ts.heightScale = 26.0F;
ts.noiseScale = 0.0135F;
ts.octaves = 6;
ts.persistence = 0.5F;
ts.lacunarity = 2.05F;
ts.seed = 0xC047ACEEu;
ts.worldUnitsPerTextureRepeat = ts.halfExtentX * 2.0F;  // one texture span

auto* terrainGo = world.CreateGameObject();
terrainGo->AddComponent<TransformComponent>();
terrainGo->AddComponent<TerrainComponent>(ts);
if (MaterialComponent* m = terrainGo->AddComponent<MaterialComponent>(groundTex)) {
    m->SetMetallic(0.02F);
    m->SetRoughness(0.94F);
}

SceneRenderParams params{};
params.worldClearColorEnabled = true;
params.worldClearColor = {0.42F, 0.62F, 0.92F};  // soft blue horizon
```

`TerrainGeneratorSettings` fields:

| Field | Default | Meaning |
|-------|---------|---------|
| `subdivX`, `subdivZ` | 96 | Height grid resolution |
| `halfExtentX`, `halfExtentZ` | 56 | Half-size in world units |
| `heightScale` | 14 | Max vertical displacement |
| `noiseScale` | 0.055 | Base noise frequency |
| `octaves`, `persistence`, `lacunarity` | 6, 0.48, 2.05 | fBM detail |
| `worldUnitsPerTextureRepeat` | 112 | UV tiling scale |

Help text uses `DemoHelpHud` — **hidden by default**; press **H** to toggle. See [Engine Loop](../1-overview-architecture/04-engine-loop.md#demo-shell-shortcuts).

## Height Brush Editing

```cpp
TerrainComponent* terrain = terrainGo->GetComponent<TerrainComponent>();
Vector3 hitWorld{};
if (terrain->TryRaycastWorld(*terrainGo, rayOrigin, rayDir, 750.0F, hitWorld)) {
    terrain->ApplyHeightBrushWorld(*terrainGo, hitWorld, 4.0F, 0.35F);
}
```

## Class Design: `SkyComponent`

```cpp
explicit SkyComponent(SceneSkyMode mode) noexcept;  // Box, Dome, Plane
void SetSkyTexture(SharedPtr<Texture2D> t);
void SetTint(const Vector3& c) noexcept;
```

Pair with `MeshComponent` using matching sky mesh (`Mesh::CreateSkyDome`) and `SceneSkyMode` on the draw item. See `SkyDemo` (launcher **#2**) for box/dome/plane modes and HDR equirect textures.

`GltfSamples3DDemo` (launcher **#21**, key **Q**) uses a scaled sky sphere + HDR equirect for background and **opt-in** HDR IBL on `DamagedHelmet.glb`:

```cpp
auto skyMesh = MakeShared<Mesh>(Mesh::CreateSkySphere(1.0F, 20, 40));
auto* skyGo = world.CreateGameObject();
skyGo->AddComponent<TransformComponent>()->SetUniformScale(120.0F);
skyGo->AddComponent<SkyComponent>(SceneSkyMode::Dome);
skyGo->AddComponent<MeshComponent>(skyMesh, SceneMeshSlot::Custom, Vector3::One);
skyGo->AddComponent<MaterialComponent>()->SetBaseColorTexture(hdrEquirectTex);

SceneRenderParams params{};
params.iblEnabled = true;
params.iblUseHdrSkyEnvironment = true;  // use HDR sky for reflections (not default)
```

By default, HDR sky textures are **background-only**; IBL uses the procedural hemisphere from ambient colors unless `iblUseHdrSkyEnvironment` is set.

## World clear color vs sky mesh

| Approach | When to use | API |
|----------|-------------|-----|
| **World clear color** | Simple gradient/solid horizon, no cubemap | `worldClearColorEnabled`, `worldClearColor` |
| **Sky mesh** | Textured dome/box/plane, HDR equirect backdrop | `SkyComponent` + `MeshComponent` + `SceneSkyMode` |

## Fog

```cpp
params.fogEnabled = true;
params.fogColor = {0.65F, 0.75F, 0.85F};
params.fogDensity = 0.015F;
```

See **`TimeOfDayDemo`** (launcher **#17**, key **N**) for sun/sky/fog animation with Khronos **`Lantern.glb`**, HDR equirect sky dome, regional fog/post volumes, and SSAO. The demo submits via `FillStandardLitSceneFromWorld` (lighting resolve, IBL, shadow flags, glTF material maps). Or drive time from ECS:

```cpp
auto* tod = worldRoot->AddComponent<TimeOfDayDriverComponent>();
tod->SetDayLengthSeconds(120.0F);
tod->SetLooping(false);  // when you advance time manually in gameplay code
// FillStandardLitSceneFromWorld calls ProcessTimeOfDayDrivers internally.
```

Regional fog: add `FogVolumeComponent` on a trigger volume (camera-inside test at submit).

## Water bodies (`WaterBodyComponent`)

Infinite ocean / lake surfaces use a dedicated **water pass** (Gerstner waves, refraction, depth absorption, crest foam). Water is collected into `SceneRenderParams::waterDraws` and drawn **after opaque + sky** and **before** generic `transparentDraws`.

```cpp
#include "spark/ecs/components/water/WaterBodyComponent.hpp"
#include "spark/scene/water/WaterBodyMode.hpp"
#include "spark/scene/water/WaterWavePreset.hpp"

WaterSurfaceMeshSettings meshSettings{};
meshSettings.SetSubdivisionsPerAxis(96);
meshSettings.SetTileHalfExtent(128.0F);

auto* waterGo = world.CreateGameObject();
waterGo->AddComponent<TransformComponent>();
waterGo->AddComponent<WaterBodyComponent>(
        WaterBodyMode::InfiniteOcean,
        0.0F,  // waterLevelY
        WaterBodyExtent::MakeInfinitePlaceholder(),
        WaterWavePresetId::StormySea,
        meshSettings,
        Vector3{0.10F, 0.58F, 0.78F});  // surface albedo

WaterBodyComponent* water = waterGo->GetComponent<WaterBodyComponent>();
water->SetWavePresetId(WaterWavePresetId::OceanModerate);
water->SetAbsorption(0.03F);
water->SetFoamStrength(0.9F);
water->SetDetailNormalStrength(0.35F);
```

### Wave presets (`.sparkwater`)

Built-in presets ship as assets under `assets/water/`:

| File | Preset id | Use |
|------|-----------|-----|
| `calm_lake.sparkwater` | `CalmLake` | Small ripples |
| `ocean_moderate.sparkwater` | `OceanModerate` | Default coast |
| `stormy_sea.sparkwater` | `StormySea` | Large swell |

Format (`sparkwater_v1`):

```text
sparkwater_v1
preset CalmLake
globalWindSpeed 1.0
wave <dirX> <dirZ> <amplitude> <wavelength> <speed> <steepness>
```

`WaterWaveSettings::FromPreset` loads the asset when present (build copies to `SPARK_BUILD_ASSETS_DIR/water/`) and falls back to embedded defaults otherwise. Gameplay height queries use the same math via `GerstnerWaveSurface`.

```cpp
GerstnerWaveSurface surface = WaterWavePreset(WaterWavePresetId::CalmLake).ToSurface();
float height = surface.SampleHeight(worldX, worldZ, timeSeconds);
Vector3 normal = surface.SampleNormal(worldX, worldZ, timeSeconds);
```

See **`WaterLakeDemo`** (SparkDemo **#23**, key **W**) for fly camera, preset hotkey (**P**), and submerged test geometry. Roadmap: [`docs/WATER_ROADMAP.md`](../../../WATER_ROADMAP.md).

## Grass and trees (planned)

Wind-reactive vegetation is not implemented yet. Tracked milestones:

| Topic | Roadmap |
|-------|---------|
| Grass chunks, density masks, blade wind | [`docs/FOLIAGE_ROADMAP.md`](../../../FOLIAGE_ROADMAP.md) (F2) |
| Instanced trees, foliage materials, LOD / impostors | [`docs/FOLIAGE_ROADMAP.md`](../../../FOLIAGE_ROADMAP.md) (F3–F4) |
| Global wind field (shared with water waves) | [`docs/FOLIAGE_ROADMAP.md`](../../../FOLIAGE_ROADMAP.md) (F0) · [`docs/WATER_ROADMAP.md`](../../../WATER_ROADMAP.md) |

Today, sample trees are placed as individual glTF entities (see `CharacterCameraDemo` and `assets/models/Tree_*.gltf`).

Next: [Particles](07-particles.md).

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

```cpp
#include "spark/ecs/components/rendering/TerrainComponent.hpp"
#include "spark/scene/TerrainGeneratorSettings.hpp"

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

Pair with `MeshComponent` using matching sky mesh (`Mesh::CreateSkyDome`) and `SceneSkyMode` on the draw item. See `SkyDemo` for box/dome/plane modes and HDR equirect textures.

```cpp
auto skyMesh = MakeShared<Mesh>(Mesh::CreateSkyDome(500.0F, 32, 64));
auto* skyGo = world.CreateGameObject();
skyGo->AddComponent<MeshComponent>(skyMesh, SceneMeshSlot::Custom, Vector3::One);
skyGo->AddComponent<SkyComponent>(SceneSkyMode::Dome);
```

## Fog

```cpp
params.fogEnabled = true;
params.fogColor = {0.65F, 0.75F, 0.85F};
params.fogDensity = 0.015F;
```

See `TimeOfDayDemo` for sun/sky/fog animation, or drive time from ECS:

```cpp
worldRoot->AddComponent<TimeOfDayDriverComponent>()->SetDayLengthSeconds(120.0F);
// FillStandardLitSceneFromWorld calls ProcessTimeOfDayDrivers internally.
```

Regional fog: add `FogVolumeComponent` on a trigger volume (camera-inside test at submit).

Next: [Particles](07-particles.md).

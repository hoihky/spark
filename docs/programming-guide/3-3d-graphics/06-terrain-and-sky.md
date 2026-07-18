---
title: Terrain and Sky
order: 6
---

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

```cpp
TerrainGeneratorSettings settings{};
settings.gridCellsX = 128;
settings.gridCellsZ = 128;
settings.cellWorldSize = 1.0F;

auto* terrainGo = world.CreateGameObject();
terrainGo->AddComponent<TransformComponent>();
terrainGo->AddComponent<TerrainComponent>(settings);
```

## Class Design: `SkyComponent`

```cpp
explicit SkyComponent(SceneSkyMode mode) noexcept;  // Box, Dome, Plane
void SetSkyTexture(SharedPtr<Texture2D> t);
void SetTint(const Vector3& c) noexcept;
```

Pair with `MeshComponent` using matching sky mesh (`CreateSkyDome`) and `SceneSkyMode` on the draw item.

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

Next: [Particles](3d-graphics/07-particles.html).

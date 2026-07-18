---
title: Lighting
order: 3
---

# Lighting

## ECS Light Components

| Component | GPU representation |
|-----------|-------------------|
| `PointLightComponent` | `ScenePointLight` (position, range, color, intensity) |
| `SpotLightComponent` | `SceneSpotLight` (cone angles, direction) |
| Sun (global) | Passed to `FillStandardLitSceneFromWorld` |

```cpp
auto* lamp = world.CreateGameObject();
lamp->AddComponent<TransformComponent>()->SetTranslation({3, 2, 0});
auto* pl = lamp->AddComponent<PointLightComponent>();
pl->SetColor({1.0F, 0.85F, 0.6F});
pl->SetIntensity(4.0F);
pl->SetRange(12.0F);
pl->SetCastsShadow(true);
```

## Submit with Lighting Arguments

```cpp
SubmitStandardLitSceneFromWorld(
    GetWorld(), context, viewProj, camera.position,
    Vector3{0.25F, -1.0F, 0.15F}.Normalized(),  // directional "sun"
    Vector3{1.0F, 0.97F, 0.92F},                 // sun color
    3.5F,                                        // sun intensity
    Vector3{0.12F, 0.14F, 0.20F},               // ambient hemisphere
    true,                                        // particles
    camRight, camUp, sceneTime);
```

## Clustered Forward

`VulkanRenderer` packs up to **256 point** and **128 spot** lights per frame via clustered shading (`VulkanClusteredForwardLights`).

## Scene Lighting Profile

`SceneLightingProfile` on `SceneRenderParams` controls exposure, tonemap, and IBL contribution — see `spark/render/SceneLightingProfile.hpp`.

Read engine docs: `docs/LIGHTING_AND_SHADOWS.md`, `docs/MATERIALS_AND_LIGHTING.md`.

Next: [Shadows](3d-graphics/04-shadows.html).

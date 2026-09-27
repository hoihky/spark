# 2D Lighting

## `SpriteLighting2DComponent`

Attach alongside `SpriteComponent` for per-sprite shading in `sprite.frag`:

```cpp
#include "spark/ecs/components/rendering/SpriteLighting2DComponent.hpp"

go->AddComponent<SpriteComponent>(tex, tint, uvRect, sortOrder);
auto* lit = go->AddComponent<SpriteLighting2DComponent>(
    SpriteLighting2DMode::NormalMapped,
    Vector4{1.0F, 0.35F, 0.9F, 1.0F},  // normal strength, ambient, directional, point scale
    Vector4{});
lit->SetNormalMap(normalAtlas);
lit->SetSyncNormalUvWithSprite(true);  // follows SpriteAnimator / uvRect each frame
lit->SetRampMap(rampTex);              // Ramp mode
```

| `SpriteLighting2DMode` | Effect |
|------------------------|--------|
| `None` | Flat tint only (default) |
| `DirectionalLambert` | Fake XY normal from quad center vs sun direction |
| `Rim` | Silhouette rim highlight |
| `PulseEmission` | Pulsing emissive overlay |
| `PointSoft` | Soft point lights from clustered scene lights |
| `NormalMapped` | Tangent normal map + sun + point lights |
| `Ramp` | Normal map + 1D ramp lookup (toon-style) |
| `SpecularGloss` | Normal map + Blinn specular toward sun |
| `Hemisphere` | Normal map + sky/ground hemispheric tint |
| `WrapDiffuse` | Normal map + wrapped (soft) sun diffuse |
| `NormalMappedRim` | Normal map + sun + view rim (`param1.rgb` = rim color) |
| `MatcapApprox` | Normal map + ramp matcap from normal XY |
| `FlickerLit` | Normal map + sun/points with sinusoidal intensity flicker |

### Normal atlases

Use a second texture with the **same grid layout** as the albedo atlas and keep `SetSyncNormalUvWithSprite(true)` so animation frames stay aligned. Helpers:

- `Texture2D::CreateNormalAtlasForUniformGrid(columns, rows, cellW, cellH)`
- `Texture2D::CreateFlatNormalMap()` for placeholders

Override UVs with `SetSyncNormalUvWithSprite(false)` and `SetNormalUvRect(...)`.

## `PointLight2DComponent`

2D-oriented punctual light (no shadows). Position = owner transform + `localOffset`, Z from `worldZ`. Optional flicker:

```cpp
#include "spark/ecs/components/lighting/PointLight2DComponent.hpp"

auto* lamp = go->AddComponent<PointLight2DComponent>(Vector3{1, 0.9f, 0.7f}, 2.2f, 8.0f);
lamp->SetLocalOffset({0.0f, 0.4f});
lamp->SetFlicker(true, 0.2f, 10.0f);
```

Collected into `SceneRenderParams::pointLights` with standard `PointLightComponent`.

## Directional sun + Y-sort

```cpp
SubmitStandardLitSceneFromWorldWithCamera(world, context, sunDir, sunColor, intensity, ambient, false, sceneTime);
params.spriteSortMode = SceneSpriteSortMode::SortOrderThenWorldY;
```

## 2D particles on the sprite layer

`ParticleEmitterComponent::SetRenderSpace(ParticleRenderSpace::SpriteLayer)` draws into the sprite composite pass (sorted with sprites/tilemaps). Scene save encodes render space in particle snapshot extension `p5`.

Built-in 2D VFX (sprite-layer): `hit_spark_2d`, `coin_pop_2d`, `jump_ring_2d`, `lantern_glow_2d`, `rain_splash_2d`, `slash_arc_2d`, `footstep_puff_2d`, `block_impact_2d`, `magic_nova_2d`, `heal_sparkle_2d`, `poison_bubble_2d`, `shield_pulse_2d`, `water_ripple_2d`, `ember_motif_2d` (`assets/vfx/*.sparkvfx`).

Next: [2D Render Pipeline](06-2d-render-pipeline.md).

# 2D Lighting

2D lighting is implemented inside the **sprite fragment shader** when `SpriteLighting2DComponent` is present on the same `GameObject` as `SpriteComponent`. Directional “sun” comes from `SceneRenderParams` (`lightDirectionWorld`, `lightColor`, `lightIntensity`, `ambientColor`). **Point lights** reuse the 3D `ScenePointLight` array — both `PointLightComponent` and `PointLight2DComponent` append to `params.pointLights` during `FillStandardLitSceneFromWorld`.

2D point lights **do not cast shadows** (`castsShadow = false` in submit).

## Modes overview

| `SpriteLighting2DMode` | Effect |
|------------------------|--------|
| `None` | Tint only |
| `DirectionalLambert` | Fake normal from quad vs sun |
| `Rim` | Silhouette highlight |
| `PulseEmission` | Pulsing emissive |
| `PointSoft` | Clustered scene point lights |
| `NormalMapped` | Tangent normal map + sun + points |
| `Ramp` | Normal map + 1D ramp (toon) |
| `SpecularGloss` | Blinn specular toward sun |
| `Hemisphere` | Sky/ground hemispheric tint |
| `WrapDiffuse` | Soft wrapped diffuse |
| `NormalMappedRim` | Normal + view rim |
| `MatcapApprox` | Ramp matcap from normal XY |
| `FlickerLit` | Sinusoidal intensity flicker |
| `HitFlash` | Full-sprite flash (`param1.y` = flash start time) |
| `Outline` | Screen-space outline from alpha neighbors |
| `Dissolve` | Noise dissolve (`param0.x` = progress) |

Parametric helpers: `SpriteFx2D::ApplyOutline`, `ApplyHitFlashAtSceneTime`, `ApplyDissolveProgress` (`spark/render/sprites2d/SpriteFx2D.hpp`).

## Concept: normal-mapped sprite

Use a **second atlas** aligned to the albedo grid; keep `SetSyncNormalUvWithSprite(true)` when using `SpriteAnimatorComponent`.

### C++

```cpp
#include "spark/ecs/components/rendering/SpriteLighting2DComponent.hpp"

go->AddComponent<SpriteComponent>(albedo, tint, uvRect, sortOrder);
SpriteLighting2DComponent* lit = go->AddComponent<SpriteLighting2DComponent>(
    SpriteLighting2DMode::NormalMapped,
    Vector4{1.0F, 0.35F, 0.9F, 1.0F},
    Vector4{});
lit->SetNormalMap(normalAtlas);
lit->SetSyncNormalUvWithSprite(true);
```

Placeholders: `Texture2D::CreateFlatNormalMap()`, `CreateNormalAtlasForUniformGrid(cols, rows, cellW, cellH)`.

### C\#

```csharp
go.AddSprite(albedoKey, tint, uv, sortOrder);
var lit = go.AddSpriteLighting2D(SpriteLighting2DMode.NormalMapped);
lit.SetNormalMap(normalKey);
lit.SetSyncNormalUvWithSprite(true);
```

### Python

```python
go.add_sprite(albedo_key, tint, uv, sort_order=sort)
lit = go.add_sprite_lighting_2d()
lit.set_mode_normal_mapped()
lit.set_normal_map(normal_key)
lit.set_sync_normal_uv_with_sprite(True)
```

## Concept: gameplay FX (outline, hit flash, dissolve)

### C++

```cpp
#include "spark/render/sprites2d/SpriteFx2D.hpp"

SpriteFx2D::ApplyOutline(lighting, outlineColor, widthTexels, softness);
SpriteFx2D::ApplyHitFlashAtSceneTime(lighting, flashColor, decaySeconds, sceneTimeSeconds);
SpriteFx2D::ApplyDissolveProgress(lighting, progress01);
```

Used in `GameFlow2DProductPathDemo` for gem pickup feedback.

### C\#

```csharp
SpriteFx2D.ApplyOutline(lit, outlineColor, widthTexels, softness);
SpriteFx2D.ApplyHitFlashAtSceneTime(lit, flashColor, decay, sceneTime);
```

### Python

```python
# Call native helpers via gameplay components when wrapped; otherwise toggle mode HitFlash on SpriteLighting2DComponent.
lit.set_mode_hit_flash()
```

## Concept: `PointLight2DComponent`

Punctual light in the XY plane; Z from `worldZ` + transform.

### C++

```cpp
#include "spark/ecs/components/lighting/PointLight2DComponent.hpp"

PointLight2DComponent* lamp = go->AddComponent<PointLight2DComponent>(
    Vector3{1, 0.9f, 0.7f}, 2.2f, 8.0f);
lamp->SetLocalOffset({0.0f, 0.4f});
lamp->SetFlicker(true, 0.2f, 10.0f);
```

### C\#

```csharp
var lamp = go.AddPointLight2D(new Vector3(1f, 0.9f, 0.7f), intensity: 2.2f, range: 8f);
lamp.SetLocalOffset(new Vector2(0f, 0.4f));
lamp.SetFlicker(enabled: true, amount: 0.2f, speed: 10f);
```

### Python

```python
lamp = go.add_point_light_2d(Vector3(1.0, 0.9, 0.7), intensity=2.2, range=8.0)
lamp.set_local_offset(0.0, 0.4)
lamp.set_flicker(True, 0.2, 10.0)
```

## Concept: sun direction + Y-sort

Submit sets global light; per-sprite modes multiply into the result.

### C++

```cpp
SubmitStandardLitSceneFromWorldWithCamera(
    world, context, sunDir, sunColor, intensity, ambient,
    enableParticles, sceneTimeSeconds);
// internally sets params.spriteSortMode — pass SortOrderThenWorldY in overloads that accept sort mode
```

### C\#

```csharp
SceneSubmit.SubmitStandardLitSceneFromWorldWithCamera(
    world, context, sunDir, sunColor, 0.85f, ambient,
    enableParticles: false, sceneTime);
```

### Python

Sun/ambient usually come from the native game template; script games inherit lighting from the loaded scene's `OnRender` unless you add custom render hooks.

## 2D particles on the sprite layer

`ParticleEmitterComponent::SetRenderSpace(ParticleRenderSpace::SpriteLayer)` composites with sprites/tilemaps (same sort domain). Built-in keys include `footstep_puff_2d`, `coin_pop_2d`, `slash_arc_2d` under `assets/vfx/*.sparkvfx`.

### C++

```cpp
emitter->SetRenderSpace(ParticleRenderSpace::SpriteLayer);
emitter->SetEffectKey("footstep_puff_2d");
```

### C\#

```csharp
emitter.SetRenderSpace(ParticleRenderSpace.SpriteLayer);
emitter.SetEffectKey("footstep_puff_2d");
```

### Python

```python
emitter.set_render_space_sprite_layer()
emitter.set_effect_key("footstep_puff_2d")
```

## Performance tips

- Prefer **one** normal atlas shared across characters.
- Limit active `PointLight2DComponent` count — same cap as 3D point lights in `SceneRenderParams`.
- Use `DirectionalLambert` or `Rim` for background props; reserve `NormalMapped` for heroes.
- Batch outline/hit FX — flashing many outlines in one frame stresses fill rate.

Limits: [2D runtime limits](../8-2d-game/08-scene2d-runtime-limits.md).

Next: [2D Render Pipeline](06-2d-render-pipeline.md).

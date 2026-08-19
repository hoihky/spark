# 2D Lighting

## `SpriteLighting2DComponent`

Attach alongside `SpriteComponent` to enable per-sprite 2D lighting modes:

```cpp
#include "spark/ecs/components/rendering/SpriteLighting2DComponent.hpp"

go->AddComponent<SpriteComponent>(tex, tint, uvRect, sortOrder);
go->AddComponent<SpriteLighting2DComponent>(
    SpriteLighting2DMode::NormalMapped,  // or Ramp, None
  1.0F,   // normal strength
  0.0F);  // ramp offset
```

| `SpriteLighting2DMode` | Effect |
|------------------------|--------|
| `None` | Flat tint only (default) |
| `NormalMapped` | Per-pixel normal lighting |
| `Ramp` | Gradient ramp lookup |

The submit path copies mode and parameters into each `SceneSpriteDraw`.

## Directional Light on 2D Scenes

`SubmitStandardLitSceneFromWorld` accepts a **sun direction** even for sprite-only scenes — tints sprites when lighting mode is enabled:

```cpp
SubmitStandardLitSceneFromWorldWithCamera(
    world, context,
    Vector3{0.30F, 0.86F, 0.36F}.Normalized(),  // lightDirectionWorld
    Vector3{1.0F, 0.98F, 0.95F},                 // lightColor
    0.85F,                                       // lightIntensity
    Vector3{0.16F, 0.18F, 0.24F},               // ambientColor
    false,                                       // particles
    sceneTime);
```

For manual submit with Y-sort:

```cpp
params.spriteSortMode = SceneSpriteSortMode::SortOrderThenWorldY;
```

## Y-Sort for Top-Down ARPG

```cpp
params.spriteSortMode = SceneSpriteSortMode::SortOrderThenWorldY;
```

Within the same `sortOrder`, sprites with **lower world Y** draw on top (southern characters occlude northern). `RenderLayers2DDemo` and `FarmingRpgRenderLayersDemo` demonstrate layer + Y-sort combinations.

## Fake Point Lights

Spawn additive-style sprites at lamp positions with high `sortOrder` and soft gradient textures, or use `PointLightComponent` in mixed 2.5D scenes where 3D lighting affects sprites.

See `docs/2D_ARPG_FEATURES.md` for ARPG-specific roadmap notes.

Next: [2D Render Pipeline](06-2d-render-pipeline.md).

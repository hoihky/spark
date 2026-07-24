# 2D Lighting

## `SpriteLighting2DMode`

Each `SceneSpriteDraw` can carry 2D lighting parameters (`spark/render/SpriteLighting2D.hpp`):

| Mode | Effect |
|------|--------|
| `None` | Flat tint only (default) |
| `NormalMapped` | Per-pixel normal lighting |
| `Ramp` | Gradient ramp lookup |

Fields on `SceneSpriteDraw`:

```cpp
SpriteLighting2DMode lightingMode = SpriteLighting2DMode::None;
Vector4 lightingParam0{1, 1, 1, 1};
Vector4 lightingParam1{1, 0, 0, 0};
```

## Directional Light on 2D Scenes

`SubmitStandardLitSceneFromWorld` accepts a **sun direction** even for sprite-only scenes — tints sprites when lighting mode is enabled:

```cpp
SubmitStandardLitSceneFromWorld(
    world, context, viewProj, camera.position,
    Vector3{0.30F, 0.86F, 0.36F}.Normalized(),  // lightDirectionWorld
    Vector3{1.0F, 0.98F, 0.95F},                 // lightColor
    0.85F,                                       // lightIntensity
    Vector3{0.16F, 0.18F, 0.24F},               // ambientColor
    false, pr, pu, sceneTime,
    SceneSpriteSortMode::SortOrderThenWorldY);
```

## Y-Sort for Top-Down ARPG

```cpp
params.spriteSortMode = SceneSpriteSortMode::SortOrderThenWorldY;
```

Within the same `sortOrder`, sprites with **lower world Y** draw on top (southern characters occlude northern).

## Fake Point Lights

Spawn additive-style sprites at lamp positions with high `sortOrder` and soft gradient textures.

See `docs/2D_ARPG_FEATURES.md` for ARPG-specific roadmap notes.

Next: [2D Render Pipeline](06-2d-render-pipeline.md).

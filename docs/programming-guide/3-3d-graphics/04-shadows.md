# Shadows

## Quick Start: Shadow-Enabled Scene

The simplest path is `SubmitStandardLitSceneFromWorldWithCamera` with a directional sun and shadow-casting punctual lights on entities:

```cpp
// 1. Sun direction + intensity passed to submit (enables directional CSM when renderer settings allow)
SubmitStandardLitSceneFromWorldWithCamera(
    GetWorld(), ctx,
    Vector3{0.3F, -1.0F, 0.2F}.Normalized(),
    Vector3{1.0F, 0.98F, 0.95F}, 1.4F,
    Vector3{0.14F, 0.16F, 0.22F},
    true, sceneTime);

// 2. Punctual shadows on individual lights
auto* pl = lamp->AddComponent<PointLightComponent>();
pl->SetCastsShadow(true);

auto* sl = spotGo->AddComponent<SpotLightComponent>();
sl->SetCastsShadow(true);
```

Opaque `MeshComponent` draws cast shadows by default. Budget shadow-casting punctual lights — each adds GPU cost.

## Directional CSM (Manual `SceneRenderParams`)

When building params yourself (e.g. `ThreeDDemo`), configure shadow fields:

```cpp
SceneRenderParams params{};
params.enableDirectionalShadows = true;
params.shadowCascadeCount = 4;
params.shadowMapResolution = 2048;
params.shadowBias = 0.002F;
params.shadowNormalBias = 0.02F;
params.directionalShadowsEnabled = true;
// ... fill draws, lights, viewProjection ...
context.SetSceneRenderParams(params);
```

When using `SubmitStandardLitSceneFromWorld`, directional shadows follow the sun vector you pass and renderer defaults.

## Punctual Light Shadows

```cpp
pl->SetCastsShadow(true);   // PointLightComponent
sl->SetCastsShadow(true);   // SpotLightComponent
```

GPU cost scales with shadow-casting light count — budget carefully. See `TimeOfDayDemo` and `MaterialShowcase3DDemo` in SparkDemo.

## Toon Shading + Shadows

```cpp
mat->SetShadingModel(SceneShadingModel::ToonCel);
mat->SetToonDiffuseBands(3);
mat->SetToonRimIntensity(0.35F);
mat->SetToonRimPower(2.5F);
```

Toon materials still receive shadow terms — see `ToonShadingDemo` (SparkDemo).

## Debugging Shadow Artifacts

| Artifact | Tweak |
|----------|-------|
| Peter-panning | Increase `shadowBias` |
| Shadow acne | Increase `shadowNormalBias` |
| Shimmering CSM | Adjust cascade splits in renderer settings |

Next: [Skinned Characters](05-skinned-characters.md).

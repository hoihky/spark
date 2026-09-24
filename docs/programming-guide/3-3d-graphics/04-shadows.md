# Shadows

## Quick Start: Shadow-Enabled Scene

The simplest path is `FillStandardLitSceneFromWorld` with a directional sun and shadow-casting punctual lights on entities:

```cpp
SceneRenderParams params{};
params.lightingProfile = SceneLightingProfile::Outdoor;
params.directionalShadowsEnabled = true;
params.punctualShadowsEnabled = true;

FillStandardLitSceneFromWorld(
    world, context, viewProj, cameraPos,
    Vector3{0.3F, -1.0F, 0.2F}.Normalized(),  // toward sun (overridden when useTimeOfDay)
    Vector3{1.0F, 0.98F, 0.95F}, 1.4F,
    Vector3{0.14F, 0.16F, 0.22F},
    true,  // particles
    {}, {}, sceneTime, params);

// Punctual shadows on individual lights
auto* pl = lamp->AddComponent<PointLightComponent>();
pl->SetCastsShadow(true);
```

Opaque `MeshComponent` draws **cast and receive** directional shadows by default (`shadowsCastByDefault` / `shadowsReceiveByDefault` on `SceneRenderParams`, resolved via `SceneLightingProfile`).

## Directional CSM (`SceneRenderParams`)

| Field | Default | Role |
|-------|---------|------|
| `directionalShadowsEnabled` | `true` | Master toggle for CSM build + sampling |
| `shadowCascadeNear` | `0` | CSM near split; `0` = lighting preset |
| `shadowCascadeFar` | `0` | CSM far split; `0` = preset (`400` outdoor). Raise for large islands (e.g. `1200` in `WaterLakeDemo`) |
| `shadowBias` | `0.0026` | Depth bias in shadow compare |
| `shadowNormalBias` | `0.048` | Extra bias on grazing surfaces |
| `shadowDepthSampleFlipV` | `false` | Flip shadow atlas V when sampling (MoltenVK) |
| `shadowDistanceMax` | `0` | World distance where shadows fade out; `0` = preset |
| `shadowFadeStartRatio` | `0` | Fade begins at `shadowDistanceMax * ratio` |
| `shadowsCastByDefault` | `true` | Default cast bit for submitted draws |
| `shadowsReceiveByDefault` | `true` | Default receive bit for submitted draws |
| `useTimeOfDay` / `timeOfDay` | preset | When enabled, overrides sun direction/color before shadow matrices are built |

```cpp
SceneRenderParams params{};
params.lightingProfile = SceneLightingProfile::Outdoor;
params.directionalShadowsEnabled = true;
params.shadowCascadeFar = 900.0F;  // wider cascade 0 for large outdoor scenes
params.shadowBias = 0.0026F;
params.shadowNormalBias = 0.048F;
```

## Per-draw and per-material shadow flags

Scene submit packs `SceneDrawItem::shadowFlags` (`kSceneShadowCast` / `kSceneShadowReceive`).

**`MaterialComponent` overrides** (optional, easiest for props):

```cpp
auto* mat = seabed->AddComponent<MaterialComponent>();
mat->SetShadowCastOverride(false);      // never cast CSM
mat->SetShadowReceiveOverride(true);  // optional explicit receive
mat->ClearShadowCastOverride();         // back to scene defaults + built-in rules
```

**Built-in submit rules** (when cast is not overridden on the material):

| Draw | Cast |
|------|------|
| `TerrainComponent` | Always casts |
| `GroundPlane` with world `Y < -0.5` | Does not cast (submerged seabed) |
| `UnitCube` with world `Y < 1` | Does not cast (underwater props) |
| `SkyComponent` | Neither cast nor receive |
| `WaterBodyComponent` | Separate `waterDraws`; receive-only via `ResolveWaterShadowFlags` (`kSceneShadowReceive`, never cast) |

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

| Artifact | Likely cause | Tweak |
|----------|--------------|-------|
| Peter-panning | Bias too low | Increase `shadowBias` |
| Shadow acne | Bias too low / grazing | Increase `shadowNormalBias` |
| Large rectangular slab on terrain | CSM cascade split across mesh; seabed casting | Raise `shadowCascadeFar`; disable cast on submerged props (`SetShadowCastOverride(false)`) |
| Shimmering CSM | Texel snapping + camera motion | Expected at cascade edges; increase `cascadeBlendFraction` in profile |
| Water shows gray matching terrain shadow | SSR/refraction samples opaque color (binding 13) | Fix terrain shadow; water does not sample CSM directly yet |

CSM sampling treats UV / depth outside the cascade tile as **fully lit** (`scene.frag`) to avoid false shadows at ortho frustum edges.

Next: [Skinned Characters](05-skinned-characters.md).

# Shadows

## Directional CSM

Cascaded shadow maps for the sun direction — configured on `SceneRenderParams`:

```cpp
params.enableDirectionalShadows = true;
params.shadowCascadeCount = 4;
params.shadowMapResolution = 2048;
params.shadowBias = 0.002F;
params.shadowNormalBias = 0.02F;
```

When using `SubmitStandardLitSceneFromWorld`, enable shadows via the directional light setup and ensure meshes cast shadows (default for opaque draws).

## Punctual Light Shadows

```cpp
pl->SetCastsShadow(true);   // PointLightComponent
sl->SetCastsShadow(true);   // SpotLightComponent
```

GPU cost scales with shadow-casting light count — budget carefully.

## Toon Shading + Shadows

```cpp
mat->SetShadingModel(SceneShadingModel::ToonCel);
mat->SetToonSteps(3);
mat->SetToonSmoothness(0.1F);
```

Toon materials still receive shadow terms — see `ToonShadingDemo`.

## Debugging Shadow Artifacts

| Artifact | Tweak |
|----------|-------|
| Peter-panning | Increase `shadowBias` |
| Shadow acne | Increase `shadowNormalBias` |
| Shimmering CSM | Adjust cascade splits in renderer settings |

Next: [Skinned Characters](05-skinned-characters.md).

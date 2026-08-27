# Lighting

## ECS Light Components

| Component | GPU representation |
|-----------|-------------------|
| `PointLightComponent` | `ScenePointLight` (position, range, color, intensity) |
| `SpotLightComponent` | `SceneSpotLight` (cone angles, direction) |
| `DirectionalLightComponent` | Overrides sun direction/color on submit |
| Sun (global) | Passed to `FillStandardLitSceneFromWorld` or `SubmitStandardLitSceneFromWorldWithCamera` |

```cpp
#include "spark/ecs/components/lighting/PointLightComponent.hpp"
#include "spark/ecs/components/lighting/SpotLightComponent.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"

auto* lamp = world.CreateGameObject();
lamp->AddComponent<TransformComponent>()->SetTranslation({3, 2, 0});
auto* pl = lamp->AddComponent<PointLightComponent>();
pl->SetColor({1.0F, 0.85F, 0.6F});
pl->SetIntensity(4.0F);
pl->SetRange(12.0F);
pl->SetCastsShadow(true);

// Spot light — local −Z is the beam axis
auto* spotGo = world.CreateGameObject();
auto* str = spotGo->AddComponent<TransformComponent>();
str->SetTranslation({0, 4, 0});
str->SetRotationEuler({DegreesToRadians(-45.0F), 0, 0});
auto* sl = spotGo->AddComponent<SpotLightComponent>();
sl->SetColor({0.9F, 0.95F, 1.0F});
sl->SetIntensity(6.0F);
sl->SetRange(18.0F);
sl->SetInnerConeDegrees(12.0F);
sl->SetOuterConeDegrees(28.0F);
sl->SetCastsShadow(true);
```

## Complete Lit Scene (from `ThreeDDemo` / `MaterialShowcase3DDemo`)

Spawn meshes and lights in `OnAttach`, then submit each frame:

```cpp
void OnRender(IRenderFrame&, IEngineContext& ctx) override {
    SubmitStandardLitSceneFromWorldWithCamera(
        GetWorld(), ctx,
        Vector3{0.25F, -1.0F, 0.15F}.Normalized(),  // sun direction
        Vector3{1.0F, 0.97F, 0.92F},                 // sun color
        3.5F,                                        // sun intensity
        Vector3{0.12F, 0.14F, 0.20F},               // ambient hemisphere
        true,                                        // collect particles
        sceneTime);
}
```

For manual camera control (fly camera, custom projection):

```cpp
int w = 1, h = 1;
ctx.GetFramebufferSize(w, h);
const float aspect = static_cast<float>(w) / static_cast<float>(h);
const Matrix4 proj = Matrix4::PerspectiveVulkan(DegreesToRadians(70.0F), aspect, 0.1F, 500.0F);
const Matrix4 vp = proj * camera.ViewMatrix();
Vector3 camRight{}, camUp{};
camera.BillboardBasis(camRight, camUp);

SubmitStandardLitSceneFromWorld(
    GetWorld(), ctx, vp, camera.position,
    Vector3{0.25F, -1.0F, 0.15F}.Normalized(),
    Vector3{1.0F, 0.97F, 0.92F}, 3.5F,
    Vector3{0.12F, 0.14F, 0.20F},
    true, camRight, camUp, sceneTime);
```

`FillStandardLitSceneFromWorld` walks the world and fills `SceneRenderParams` — point/spot lights from components, sky, sprites, skinned meshes, particles, fog/post volumes, and time-of-day drivers.

## Emissive Materials as Local Lights

Use `MaterialComponent::SetEmissive` for glowing props (no extra light component needed):

```cpp
if (MaterialComponent* mat = target->AddComponent<MaterialComponent>()) {
    mat->SetEmissive({0.95F, 0.28F, 0.12F}, 2.4F);
    mat->SetRoughness(0.4F);
}
```

See `Maze3DDemo` gem pickups and guard mesh for this pattern.

## Clustered Forward

`VulkanRenderer` packs up to **256 point** and **128 spot** lights per frame via clustered shading (`VulkanClusteredForwardLights`).

## Image-Based Lighting (IBL)

For metallic PBR materials, set `iblEnabled` and either bind an equirect environment map on a sky draw or set `iblEnvironmentLayer` explicitly. When `iblEnvironmentLayer == -1`, submit resolves the first sky draw with a bound texture (see `ResolveIblEnvironmentLayer` in `SceneSubmitMaterial.cpp`). Specular split-sum uses a precomputed **2D BRDF LUT** (`VulkanIblBrdfLut`, descriptor binding **12**).

```cpp
SceneRenderParams params{};
params.iblEnabled = true;
params.iblEnvironmentLayer = -1;  // auto from sky dome texture
params.iblIntensity = 1.0F;
FillStandardLitSceneFromWorld(world, ctx, viewProj, cameraPos, sunDir, sunColor, sunIntensity,
                              ambient, false, {}, {}, 0.0F, params, SceneSpriteSortMode::SortOrderOnly, &scene);
```

`GltfSamples3DDemo` (launcher **#21**, key **Q**) pairs Khronos `DamagedHelmet.glb` with Poly Haven `studio_small_08_1k.hdr` on a sky dome. HDR files are loaded via `Texture2D::TryLoadFromFile` (currently tonemapped to LDR for the scene texture array).

## Scene Lighting Profile

`SceneLightingProfile` on `SceneRenderParams` controls exposure, tonemap, and IBL contribution — see `spark/render/SceneLightingProfile.hpp`.

Read engine docs: `docs/LIGHTING_AND_SHADOWS.md`, `docs/MATERIALS_AND_LIGHTING.md`.

Next: [Shadows](04-shadows.md).

# Lighting & shadows — feature plan and implementation status

This document lists **recommended features** for open-world action lighting, and what is **implemented in Spark** today.

## Forward punctual lights (no extra shadow maps)

| Feature | Description |
|--------|-------------|
| **Point lights** | Up to **`MaxPointLights` (256)** omnidirectional lights; range attenuation; **clustered forward** SSBO + 16³ grid (`VulkanClusteredForwardLights`). |
| **Spot lights** | Up to **`MaxSpotLights` (128)** cone lights; inner/outer cones; distance falloff; axis = object **local −Z** in world space. Optional shadow maps when `castsShadow` is set (capped). |
| **Material-driven micro-detail** | Optional **normal map** and **ORM** (occlusion, roughness, metallic) textures affect how directional, point, and spot lights read the surface (`MaterialComponent`, `SceneDrawItem`). See §5.4 in [`ARCHITECTURE_AND_DEVELOPER_GUIDE.md`](ARCHITECTURE_AND_DEVELOPER_GUIDE.md). |

## Implemented (Phase 1)

| Feature | Description |
|--------|--------------|
| **Cascaded directional shadows (CSM)** | Four cascades in a **2048²** atlas (2×2 tiles of **1024²** each); logarithmic-uniform split distances; per-cascade light matrices with **texel snapping**; PCF in `scene.frag`; **soft cascade blend** at split boundaries (`timeGlobal.w` / `SceneShadowSettings::cascadeBlendFraction`). |
| **PCF filtering** | 3×3 percentage-closer filtering in `scene.frag` for soft penumbra (cheap). |
| **Slope-scaled bias** | Reduces shadow acne on shallow surfaces (`shadowBias` + `shadowNormalBias` on `SceneRenderParams`). |
| **Skinned meshes in shadow pass** | Same skin SSBO path as the main scene pass. |
| **Toggle & tuning** | `SceneRenderParams::directionalShadowsEnabled`, `shadowBias`, `shadowNormalBias`. |
| **HDR + tonemap** | Scene renders to **R16G16B16A16**, optional **SSAO** composite, then **ACES tonemap** to swapchain; UI drawn after tonemap (LDR). |
| **Screen-space ambient occlusion (SSAO)** | Optional post pass (`VulkanScreenSpaceEffectsPass`) after the HDR scene pass: copies scene depth to a sampled image, applies 16-tap hemisphere AO in `post_process.frag`, writes to a scratch HDR target consumed by tonemap. Toggles: `ssaoEnabled`, `ssaoRadius`, `ssaoBias`, `ssaoStrength`. |
| **`SceneLightingProfile`** | `Outdoor` / `Interior` / `NightInterior` / `Default` — shared preset struct: cascades, exposure, **shadow fade** (end + start ratio), **cast/receive defaults**, **hemisphere + probe** ambient, optional **time-of-day**. |
| **Time-of-day demo** | SparkDemo menu **19** / key **N** — `TimeOfDayDemo` animates `timeOfDay` 0→1 over 90s (sunshine → sunset → night → dawn). |
| **Per-draw shadow flags** | `SceneDrawItem::shadowFlags` (`kSceneShadowCast` / `kSceneShadowReceive`) — shadow pass skips non-casters; lit pass skips receivers. |
| **Time of day** | `SceneRenderParams::useTimeOfDay` + `timeOfDay` (0–1) — analytic sun direction/color/intensity and sky/ground/probe fill. |
| **Clustered forward punctual lights** | Point + spot lights in SSBOs (bindings **4** / **5**); per-fragment direct SSBO iteration in `scene.frag` / sprite mode 4; caps **`MaxPointLights` (256)** / **`MaxSpotLights` (128)**. |
| **Point / spot shadow maps** | Up to **4** spot tiles (512² in 1024² atlas) + **2** point cubemap slices (512² × 6 faces); `castsShadow` on `PointLightComponent` / `SpotLightComponent`; bindings **6**–**8**; 3×3 PCF in `punctual_shadows.glsl`. |

## Recommended next features (prioritized)

### Shadows

| Priority | Feature | Notes |
|----------|---------|------|
| P1 | **Contact shadows** | Very short screen-space trace along the sun for small gaps (not implemented). |

### Lighting & atmosphere

| Priority | Feature | Notes |
|----------|---------|------|
| — | **HDR + tonemap** | **Implemented:** R16G16B16A16 scene target, ACES tonemap pass, exposure from `SceneLightingProfile` / overrides. |
| — | **SSAO** | **Implemented:** `VulkanScreenSpaceEffectsPass` + `post_process.frag` (see § Implemented). |
| P0 | **Image-based ambient** | SH or irradiance volume from probes for outdoor bounce fill (basic equirect IBL exists for PBR metals). |
| P1 | **Physical sun/sky model** | Hosek-Wilkie or Preetham for disk + atmosphere; ties to time-of-day. |
| P2 | **Volumetric fog / god rays** | Height fog or light shafts; separate pass or raymarch (not implemented). |

### Content & workflow

| Priority | Feature | Notes |
|----------|---------|------|
| — | **Per-draw shadow cast / receive** | **Implemented:** `shadowFlags` on `SceneDrawItem` + push constants. |
| — | **Shadow distance fade** | **Implemented:** `shadowDistanceMax` + `shadowFadeStartRatio` via lighting profile. |

## Frame flow (GPU)

Per frame, `VulkanRenderer::RecordSceneCommandBuffer` records (in order):

1. **Texture / UI font uploads** (`VulkanDeferredUploadBatch`)
2. **Shadow maps** — punctual then directional (`VulkanPunctualShadowPass`, `VulkanDirectionalShadowPass`)
3. **HDR scene pass** — opaque + sky (`VulkanSceneOpaquePass`), sprites, particles; color → `R16G16B16A16`, depth stored for copy
4. **SSAO (optional)** — when `ssaoEnabled`: `vkCmdCopyImage` scene depth → per-flight sample image; fullscreen `post_process.frag` → scratch HDR
5. **Tonemap** — ACES from HDR or SSAO scratch into swapchain image (`VulkanHdrTonemapPass`)
6. **Screen UI** — solid rects + text in the **present** render pass (`VulkanScreenUiPass`; rects then text per layer for stable batching)

## Technical references (repo)

- `shaders/scene.vert`, `shaders/scene.frag` — lit opaque pass + shadow sampling.
- `shaders/shadow_depth.vert` — depth-only pass from the light.
- `shaders/post_process.frag`, `shaders/post_common.glsl` — SSAO composite (fullscreen).
- `shaders/punctual_shadows.glsl` — spot/point shadow sampling in the lit pass.
- `include/spark/engine/SceneRenderParams.hpp` — CPU → GPU scene parameters (`ssaoEnabled`, …).
- `include/spark/render/post/VulkanScreenSpaceEffectsPass.hpp` — SSAO pass resources and recording.
- `include/spark/render/ui/VulkanScreenUiPass.hpp` — screen-space UI (font atlas, solid/text batches).
- `include/spark/render/core/VulkanRenderer.hpp` — composes passes; delegates device/swapchain to `VulkanDeviceContext`.
- `include/spark/render/scene/VulkanSceneUniformGpu.hpp` — `SceneUniformGpu` must stay **std140**-compatible with the scene UBO in GLSL.
- `include/spark/render/scene/VulkanSceneDescriptors.hpp` — scene descriptor pool, layout, per-frame sets.
- `include/spark/render/lighting/SceneLightingResolver.hpp` — profile + time-of-day → resolved sun/ambient.
- `src/spark/render/core/VulkanRenderer.cpp` — frame recording and swapchain presentation.

## UBO size

`SceneUniformGpu` packs matrices, directional sun, **hemisphere ambient**, **four CSM matrices** + cascade splits/atlas UVs, shadow fade in `viewportSize.z` / `timeGlobal.y`, **cluster grid metadata** (`clusterGrid` / `clusterDepth`), **IBL params** (`iblParams`), and global time. Punctual lights live in **`ClusterLightsGpu`** + **`ClusterGridGpu`** SSBOs; punctual shadow metadata + atlases use bindings **6**–**8** (`PunctualShadowGpu`, spot atlas, point depth array). Uniform allocation is **656 bytes**. Shared GLSL: `shaders/scene_ubo.glsl`, `shaders/punctual_shadows.glsl`.

## SSAO tuning (`SceneRenderParams`)

| Field | Default | Role |
|-------|---------|------|
| `ssaoEnabled` | `true` | When false, tonemap reads the HDR scene color directly (no post pass). |
| `ssaoRadius` | `0.35` | World-space sample radius (meters). |
| `ssaoBias` | `0.02` | Depth compare bias in clip space. |
| `ssaoStrength` | `0.65` | Blend between no AO (1) and full AO (0). |

On Apple platforms the post pass enables depth V-flip when sampling the copied depth buffer (`depthFlipV` push constant).

---

*Update this file when you add new post effects, shadow types, or change punctual light caps / UBO layout.*

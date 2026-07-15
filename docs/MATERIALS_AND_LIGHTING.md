# Materials & lighting — capabilities and gaps

This note complements [`LIGHTING_AND_SHADOWS.md`](LIGHTING_AND_SHADOWS.md) (shadows + punctual caps) and the feature catalog in [`ARCHITECTURE_AND_DEVELOPER_GUIDE.md`](ARCHITECTURE_AND_DEVELOPER_GUIDE.md). It answers: **what composition Spark can express today**, **what lights exist**, and **what is still missing** if you want parity with larger engines or glTF feature sets.

---

## Material composition (today)

| Channel | CPU / ECS | GPU / shader | Notes |
|--------|-----------|----------------|-------|
| **Base color** | `MaterialComponent` tint + optional `SetBaseColorTexture` | `SceneDrawItem::textureLayer`, multiplied in `scene.frag` | Scalar tint always applies; texture optional. |
| **Tangent-space normal** | `SetNormalTexture` | `normalMapLayer`; derivative-built TBN in `scene.frag` | No per-vertex tangents required; Y flip matches common normal-map convention. |
| **ORM packed map** | `SetMetallicRoughnessTexture` | `metallicRoughnessMapLayer` | **R** = AO × `occlusionStrength`, **G** = roughness × `roughnessFactor`, **B** = metallic × `metallicFactor` (texture replaces scalar when layer ≥ 0). |
| **Emissive** | `SetEmissive` + optional `SetEmissiveTexture` | `emissiveMapLayer`, `emissiveFactor` | Color × intensity × factor; texture RGB multiplies when bound. |
| **glTF factors** | `SetMetallicFactor`, `SetRoughnessFactor`, `SetOcclusionStrength`, `SetEmissiveFactor` | `ModelPushConstants` | Defaults **1**; applied in `scene.frag` with scalars and ORM samples. |
| **Shading model** | `SceneShadingModel::LitPbr` / `ToonCel` | `push.shadingModel` | Toon: banded diffuse, stylized spec, rim; still uses punctual lights + shadow on sun where applicable. |
| **IBL / env reflections** | `SceneRenderParams::iblEnabled`, `iblEnvironmentLayer`, `iblIntensity` | `ubo.iblParams` + `ibl.glsl` | Lit PBR: GGX-prefiltered equirect specular (split-sum BRDF) for metals; diffuse irradiance from same env. Layer -1 = procedural hemisphere; auto-picked from sky draw with texture. |
| **SSAO** | `SceneRenderParams::ssaoEnabled`, `ssaoRadius`, `ssaoBias`, `ssaoStrength` | `post_process.frag` | Screen-space AO after HDR scene pass, before tonemap; see [`LIGHTING_AND_SHADOWS.md`](LIGHTING_AND_SHADOWS.md). |

**Texture budget:** `SceneRenderParams::sceneTextures` holds up to **16** RGBA8 layers (shared array texture). Deduplication happens in `FindOrAddSceneTexture` when filling draws.

**Demonstrator:** shell menu item **“18 — Material ball…”** or **B** from the launcher — [`MaterialShowcase3DDemo`](../include/spark/demo/MaterialShowcase3DDemo.hpp) shows one **LitPbr** sphere whose maps, tint, metallic/roughness, and emissive settings are toggled and adjusted at runtime via keyboard.

---

## Lighting (today)

| Type | Source | Shadow | Limits / notes |
|------|--------|--------|------------------|
| **Directional** | `SceneRenderParams::lightDirectionWorld` (+ color, intensity) | Optional **CSM** (4 cascades) | 2×2 atlas in 2048² shadow map; PCF in `scene.frag`. |
| **Ambient** | `ambientColor` | — | Hemispheric tint hacks exist only inside **toon** path; PBR uses flat ambient × AO when ORM present. |
| **Point** | `PointLightComponent` → `ScenePointLight` | Optional cubemap shadow (`castsShadow`, max **2** active/frame) | **256** max (`MaxPointLights`). |
| **Spot** | `SpotLightComponent` → `SceneSpotLight`; axis = object **local −Z** | Optional atlas shadow (`castsShadow`, max **4** active/frame) | **128** max (`MaxSpotLights`). |

---

## Notable gaps (materials)

Prioritized for a forward PBR renderer of this size:

| Gap | Impact | Typical follow-up |
|-----|--------|---------------------|
| **No clearcoat / sheen / specular-glossiness** | glTF extensions unsupported | New BRDF lobes + parameters or second workflow enum. |
| **No subsurface / transmission** | Skin, glass, water are approximate | SSS blur pass or thin-surface approximations. |
| **Single UV set** | Second UV from glTF ignored unless mesh path extended | Duplicate attributes or atlas. |

---

## Notable gaps (lights)

| Gap | Impact | Typical follow-up |
|-----|--------|---------------------|
| **No ECS directional component** | Sun always from submit params, not transform | `DirectionalLightComponent` or level script sets `SceneRenderParams` each frame (already possible in game code). |
| **No area / line / tube lights** | Architectural interiors harder | LTC rectangles, capsule approximations, or emissive mesh proxies. |
| **IBL (basic)** | Equirect + GGX importance sample; no dedicated cubemap mips / BRDF LUT texture yet | Offline prefiltered cubemap + 2D LUT for sharper metals at low sample count. |
| **Punctual shadow quality** | 512² tiles; 2 point + 4 spot cap | Higher-res atlases, EVSM, or temporal filtering. |
| **Contact shadows / volumetric fog** | Small-scale grounding and atmosphere | Screen-space contact trace; height fog or god-ray pass (see roadmap). |

---

## When you extend materials or lights

1. **`SceneRenderParams` / `SceneDrawItem`** — public data model.  
2. **`ModelPushConstants` + `SceneUniformGpu`** in `VulkanRenderer.hpp` — alignment and `static_assert`s.  
3. **`VulkanRenderer.cpp`** — push / memcpy packing.  
4. **`shaders/scene.vert` / `scene.frag`** — matching `layout` and UBO.  
5. **Recompile SPIR-V** (`glslangValidator` per `CMakeLists.txt`).  
6. **Managed / C ABI** — only if you expose new `ComponentKind` or params through bindings.

---

*Last updated with SSAO post pass, punctual shadows, clustered forward lights, emissive texture maps, and composited Vulkan passes. API gaps: [`SCENE_AND_RENDERING_GAPS.md`](SCENE_AND_RENDERING_GAPS.md).*

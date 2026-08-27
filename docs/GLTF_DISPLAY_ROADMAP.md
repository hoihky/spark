# glTF display compatibility roadmap

Companion to [`3D_ACTION_GAME_GAPS.md`](3D_ACTION_GAME_GAPS.md). Tracks work toward **“pick any glTF and display it”**.

## Completed (Phase 1 — quick wins)

| Item | API / behavior |
|------|----------------|
| Factor-only materials | `GltfAssetBinder` always binds materials when a mesh is present (scalar PBR included) |
| Bind-pose skinned display | `SkinnedMeshComponent::SetSkeleton` + `SkinnedMeshPalette` (no `AnimatorComponent` required) |
| Auto loader routing | `GltfContentClassifier` + `GltfAssetBinder::BindFromPath` |
| Rigid load guard | `TryLoadGltf` rejects skinned files with a clear error |

**Recommended display entry point:**

```cpp
GltfAssetBinder::BindFromPath(*go, "assets/models/Anything.glb");
```

## Completed (Phase 2 — scene graph)

| Item | API / behavior |
|------|----------------|
| `GltfSceneLoader` | Parses rigid glTF into `GltfSceneDocument` (local-space meshes, dedup by `cgltf_mesh*`) |
| `GltfSceneImporter` | Walks node tree → `GameObject` hierarchy with per-node `TransformComponent` |
| `GameWorld::TryLoadGltfScene` | Path-keyed scene cache (`CachedAssetKind::GltfScene`) |
| `BindFromPath` (rigid) | Imports scene graph instead of baking into one mesh |
| `GltfMeshBuilder` | Shared primitive builder for merged loader + scene loader |

`LoadGltf` / `GltfRigidLoader` still merge geometry for legacy single-mesh workflows.

**Still open:** rigid node animation, scene snapshot serialization.

## Completed (Phase 3 — compression)

| Item | API / behavior |
|------|----------------|
| `IGltfPrimitiveDecoder` | Strategy interface; `GltfPrimitiveDecoderRegistry` resolves Draco vs uncompressed |
| `GltfDracoPrimitiveDecoder` | `KHR_draco_mesh_compression` via Google Draco (`SPARK_ENABLE_GLTF_DRACO`, default ON) |
| `GltfMeshoptBuffers` | `EXT/KHR_meshopt_compression` buffer-view decode via meshoptimizer (`SPARK_ENABLE_GLTF_MESHOPT`, default ON) |
| `LoadParsedGltfFile` | Shared parse + buffer load + meshopt decode for all glTF loaders |
| `GltfSkinnedMeshBuilder` | Skinned primitives decode through the same registry (Draco + meshopt accessors) |
| `GltfMeshBuilder` | Decodes via registry before baking vertices; returns `GltfMeshBuildOutcome` errors |
| `GltfContentClassifier` | `ProbeResult::compression.draco` / `.meshopt` flags |

Meshopt views with a separate fallback buffer are read directly; compression-only views are decompressed into `buffer_view.data`.

**Still open:** meshopt color filter (meshoptimizer 0.22), compression-only meshopt regression asset.

## Completed (Phase 4 — visual parity)

| Item | API / behavior |
|------|----------------|
| `COLOR_0` | Decoded in primitive decoders; stored on `Mesh` / `SkinnedMesh` vertices; shaded in `scene.frag` |
| Per-map texcoord | `MaterialUvMap` per texture slot; `TEXCOORD_0` / `TEXCOORD_1` in vertex layout; map index in shader push constants |
| `KHR_materials_unlit` | `GltfMaterial::unlit` → `SceneShadingModel::Unlit` (emissive + base, no lighting) |
| Joint budget | `Skeleton::MaxJoints` raised to **128** (GPU skin SSBO 8 KB) |
| Multi-skinned meshes | `GltfSceneDocument::skinnedMeshes` / `skeletons`; `GltfSkinNodeLoader`; scene import binds each skinned node |
| `KHR_texture_transform` | Parsed into `MaterialUvMap`; applied per map in `scene.frag` via `gltf_map_uv.glsl` |

**Still open:** morph targets, skin palette splitting beyond 128 joints.

## Completed (Phase 5 — KHR PBR extensions)

| Item | API / behavior |
|------|----------------|
| `KHR_materials_clearcoat` | Loader → `MaterialGltfExtensions`; shaded in `gltf_pbr_extensions.glsl` |
| `KHR_materials_transmission` | Screen-space sample of opaque HDR scratch (`VulkanSceneOpaqueBackground`, binding **13**) after opaque pass |
| `KHR_materials_iridescence` | Loader + thin-film specular modulation |
| `KHR_materials_variants` | Variant names on submeshes; runtime index on `MultiMaterialComponent` |
| `normalTexture.scale` | `MaterialComponent::normalScale` |
| BRDF LUT | `VulkanIblBrdfLut` (binding **12**) for split-sum IBL |

**Still open:** sheen, specular-glossiness, multi-layer / OIT transmission.

## Phase 6 — animation & advanced

- Morph targets
- Rigid node animation
- Scene snapshot serialization

## SparkDemo reference (`GltfSamples3DDemo`)

Launcher item **#21** (hotkey **Q**) demonstrates end-to-end glTF PBR display:

- Khronos **`DamagedHelmet.glb`** via `GltfAssetBinder::BindRigidMesh` + `ApplyGltfMaterialDesc` (normal, ORM, emissive maps)
- Poly Haven **`studio_small_08_1k.hdr`** sky dome for image-based lighting (`iblEnvironmentLayer = -1`, `SkyComponent` + equirect texture)
- CMake downloads the HDR to `assets/textures/sky/` on first configure (`SPARK_STUDIO_HDR_PATH`)

See [`GLTF_DISPLAY_ROADMAP.md`](GLTF_DISPLAY_ROADMAP.md) for the full compatibility matrix and [`MATERIALS_AND_LIGHTING.md`](MATERIALS_AND_LIGHTING.md) for PBR channel details.

## Test matrix

Run `GltfDisplayCompatibilityTest` plus manual checks:

1. DamagedHelmet.glb — rigid PBR  
2. SheenChair.glb — multi-material rigid  
3. Fox.glb — skinned bind pose (no animator)  
4. Draco sample — `GltfDracoTest` (AvocadoDraco.gltf)  
5. Meshopt sample — `GltfMeshoptTest` (MeshoptCubeTest.gltf)  
6. Skinned decode path — `GltfSkinnedCompressionTest` (Fox.glb)  
7. Multi-node scene — `GltfSceneGraphTest` + `BindFromPath` hierarchy  
8. Factor-only colored mesh — scalar PBR visible  
9. Visual parity — `GltfVisualParityTest` (vertex color/UV path, unlit, multi-skinned scene)

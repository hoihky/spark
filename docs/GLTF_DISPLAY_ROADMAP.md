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

## Phase 3 — Draco / meshopt (next major milestone)

**Goal:** Load compressed geometry from web/CAD exporters.

| Task | Notes |
|------|-------|
| `KHR_draco_mesh_compression` | Integrate Draco decoder; decompress before `AppendPrimitive` |
| `EXT_meshopt_compression` | Optional; meshoptimizer decode pass |
| Classifier extension | Probe reports `Compressed` when extensions present |
| Failure messages | Actionable errors (“enable Draco” / re-export uncompressed) |

**Design:** `IGltfPrimitiveDecoder` strategy — uncompressed path stays default; Draco/meshopt plug in behind the same `AppendPrimitive` interface.

## Phase 4 — visual parity (after display works)

- Vertex colors (`COLOR_0`)
- Per-map texcoord indices + `KHR_texture_transform`
- `KHR_materials_unlit`
- Multiple skinned meshes per file
- Joint count > 64 (split palettes or GPU skinning budget)
- Morph targets

## Test matrix

Run `GltfDisplayCompatibilityTest` plus manual checks:

1. DamagedHelmet.glb — rigid PBR  
2. SheenChair.glb — multi-material rigid  
3. Fox.glb — skinned bind pose (no animator)  
4. Draco sample — expect fail until Phase 3  
5. Multi-node scene — `GltfSceneGraphTest` + `BindFromPath` hierarchy  
6. Factor-only colored mesh — scalar PBR visible  

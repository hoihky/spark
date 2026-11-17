# Scene submit & spatial refactor (factoring log)

Summary of the **2025–2026** restructuring that split monolithic scene submit into focused types and extracted reusable spatial indexes. Public **gameplay APIs are unchanged in spirit**; prefer `LitSceneSubmitOptions` and `LitScene::Builder` over long positional overloads.

**Privacy:** docs and scripts use placeholders (`/path/to/spark`, `SPARK_ENGINE_ROOT`). Before push, run `./tools/check-paths-privacy.sh` — see [PATHS_AND_PRIVACY.md](PATHS_AND_PRIVACY.md).

## Module map

| Area | Headers | Sources | Role |
|------|---------|---------|------|
| **Entry** | `include/spark/scene/submit/SceneSubmit.hpp`, `LitScene.hpp` | `src/spark/scene/submit/SceneSubmit.cpp` | `FillStandardLitSceneFromWorld`, `SubmitStandardLitSceneFromWorld*` |
| **Pipeline** | `LitSceneSubmitter.hpp` | `LitSceneSubmitter.cpp` | Full-world collect orchestration |
| **Collect** | `SceneMeshSubmitCollectors.hpp` | `SceneMeshSubmitCollectors.cpp` | Rigid + skinned frustum sinks |
| **Build** | `SceneRigidMeshDrawBuilder.hpp` | … | Submesh + albedo path |
| **Sort / partition** | `SceneDrawListSorter.hpp` | `SceneDrawListSorter.cpp` | Opaque sort, sprite/tilemap/water order, transparent partition (replaces `SceneSubmitDrawPartition.cpp`) |
| **Domains** | `SceneWaterSubmit.hpp`, `SceneTilemapSubmit.hpp` | `SceneSubmitWater.cpp`, … | Water + tilemap passes inside fill |
| **Cull** | `SceneSpatialCull.hpp` | `src/spark/scene/submit/SceneSpatialCull.cpp` | Frustum dispatch via spatial policy |
| **Lighting / material** | `detail/SceneSubmitLighting.hpp`, … | `SceneSubmitLighting.cpp`, `SceneSubmitMaterial.cpp` | Overrides and texture resolve |
| **Spatial indexes** | `include/spark/scene/spatial/*.hpp` | `src/spark/scene/spatial/*.cpp` | `BvhTree`, `OctTree`, `QuadTree`, `SpatialBoundsUtil` |

Deep dives: [`include/spark/scene/submit/README.md`](../include/spark/scene/submit/README.md), [`include/spark/scene/spatial/README.md`](../include/spark/scene/spatial/README.md), [`SCENE_CORE.md`](SCENE_CORE.md), [`MODULE_LAYOUT.md`](MODULE_LAYOUT.md).

## Naming & style

- Class **`OctTree`** (not `Octree`); enum **`ScenePartitionKind::Octree`** unchanged for serialization/script.
- Spatial tree members use **no trailing `_`** on data fields (project convention in this subtree).
- **`LitScene::Builder`** is canonical; `LitSceneBuilder.hpp` is a **`using LitSceneBuilder = LitScene::Builder`** alias only.

## C# / interop

Script surface and regen notes: **[CSHARP_SCENE_SUBMIT_SYNC.md](CSHARP_SCENE_SUBMIT_SYNC.md)**. Managed companions live in **spark-sharp** (`LitSceneSubmitOptions`, `LitScene.Builder`, `WorldRendering` fill/submit).

**Bindings note:** `Vector3` in **Spark.Bindings** is a readonly struct — use `new Vector3 { X = …, Y = …, Z = … }`, not a three-argument constructor. Blittable interop structs use `FromNative` **by value** (no `in` on generated signatures).

## What stayed internal (not script API)

`LitSceneSubmitter`, `SceneDrawListSorter`, `SceneMeshSubmitCollectors`, and everything under `spark/scene/spatial/` run inside native fill when you call submit/fill from C++ or C#.

## Related docs

| Doc | Updates |
|-----|---------|
| [ARCHITECTURE_AND_DEVELOPER_GUIDE.md](ARCHITECTURE_AND_DEVELOPER_GUIDE.md) | Lit-scene fast path, builder sample |
| [API_POLISH.md](API_POLISH.md) | Facades + lit render naming |
| [BINDINGS_COVERAGE.md](BINDINGS_COVERAGE.md) | What is / is not mirrored to C# |
| [SCENE_AND_RENDERING_GAPS.md](SCENE_AND_RENDERING_GAPS.md) | Submit bridge table |
| spark-sharp [C_SHARP_API.md](https://github.com/hoihky/spark-sharp/blob/main/docs/C_SHARP_API.md) | Lit submit examples |

*Last updated: 2026-11 — spatial extraction + submit pipeline factoring.*
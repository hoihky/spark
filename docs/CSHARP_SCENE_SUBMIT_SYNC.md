# C# sync: scene submit & spatial (engine refactor)

Companion repo: **[spark-sharp](../SPARK_SHARP.md)**. Engine-side factoring log: **[SCENE_SUBMIT_AND_SPATIAL_REFACTOR.md](SCENE_SUBMIT_AND_SPATIAL_REFACTOR.md)**. Regenerate ClangSharp bindings after editing `SparkInterop.h` / `SparkInteropTypes.h`.

**Privacy:** use `SPARK_ENGINE_ROOT=/path/to/spark` in examples; run `./tools/check-paths-privacy.sh` in each repo before push ([PATHS_AND_PRIVACY.md](PATHS_AND_PRIVACY.md)).

## C++ changes consolidated

| Area | C++ layout | Behavior |
|------|------------|----------|
| **Spatial indexes** | `spark/scene/spatial/` | `BvhTree`, `OctTree`, `QuadTree` — used from `SceneSpatialCull` |
| **Lit pipeline** | `LitSceneSubmitter` | Replaces monolithic `SceneSubmit.cpp` fill path |
| **Mesh collect** | `SceneMeshSubmitCollectors` | Rigid + skinned frustum sinks |
| **Sort / partition** | `SceneDrawListSorter` | Opaque/transparent partition, sprite/tilemap/water order |
| **Domains** | `SceneWaterSubmit`, `SceneTilemapSubmit` | Still native-only inside fill |
| **Public entry** | `SceneSubmit.hpp`, `LitScene` | `LitScene::Builder` replaces standalone `LitSceneBuilder` |
| **Options** | `LitSceneSubmitOptions` | `sceneForCulling` enables spatial frustum when `Scene` is set |

## C# surface (spark-sharp)

| C++ | C# |
|-----|-----|
| `LitSceneSubmitOptions` | `LitSceneSubmitOptions` + `SparkLitSceneSubmitOptions` (blittable) |
| `LitScene::Builder` | `LitScene.Builder` |
| `FillStandardLitSceneFromWorld(..., options)` | `world.Rendering.FillStandardLitSceneParams(..., options)` |
| `SubmitStandardLitSceneFromWorldWithCamera` | `LitScene.SubmitWithCamera` / `world.Rendering.SubmitStandardLitWithCamera` |
| `TryFillSceneCameraFromWorld` | `LitScene.TryResolveCamera` / `spark_scene_try_resolve_lit_camera` |
| `ScenePartitionKind` | `ScenePartitionKind` + `SceneSpatialPolicyComponent.Partition` |
| `LitSceneSubmitter`, `SceneDrawListSorter`, spatial trees | **Not** script API (run inside native fill) |

### Frustum culling from C#

Set `LitSceneSubmitOptions.UseSceneFrustumCulling = true` before fill/submit. Native code passes `IEngineContext::TryGetScene()` as `sceneForCulling`. Tune acceleration via `SceneSpatialPolicyComponent.Partition` (`Octree` → engine `OctTree`).

### Deprecated

- `LitSceneBuilder` → use `LitScene.Builder` (`using LitSceneBuilder = LitScene.Builder` alias remains).
- Long positional submit overloads remain but prefer `LitSceneSubmitOptions`.

## New interop symbols

- `spark_scene_fill_standard_lit_from_world_options`
- `spark_scene_submit_standard_lit_from_world_options`
- `spark_scene_try_resolve_lit_camera`
- `spark_scene_submit_standard_lit_with_camera`
- `SparkScenePartitionKind` / `SparkLitSceneSubmitOptions` in `SparkInteropTypes.h`

See **spark-sharp** `docs/C_SHARP_API.md` for usage examples.

### C# bindings quirks

- **`Vector3`** is a readonly struct in generated bindings — use object initializers (`new Vector3 { X = …, Y = …, Z = … }`), not `new Vector3(x, y, z)`.
- **`FromNative`** on blittable structs: pass by value in hand-written wrappers (generated P/Invoke may not accept `in` on argument 1).

*Last updated: 2026-11.*
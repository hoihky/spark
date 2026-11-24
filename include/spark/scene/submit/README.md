# `spark/scene/submit`

ECS → `SceneRenderParams` for the standard lit renderer. Refactor log: `docs/SCENE_SUBMIT_AND_SPATIAL_REFACTOR.md`.

## Layers (what to use)

| Layer | Types | Notes |
|-------|--------|--------|
| **Entry** | `WorldRendering.hpp` | **`world.Rendering().SubmitLit*`** (blessed); `SceneSubmit.hpp` legacy |
| **Pipeline** | `LitSceneSubmitter` | Full-world collect (meshes, 2D, water, lights, foliage) |
| **Domains** | `SceneWaterSubmit`, `SceneTilemapSubmit` | Specialized passes invoked from the pipeline |
| **Collect** | `SceneMeshSubmitCollectors` | Rigid + skinned frustum sinks |
| **Build** | `SceneRigidMeshDrawBuilder`, `SkinnedSceneDrawMaterialApplicator` | Multi-submesh + material/texture resolve |
| **Sort** | `SceneDrawListSorter` | Opaque sort, sprite/tilemap/water order, opaque/transparent partition |
| **Cull** | `SceneSpatialCull`, `SceneSpriteTileCull` | Frustum / tile visibility |
| **Detail** | `detail/SceneSubmitDetail.hpp` | Texture atlas, shadow flags, sky populate (legacy `SceneSubmitDetail::` forwards) |

## Skinned animation (adjacent, not draw sort)

`SkinnedAnimationService`, `SkinnedIkService`, `SkinnedPaletteResolver`, `SkinnedAnimationBudget` — runtime animation policy consumed during skinned collect.

## 2D ordering

`DrawableSortResolver`, `DrawableSortKey`, `RenderLayerRegistry` — sorting layers / render layers for sprites and tilemaps.
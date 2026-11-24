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
| **Materials / textures** | `SceneSubmitMaterials.hpp`, `SceneSubmitTextures.hpp` | Material → draw item; texture layers (prefer over `detail/SceneSubmitDetail.hpp`) |

## Skinned animation (adjacent, not draw sort)

`SkinnedAnimationService`, `SkinnedIkService`, `SkinnedPaletteResolver`, `SkinnedAnimationBudget` — runtime animation policy consumed during skinned collect.

## 2D ordering

`DrawableSortResolver`, `DrawableSortKey`, `SceneRenderQueue`, `RenderLayerRegistry` — sorting layers for sprites, tilemaps, and `SceneDrawItem` mesh submits.
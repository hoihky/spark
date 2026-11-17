# Spark module layout (top-level `include/spark`)

| Former path | Current path | Notes |
|-------------|--------------|--------|
| `spark/character/` | `spark/animation/character/` | Motors, root motion arbitration, simulation pipeline |
| `spark/memory/` | `spark/core/` | `SharedPtr`, `UniquePtr`, `SparkMemory.hpp` |
| `spark/gameflow/` | `spark/gameplay/gameflow/` | `GameFlowCoordinator`, persistent run data |
| `spark/imgui/` | `spark/ui/imgui/` | Dear ImGui layer + Vulkan backend glue |
| `spark/text/` | `spark/ui/text/` | `Font` (mesh/UI text) |
| `spark/save/` | `spark/gameplay/save/` | `GameSave` JSON helpers |

## Scene submit & spatial (2025–2026 factoring)

| Area | Path | Notes |
|------|------|--------|
| Lit submit pipeline | `spark/scene/submit/` | `LitSceneSubmitter`, `SceneDrawListSorter`, thin `SceneSubmit.cpp` |
| Frustum cull glue | `spark/scene/submit/SceneSpatialCull.*` | Uses indexes under `spark/scene/spatial/` |
| Acceleration structures | `spark/scene/spatial/` | `BvhTree`, `OctTree`, `QuadTree` — shared by cull, not script API |

Full change log: [SCENE_SUBMIT_AND_SPATIAL_REFACTOR.md](SCENE_SUBMIT_AND_SPATIAL_REFACTOR.md).

Animation clip/skeleton/IK code remains under `spark/animation/` (`locomotion/`, `combat/`, `ik/`, …).
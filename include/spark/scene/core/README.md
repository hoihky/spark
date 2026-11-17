# `spark/scene/core`

Runtime **world** (ECS + assets + presentation), **scene graph** traversal, **level load**, and **spawn** resolution. Prefer facet entry points on `GameWorld` (`Entities`, `Assets`, `Simulation`, …) — see `world/`.

| Area | Location | Role |
|------|-----------|------|
| World runtime | `GameWorld.hpp`, `GameWorld*.cpp`, `world/` | ECS graph, asset store, presentation; thin facet views |
| Scene container | `Scene.hpp` | Owns one `GameWorld`; `ForEach*` draw/light traversal |
| Load / instances | `load/` | `SceneManager`, `SceneLoadSession`, `SceneInstanceId`, `SceneInstanceTracker` |
| Spawn | `spawn/` | `FindSpawnPoint`, `FindSpawnPoint2D`, injectable `SpawnPoint*Service` |
| Culling / policy | `ScenePartitionKind.hpp`, `SceneDrawableFrustumSink.hpp` | Frustum dispatch for `Scene::ForEach*InViewFrustum` |
| Spatial indexes | `../spatial/` | `BvhTree`, `OctTree`, `QuadTree` for drawable frustum cull |
| Editor helpers | `SceneEntityRole.hpp` | Classify serialized entities for hierarchy UI |

**Includes:** `spark/scene/core/load/…` and `spark/scene/core/spawn/…` only (no root-level aliases).

Full map: `docs/SCENE_CORE.md`.
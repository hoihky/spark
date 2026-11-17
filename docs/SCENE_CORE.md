# Scene core layout

`include/spark/scene/core` is the **in-memory game world** layer (not file formats under `scene/serialization`, not GPU submit under `scene/submit`).

## Dependency sketch

```text
Scene                    GameWorld
  | owns 1:1                |
  ForEach* ───────────────► Entities().ForEachActive
                             ├── entityGraph (GameWorldEntityGraph)
                             ├── assetStore (GameWorldAssetStore)
                             └── presentation (VFX, wind, skinned, UI fonts)
                             Facets: world/World*.hpp (no extra state)
```

## Files in use (audited)

| File | Used by | Notes |
|------|---------|--------|
| `GameWorld` + entity graph / asset store / presentation | Engine, demos, editor, tests | Flat `CreateGameObject` delegates to graph; prefer `Entities().Create()` |
| `world/World*` facets | `Game::OnUpdate`, demos, gameplay | Implemented in `GameWorldFacets.cpp` |
| `Scene.hpp` | `Game`, submit, editor | Header-only traversal templates + `RaycastPick` |
| `load/SceneManager` | Level loads, prefab host, platformer pipeline | Additive `.sparkscene` instances |
| `load/SceneLoadSession` | Platformer, tilemap showcase, editor play | Blocking / async pump wrapper |
| `load/SceneInstanceTracker` | Editor, scene editor demo | Bookkeeping for unload |
| `spawn/*` | `SceneManager`, platformer, 2D flow demo | 3D + 2D spawn pose structs |
| `SceneEntityRole` | Editor content model, 3D scene demo | Not used in shipping game loop |
| `EntityId` | `GameObject`, facets | Typed object id |
| `ScenePartitionKind`, `SceneDrawableFrustumSink` | `Scene`, spatial cull submit | |

Nothing in this tree is orphaned; overlap was **organizational** (spawn vs load vs world facets).

## Prefer vs legacy

| Prefer | Legacy on `GameWorld` |
|--------|------------------------|
| `world.Entities().Create()` | `CreateGameObject()` |
| `world.Entities().ForEachActive` | `ForEachActiveGameObject` (removed) |
| `world.QueryObjects().CollectWithTag` | `CollectGameObjects` / `FindGameObjectWithTag` |
| `load/SceneLoadSession` | Calling `SceneManager` pump manually only |

## Refactor notes (2025)

- **`spawn/`** and **`load/`** are the public include paths (e.g. `spark/scene/core/load/SceneManager.hpp`).
- **`SpawnPoint*Service`** implementations live in `SceneSpawn.cpp` / `SceneSpawn2D.cpp` (single TU per axis).
- Removed unused **`WorldSceneIteration.hpp`** — use `world.Entities()` directly.
# C++ / C# API polish (incremental)

This document tracks the object-oriented façade layer added without removing legacy entry points. Performance-sensitive paths stay inline or delegate with no extra allocations.

## Principles

1. **Views over god-methods** — `GameWorld::Entities()`, `Assets()`, `Simulation()`, `Rendering()`, `Physics(subsystem)` group related calls (mirrors spark-sharp; base type is `WorldView`, not `*Facet`).
2. **Decomposed world** — `GameWorldEntityGraph` (entities), `GameWorldAssetStore` (cache+loader), `GameWorldPresentation` (VFX/wind/skinned/fonts); `WorldAssets` implements the asset surface.
3. **Subsystem classes over loose scripts** — `AiSubsystem`, `SoundSubsystem`, `PhysicsSubsystem`, `NavSubsystem`, `GameplaySubsystem` own frame pipelines; free functions remain as thin wrappers.
4. **Short names for verbose pipelines** — `LitScene::Fill` / `Submit` wrap `FillStandardLitSceneFromWorld` / `SubmitStandardLitSceneFromWorld`.
5. **No behavior change** — old symbols keep working; new names are preferred in new code and in `Spark::Game`.

## C++ (implemented)

| Area | Preferred API | Legacy (still valid) |
|------|----------------|----------------------|
| World graph | `world.Entities().Create()` | `CreateGameObject()` |
| Assets | `world.Assets().Pump()` | — (flat `GameWorld` asset API removed) |
| Frame slice | `world.Simulation().RunDefaultSlice(timing, ctx)` | manual update + `ProcessSoundCues` |
| Post-physics gameplay | `world.Simulation().RunGameplaySlice()` after `Physics().Simulate2D` | `SparkGameplayAfterPhysics2D` |
| Interact press | `world.Gameplay().InteractPress(instigator)` | `ProcessInteractables2D` |
| Audio | `SoundSubsystem::ProcessFrame` | `ProcessSoundCues` |
| AI | `AiSubsystem::RunFrame` | `SimulateGameAi` |
| Navigation | `NavSubsystem::ProcessGridNav2D` | `ProcessGridNavAgents2D` |
| Lit render | `world.Rendering().SubmitLit(...)` / `LitScene::Submit` | `SubmitStandardLitSceneFromWorld` |
| Scene policy | `scene.ApplySpatialPolicy()` | `ApplySpatialPolicyFromFirstMatchingObject` |
| Physics | `world.Physics(physics).Simulate3D(timing)` | `physics.Simulate3D(world, timing)` |
| Batched query | `world.QueryObjects().CollectWithTag("Enemy", buffer)` | `CollectGameObjects` / `FindGameObjectWithTag` |
| C# query | `world.Query.Collect(filter, buffer)` / `Query(filter)` | same interop via `spark_world_query_collect_filtered` |
| Demo assets | `world.Assets().RegisterTexture(...)` | — |

## Layout (Step 1–2)

| Piece | Role |
|-------|------|
| `compat/GameWorldEntityApi.inl` | Legacy iteration templates → `entityGraph_` |
| `WorldEntities` + `WorldEntitiesIteration.inl` | Preferred entity iteration / `Select` |
| `QueryFilter` | Builders in `GameObjectQuery.hpp` |
| `WorldQuery` | `CollectRoots`, `CollectScene`, batched collect |

## Step 3 — gameplay slice

| Piece | Role |
|-------|------|
| `GameplaySubsystem` | `AfterPhysics2D` (pickup deferred destroys), `ProcessInteractPress` |
| `WorldGameplay` | `world.Gameplay()` facet |
| `WorldSimulation::RunGameplaySlice` | Call after 2D physics (projectiles still flush inside `PhysicsSubsystem::Simulate2D`) |

## Next increments (not done yet)

- Engine, editor, and tests use `world.Assets()` for load/register/query (flat asset API removed).
- `GameplaySnapshotRegistry` single registrar instead of many `Register*SnapshotHandlers`.
- Editor / ImGui facades on `IEngineContext`.
- Expose `LitSceneBuilder` to C# interop when script games need fluent submit.

## C# regeneration

From **spark-sharp** with `SPARK_ENGINE_ROOT` pointing at this repo:

```bash
./tools/generate-csharp-bindings.sh
```

Companion files under `Spark.Bindings/World/` should stay aligned with the C++ facet names above.

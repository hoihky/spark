# C++ / C# API polish (incremental)

This document tracks the object-oriented façade layer added without removing legacy entry points. Performance-sensitive paths stay inline or delegate with no extra allocations.

## Principles

1. **Views over god-methods** — `GameWorld::Entities()`, `Assets()`, `Simulation()`, `Rendering()`, `Physics(subsystem)` group related calls (mirrors spark-sharp; base type is `WorldView`, not `*Facet`).
2. **Decomposed world** — `GameWorldEntityGraph` (entities), `GameWorldAssetStore` (cache+loader), `GameWorldPresentation` (VFX/wind/skinned/fonts); `WorldAssets` implements the asset surface.
3. **Subsystem classes over loose scripts** — `AiSubsystem`, `SoundSubsystem`, `PhysicsSubsystem`, `NavSubsystem` own frame pipelines; free functions remain as thin wrappers.
4. **Short names for verbose pipelines** — `LitScene::Fill` / `Submit` wrap `FillStandardLitSceneFromWorld` / `SubmitStandardLitSceneFromWorld`.
5. **No behavior change** — old symbols keep working; new names are preferred in new code and in `Spark::Game`.

## C++ (implemented)

| Area | Preferred API | Legacy (still valid) |
|------|----------------|----------------------|
| World graph | `world.Entities().Create()` | `CreateGameObject()` |
| Assets | `world.Assets().Pump()` | `PumpAssets()` |
| Frame slice | `world.Simulation().RunDefaultSlice(timing, ctx)` | manual update + `ProcessSoundCues` |
| Audio | `SoundSubsystem::ProcessFrame` | `ProcessSoundCues` |
| AI | `AiSubsystem::RunFrame` | `SimulateGameAi` |
| Navigation | `NavSubsystem::ProcessGridNav2D` | `ProcessGridNavAgents2D` |
| Lit render | `world.Rendering().SubmitLit(...)` / `LitScene::Submit` | `SubmitStandardLitSceneFromWorld` |
| Scene policy | `scene.ApplySpatialPolicy()` | `ApplySpatialPolicyFromFirstMatchingObject` |
| Physics | `world.Physics(physics).Simulate3D(timing)` | `physics.Simulate3D(world, timing)` |
| Batched query | `world.QueryObjects().CollectWithTag("Enemy", buffer)` | `CollectGameObjects` / `FindGameObjectWithTag` |
| C# query | `world.Query.Collect(filter, buffer)` / `Query(filter)` | same interop via `spark_world_query_collect_filtered` |
| Demo assets | `world.Assets().RegisterTexture(...)` | flat `world.RegisterTexture` (still on `GameWorld`) |

## Next increments (not done yet)

- Move remaining flat `GameWorld::Load*` call sites to `world.Assets()` (compat forwards stay on `GameWorld`).
- `GameplaySnapshotRegistry` single registrar instead of many `Register*SnapshotHandlers`.
- Editor / ImGui facades on `IEngineContext`.
- Expose `LitSceneBuilder` to C# interop when script games need fluent submit.

## C# regeneration

From **spark-sharp** with `SPARK_ENGINE_ROOT` pointing at this repo:

```bash
./tools/generate-csharp-bindings.sh
```

Companion files under `Spark.Bindings/World/` should stay aligned with the C++ facet names above.

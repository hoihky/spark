# P0 2D product path

This page documents the blessed patterns for shipping a code-first 2D game on Spark. Phase D editor UX (palette, chunked maps, TMX export parity) remains on the [tilemap editor roadmap](../2-2d-graphics/04-tilemap-editor-roadmap.md).

**Hands-on reference:** SparkDemo **#26 / hotkey F — “2D P0 product path”** (`GameFlow2DProductPathDemo`).  
**API catalog:** [Gameplay API guide](09-2d-gameplay-api-guide.md) · [Components handbook](10-2d-components-handbook.md).

## Level pipeline (authored map)

1. Author tiles in Tiled (`sampleMap.tmx`) and check in a minimal `platformer_level.sparkscene` (or P0-specific scene) with:
   - `Level` entity: `tilemap_map_source` → TMX path, tile size, layers.
   - `PlayerSpawn` entity: `spawn_point` named `Player`.
2. At runtime, load via `SceneLevelLoader` + `SceneLoadSession` (same stack as the tilemap showcase).
3. On the level root, attach:
   - `TilemapGameplayGridComponent` — walkability for nav and spawn validation
   - `TilemapCollider2DComponent` — static tile collision
   - `TilemapObjectLayerComponent` + `TilemapObjectSpawnComponent` — marker-driven spawns
4. Register handlers on `TilemapObjectSpawnRegistry` (`p0_gem`, `p0_goal` in P0) and call `RespawnAll`.

**Bridge:** Kenney `sampleMap.tmx` may lack object layers. P0 seeds markers in code until a companion `.sparkmap` ships.

**Classic platformer:** `Platformer2DDemo` (**#6**) keeps procedural sprite platforms for side-view teaching.

## Gameplay data (`p0_demo.sparkgameplay`)

| Key | Role |
|-----|------|
| `p0.gems_required` | Gems to reveal goal |
| `p0.gem_pool_size` | `GameObjectPool` warm count |
| `p0.move_speed_scale` | Player WASD / path speed |
| `p0.patrol_enabled` / `p0.patrol_speed_scale` | Green patrol NPC |
| `p0.chaser_enabled` / `p0.chaser_speed_scale` | Red chaser NPC |
| `p0.gem_dissolve_seconds` | Dissolve duration after pickup |

## Simulation frame order (P0 demo)

`SparkShellDemo` calls `GameFlow2DProductPathDemo::Simulate` **before** `Game::OnUpdate`. Inside `Simulate` when playing:

1. `TickProductPatrolAssignGoals` → `ProcessGridNavAgents2D` → patrol/chaser motion helpers
2. Player click repath + keyboard or `ApplyGridNavAgent2DRigidbodyMotion`
3. `physics.Simulate2D`
4. `TickGemPickupDissolves` → proximity collect → goal check → camera sync → HUD

Patrol assigns the next **grid cell goal** and `RequestRepath` **before** `ProcessGridNavAgents2D` so paths exist the same frame motion runs.

## Sprite FX, grid AI, composite minimap

| Topic | P0 demo behavior |
|--------|------------------|
| **Sprite FX** | Gems: `ApplyOutline`; on collect: `ApplyDissolveProgress` + scale shrink, then pool `Release`. Player: `ApplyHitFlashAtSceneTime` (`sprite.frag` modes 13–15). |
| **2D AI** | Green **patrol** — four walkable waypoints, `GridCell` goals, `ApplyGridNavAgent2DRigidbodyMotion`. Red **chaser** — `TargetObject` = player, steering/rigidbody motion. Tune via gameplay table. |
| **Composite minimap** | `Scene2DCompositeViewComponent` on flow object; RGBA8 `RenderTexture`; GPU ortho capture + UI atlas blit. See [runtime limits](../7-2d-game/08-scene2d-runtime-limits.md). |
| **Pools** | `prefabs/p0_gem.sparkscene`; `PickupComponent::SetDestroyOwnerOnCollect(false)` for dissolve-then-release. |

## Scene flow (title → play → pause → victory → reload)

| Concern | Where it lives |
|--------|----------------|
| Per-scene state machine | `GameStateComponent` on a manager entity |
| Triggers (goal, death) | `GameFlowTriggerComponent` |
| Cross-scene progress | `GameFlowPersistentData` + `GameSave` (`spark_save_v1`) |
| Reload / additive scenes | `GameFlowCoordinator` + `SceneManager` |

Typical loop:

1. **Title / intro** — `GameFlowState::Intro` (optional UI scene loaded additively).
2. **Play** — load level `.sparkscene`, `RequestState(Playing)`.
3. **Pause** — `PushState(Paused)` (**P** in P0 demo).
4. **Victory** — goal trigger or script calls `Victory`; coordinator syncs gems to `GameSave`.
5. **Reload** — **R** or `GameFlowCoordinator::ReloadActiveLevel`.

Persistent data lives **outside** ECS so entity handles are not saved across reloads.

## Serialization

Handlers for `game_state`, `game_flow_trigger`, `input_action_map`, `player_input`, and `tilemap_object_spawn` live in `ComponentSnapshotHandlersGameFlow.cpp`. Extend this set when new gameplay components join your scene template.

Integration test: `PlatformerLevelSceneRoundTripTest` loads `platformer_level.sparkscene`, captures, and verifies TMX path + player spawn after reload.

## Player progress / settings

Use `GameSave::TryLoad` / `TrySave` with a versioned `GameSaveSlot`. Hook saves from `GameState` transitions or `GameFlowCoordinator::SyncProgressFromGameplay`.

Default path (demo): `GameSave::DefaultSlotPath()` under build assets `save/slot0.savespark`.

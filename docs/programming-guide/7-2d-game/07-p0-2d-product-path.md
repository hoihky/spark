# P0 2D product path

This page documents the blessed patterns for shipping a code-first 2D game on Spark. Phase D editor UX (palette, chunked maps, TMX export parity) remains on the [tilemap editor roadmap](../2-2d-graphics/04-tilemap-editor-roadmap.md).

## Level pipeline (authored map)

1. Author tiles in Tiled (`sampleMap.tmx`) and check in a minimal `platformer_level.sparkscene` with:
   - `Level` entity: `tilemap_map_source` → TMX path, tile size, layers.
   - `PlayerSpawn` entity: `spawn_point` named `Player`.
2. At runtime, `Platformer2DLevelPipeline` loads the scene via `SceneLevelLoader` + `SceneLoadSession` (same stack as the tilemap showcase).
3. Attach `TilemapCollider2DComponent`, `TilemapObjectLayerComponent`, and `TilemapObjectSpawnComponent` on the level root.
4. Register gameplay types on `TilemapObjectSpawnRegistry` (`gem`, `enemy`, `goal` in the platformer demo) and call `RespawnAll`.

**Bridge:** Kenney `sampleMap.tmx` has no object layers yet. `Platformer2DLevelPipeline::SeedObjectMarkers` places markers from the demo layout until a companion `.sparkmap` ships. Replace seeding with imported markers when the editor path is ready.

**Reference:** Run shell demo **#26 / hotkey F — “2D P0 product path”** (`GameFlow2DProductPathDemo`). The teaching **platformer** (`Platformer2DDemo`) keeps the classic procedural layout for playability.

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
3. **Pause** — `PushState(Paused)` (Escape in the platformer demo).
4. **Victory** — trigger or script calls `Victory`; coordinator syncs gems/defeats to `GameSave`.
5. **Reload** — `GameFlowCoordinator::ReloadActiveLevel` unloads and reloads the active path.

Persistent data intentionally lives **outside** ECS so entity handles are not saved across reloads.

## Serialization

Handlers for `game_state`, `game_flow_trigger`, `input_action_map`, `player_input`, and `tilemap_object_spawn` live in `ComponentSnapshotHandlersGameFlow.cpp`. Extend this set when new gameplay components join your scene template.

Integration test: `PlatformerLevelSceneRoundTripTest` loads `platformer_level.sparkscene`, captures, and verifies TMX path + player spawn after reload.

## Player progress / settings

Use `GameSave::TryLoad` / `TrySave` with a versioned `GameSaveSlot`. Hook saves from `GameState` transitions or `GameFlowCoordinator::SyncProgressFromGameplay` — do not invent ad hoc JSON per game.

Default path (demo): `GameSave::DefaultSlotPath()` under build assets `save/slot0.savespark`.

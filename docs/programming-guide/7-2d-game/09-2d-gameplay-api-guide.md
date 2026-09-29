# 2D gameplay API guide

This chapter is a **practical catalog** of Spark APIs for 2D games: loading maps, collision, object spawns, camera, grid navigation, NPC behavior, pickups, minimap, and presentation FX. It aligns with `GameFlow2DProductPathDemo` (shell **F**) and `TilemapShowcase2DDemo` (**#19**).

For a **component-by-component** index (all **108** `ComponentKind` values, 2D vs 3D), use [10 — Components handbook](10-2d-components-handbook.md) alongside the [full reference](../1-overview-architecture/07-game-component-reference.md).

---

## 1. Loading a tilemap level

### 1.1 Scene + TMX on the level root

Author a minimal `.sparkscene` with a **Level** entity that references your TMX:

- `TilemapMapSourceComponent` — path, pixels-per-world-unit, hot reload
- `TilemapComponent` — created or filled by import

```cpp
#include "spark/ecs/components/tilemap/TilemapMapSourceComponent.hpp"
#include "spark/ecs/components/rendering/TilemapComponent.hpp"

GameObject* level = world.CreateGameObject();
auto* source = level->AddComponent<TilemapMapSourceComponent>();
source->SetTmxPath("sprites/kenney_tiny-dungeon/Tiled/sampleMap.tmx");
source->SetPixelsPerWorldUnit(16.0F);
source->SetImportOnAttach(true);
source->ImportNow(*level, world);
```

`ImportNow` runs `TmxImporter` → `ApplyTilemapDocument`, which loads tileset textures, builds layers, and (for Kenney dungeon packs) applies gameplay tile definitions and layer flags.

See [Tilemaps](../2-2d-graphics/03-tilemaps.md) for coordinate conventions (+Y up, cell origin bottom-left).

### 1.2 Spawn point from the scene

Use a named `spawn_point` entity in `.sparkscene` (e.g. `PlayerSpawn`) or a default cell when the map has no object layer yet. `GameFlow2DProductPathDemo` resolves `PlayerSpawn` and falls back to a known walkable cell on `sampleMap.tmx`.

---

## 2. Tile layers: visuals vs gameplay vs collision

Each `TilemapLayer` has flags you set in code or via import rules:

| Flag | Effect |
|------|--------|
| `contributeCollision` | `TilemapCollider2DComponent` bakes static shapes from tile definitions |
| `contributeGameplayGrid` | Cell included when baking walkability for pathfinding |
| `orderInLayerOffset` / `sortMode` | Draw order and optional world-Y sort |

**Tileset** (`TileDefinition` per tile id):

| Field | Effect |
|-------|--------|
| `collisionShape` | `FullCell`, `HalfCell`, `None`, custom convex |
| `flags` | e.g. `BlocksPathfinding`, `ForceWalkable` |

```cpp
tileset->Definition(wallId).collisionShape = TileCollisionShape::FullCell;
tileset->Definition(wallId).flags = TileDefinitionFlags::BlocksPathfinding;
tileset->Definition(floorId).collisionShape = TileCollisionShape::None;
```

Decor layers: `contributeGameplayGrid = false`, `contributeCollision = false`.

---

## 3. Collision detection layer (physics)

### 3.1 Tilemap static collision

Attach on the **same** object as `TilemapComponent`:

```cpp
#include "spark/ecs/components/physics/2d/TilemapCollider2DComponent.hpp"

auto* tileCollider = level->AddComponent<TilemapCollider2DComponent>();
// Uses layers with contributeCollision + tile collision shapes
```

`PhysicsSubsystem::Simulate2D` resolves dynamic bodies (player, NPCs) against these statics each frame.

### 3.2 Per-entity colliders

| Component | Use |
|-----------|-----|
| `BoxCollider2DComponent` | Player, crates, platforms (manual layout) |
| `CircleCollider2DComponent` | Round pickups, projectiles |
| `TriggerVolume2DComponent` | Overlap only — gems, goals, zones (`Physics2DTriggerEnter`) |
| `Rigidbody2DComponent` | `Dynamic` (movable), `Static` (immovable), `Kinematic` (scripted) |

**Categories / masks** — set on colliders so enemies do not block pickups, etc. See [Colliders](../5-physics/03-colliders.md).

### 3.3 Grid-only blocking (no extra collider)

For top-down grid games you can combine:

- `TilemapGameplayGridComponent` for **logical** walkability (AI + spawn placement)
- `TilemapCollider2DComponent` for **physical** blocking

`GameFlow2DProductPathDemo::CorrectPlayerAgainstBlockedGrid` shows an extra guard: if the player ends on a blocked cell after physics, snap back to the last valid position.

---

## 4. Gameplay grid and walkability

```cpp
#include "spark/ecs/components/tilemap/TilemapGameplayGridComponent.hpp"

auto* walkGrid = level->AddComponent<TilemapGameplayGridComponent>();
walkGrid->SetWalkRule(TilemapGameplayWalkRule::DefinitionAndFlags);
walkGrid->SetAutoRebake(true);
walkGrid->RebakeIfNeeded(*level);

const TilemapGridFrame& frame = walkGrid->GetGridFrame();
const IGridWalkability& walk = walkGrid->GetWalkability();
```

| `TilemapGameplayWalkRule` | When to use |
|---------------------------|-------------|
| `OccupiedWalkable` | Any painted gameplay-layer cell is walkable |
| `DefinitionAndFlags` | **Default** — respects tile flags + collision metadata |
| `CollisionAligned` | Blocked when tile would contribute physics collision |

**Coordinates:** `frame.WorldXYToCell`, `frame.CellCenterToWorldXY`, `frame.IsCellInBounds`.

---

## 5. Object layers (markers and spawning)

Object layers are **not** TMX tile layers; they are logical marker lists used for gameplay spawn.

```cpp
#include "spark/ecs/components/tilemap/TilemapObjectLayerComponent.hpp"
#include "spark/ecs/components/tilemap/TilemapObjectSpawnComponent.hpp"
#include "spark/scene/tilemap/TilemapObjectSpawnRegistry.hpp"

auto* objects = level->AddComponent<TilemapObjectLayerComponent>();
const std::uint32_t layerIndex = objects->AddObjectLayer("P0Objects");

TilemapObjectMarker gem{};
gem.typeId = Utf8String("p0_gem");
gem.cellX = 12;
gem.cellY = 8;
static_cast<void>(objects->AddMarker(layerIndex, gem));

level->AddComponent<TilemapObjectSpawnComponent>();
```

Register a handler per `typeId`:

```cpp
TilemapObjectSpawnRegistry::Default().Register("p0_gem", &SpawnProductGem);
// Handler signature: GameObject*(GameWorld&, GameObject& mapOwner, const TilemapObjectMarker&, const TilemapGridFrame&)
```

Call **`RespawnAll`** on `TilemapObjectSpawnComponent` after markers are ready (P0 demo does this in `SetupGameplaySpawnAndMarkers`).

**Seeding without TMX objects:** demos may call `SeedObjectMarkers` until a `.sparkmap` exports object layers from the editor.

---

## 6. Camera control

### 6.1 Code-driven `Camera2D` (P0 demo)

```cpp
#include "spark/scene/camera/Camera2D.hpp"

Camera2D camera{};
camera.halfExtentY = 8.0F;
camera.position = playerWorldPosition;

// Each frame after moving the player:
camera.position.x = playerTr->GetLocalTransform().translation.x;
camera.position.y = playerTr->GetLocalTransform().translation.y;

Matrix4 vp = camera.ViewProjection(fbW, fbH);
```

Scroll wheel / keys can adjust `halfExtentY` for zoom (clamped to map scale).

### 6.2 ECS rig (platformer / polish)

```cpp
#include "spark/ecs/components/rendering/Camera2DComponent.hpp"
#include "spark/ecs/components/rendering/Camera2DRigComponent.hpp"
#include "spark/ecs/components/rendering/ScreenShakeComponent.hpp"

auto* camGo = world.CreateGameObject();
camGo->AddComponent<Camera2DComponent>()->SetHalfExtentY(6.5F);
auto* rig = camGo->AddComponent<Camera2DRigComponent>();
rig->SetMode(Camera2DRigMode::FollowTarget);
rig->SetTarget(playerObject);
camGo->AddComponent<ScreenShakeComponent>();
```

See [Camera and HUD](../7-2d-game/05-camera-hud.md).

### 6.3 Screen → world → grid cell (click-to-move)

Use the same ray convention as other Spark 2D demos (framebuffer Y **down**). Unproject with `camera.ViewProjection`, intersect Z=0 plane, then:

```cpp
GridPathfinder::Cell cell = frame.WorldXYToCell(worldXY);
if (walk.IsWalkable(cell.x, cell.y)) { /* set nav goal */ }
```

Helpers: `TryPickWalkableGridCellFromScreen` patterns in `GameFlow2DProductPathDemo` / tilemap showcase.

---

## 7. Grid navigation (player and NPCs)

### 7.1 Components

| Component | Role |
|-----------|------|
| `TilemapGameplayGridComponent` | On **map** object — walkability + `TilemapGridFrame` |
| `GridNavAgent2DComponent` | On **mover** — A* replan, stores cell path + world waypoints |
| `GridNavTarget2DComponent` | On **goal entity** — optional snap for `TargetObject` mode |
| `GridPathFollower2DComponent` | Optional ECS follower (priority 125); demos often use motion helpers instead |

`GridNavAgent2DComponent` settings:

```cpp
nav->SetGridSourceObject(levelRoot);
nav->SetGoalMode(GridNavGoalMode2D::GridCell);       // or WorldPosition, TargetObject
nav->SetGoalCell({gx, gy});
nav->SetGoalTarget(playerObject);
nav->SetRepathEveryFrame(false);
nav->SetRepathIntervalSeconds(0.35F);
nav->SetSyncToAiAgent(false);  // true copies path into AiAgentComponent for steering showcase
nav->RequestRepath();
```

### 7.2 Frame order (critical)

Call **`ProcessGridNavAgents2D(world, deltaTime)`** so agents rebuild paths **before** you apply velocity from the path.

Recommended loop (P0 product path):

```text
1. TickPatrolAssignGoals()     // set next waypoint + RequestRepath when leg finished
2. ProcessGridNavAgents2D()
3. TickPatrolMotion()          // ApplyGridNavAgent2DRigidbodyMotion
4. TickChaserMotion()          // same for chase target
5. Player input / click repath + ApplyGridNavAgent2DRigidbodyMotion
6. physics.Simulate2D()
```

`ApplyGridNavAgent2DRigidbodyMotion` and `ApplyGridNavAgent2DSteeringMotion` live in `spark/ai/NavigationSubsystem.hpp`.

`SimulateGameAi` also calls `ProcessGridNavAgents2D` — if you run custom sim **before** `Game::OnUpdate`, avoid double-processing or pick one place for nav ticks.

### 7.3 Player: keyboard vs click-to-move

- **Keyboard:** clear nav path, set `Rigidbody2D` velocity directly.
- **Click:** `SetGoalCell` / `SetGoalWorldPosition`, `RequestRepath()`, then `ApplyGridNavAgent2DRigidbodyMotion` when not using keyboard.

### 7.4 NPC patrol (cell loop)

Pattern used in P0 demo:

1. Build `Array<GridPathfinder::Cell>` route (e.g. four angular samples around spawn).
2. Place NPC at `route[0]`.
3. Each finished leg: set goal to `route[index]`, `RequestRepath()`, advance index modulo count.
4. Use **`patrolAwaitingNextGoal`** (or equivalent) so waypoints advance only after the path clears — not every frame `!HasPath()`.

### 7.5 NPC chase

```cpp
chaserNav->SetGoalMode(GridNavGoalMode2D::TargetObject);
chaserNav->SetGoalTarget(playerObject);
chaserNav->SetRepathIntervalSeconds(0.35F);
```

Player should have `GridNavTarget2DComponent` if you rely on snap rules on the goal entity.

---

## 8. Gameplay data and object pools

### 8.1 `GameplayDataTable`

Load tuned constants from `assets/gameplay/p0_demo.sparkgameplay`:

| Key | Example | Purpose |
|-----|---------|---------|
| `p0.gems_required` | `3` | Win condition |
| `p0.gem_pool_size` | `8` | Pool capacity |
| `p0.move_speed_scale` | `1.0` | Player speed multiplier |
| `p0.patrol_enabled` | `1` | Toggle green patrol NPC |
| `p0.patrol_speed_scale` | `0.75` | Patrol speed |
| `p0.chaser_enabled` | `1` | Toggle red chaser |
| `p0.chaser_speed_scale` | `0.92` | Chaser speed |
| `p0.gem_dissolve_seconds` | `0.45` | Pickup dissolve duration |

```cpp
gameplayTable.LoadFromFile("gameplay/p0_demo.sparkgameplay");
int gemsRequired = gameplayTable.GetInt("p0.gems_required", 3);
```

### 8.2 `GameObjectPool` + prefab

```cpp
#include "spark/scene/spawn/GameObjectPool.hpp"

gemPool = MakeUnique<GameObjectPool>();
gemPool->Warm(world, sceneManager, "prefabs/p0_gem.sparkscene", poolSize);

GameObject* gem = gemPool->Acquire(world, "prefabs/p0_gem.sparkscene", nullptr);
ConfigureSpawnedGem(*gem, worldXY, frame);  // reset transform, pickup callback, FX

gemPool->Release("prefabs/p0_gem.sparkscene", gem);
```

Use pools when collectibles respawn or you want to avoid allocate/destroy churn.

---

## 9. Pickups, triggers, and dissolve FX

### 9.1 `PickupComponent`

```cpp
#include "spark/ecs/components/gameplay/PickupComponent.hpp"

pickup->SetItemId("gem");
pickup->SetDestroyOwnerOnCollect(false);  // true = destroy at end of physics step
pickup->SetAutoCollectOnTriggerEnter(true);
pickup->SetOnCollected([&](GameObject& collector, const char* id, int qty) {
    BeginGemPickupDissolve(*gem, sceneTimeSeconds);
});
```

Pair with `TriggerVolume2DComponent` (circle/box) on the gem. Proximity collect can also call `TryCollect` from game code (P0 uses both trigger and radius check).

**Pool-friendly collect:** disable auto-collect during dissolve; on finish call `ResetForRespawn()` and `Release` back to the pool.

### 9.2 Sprite FX (`SpriteFx2D`)

Helpers in `spark/render/sprites2d/SpriteFx2D.hpp` drive `SpriteLighting2DComponent` modes consumed by `sprite.frag`:

| Helper | Mode | Typical use |
|--------|------|-------------|
| `ApplyOutline` | 14 | Gem highlight, NPC outline |
| `ApplyHitFlashAtSceneTime` | 13 | Player flash on pickup (`param1.y` = flash start time) |
| `ApplyDissolveProgress` | 15 | Gem pickup dissolve (`progress` 0→1) |

Add or reuse `SpriteLighting2DComponent` on the sprite entity; scene submit copies mode + params into the GPU instance buffer.

---

## 10. Minimap and composite views

### 10.1 Authoring component

```cpp
#include "spark/ecs/components/rendering/Scene2DCompositeViewComponent.hpp"
#include "spark/render/sprites2d/Scene2DMinimap.hpp"

auto* view = flowObject->AddComponent<Scene2DCompositeViewComponent>();
view->SetFeature(Scene2DCompositeFeature::Minimap);
view->SetTarget(minimapRenderTexture);
view->SetHudTexture(minimapHudTexture);
view->SetWorldCapture(worldCenter, orthoHalfExtent);
view->SetScreenRect(0.82F, 0.02F, 0.16F, 0.16F);
view->SetEnabled(true);
```

Each frame, sync ortho bounds with the walkable map (`ComputeMinimapOrthoBoundsSquare`, `SyncMinimapCompositeView` in P0 demo).

### 10.2 Render texture format (GPU path)

Minimap targets use **`RenderTextureFormat::Rgba8Unorm`** via `CreateMinimapRenderTexture()` so the capture can be copied into the UI sprite atlas (RGBA8).

Composite capture (`VulkanScene2DCompositeCapture`):

- Ortho resubmit of tilemaps + sprites into the offscreen target
- Uses an **LDR offscreen render pass** (RGBA8) for LDR targets, HDR pass for `HdrRGBA16Float` targets
- After the main HDR world pass, blit into the HUD `uiTextures` layer

CPU fallback: `RebuildMinimapTextureFromGameplayGrid` tints walkability when GPU capture is disabled.

### 10.3 Player dot on HUD

`PatchScene2DMinimapHud` draws the player marker in screen space using the **same** ortho bounds as GPU capture (`WorldXYToMinimapSquareUv`).

See [Runtime limits & composite views](08-scene2d-runtime-limits.md).

---

## 11. Game flow, saves, and HUD

| Piece | Role |
|-------|------|
| `GameStateComponent` | Intro / Playing / Paused / Victory |
| `GameFlowTriggerComponent` | Goal trigger → `Victory` |
| `GameFlowCoordinator` | Reload level, sync gem count, additive UI scenes |
| `GameFlowPersistentData` + `GameSave` | Cross-session progress (`spark_save_v1`) |

P0 demo: **P** pause, **R** reload level, **Escape** menu.

---

## 12. Component quick reference (2D gameplay)

| Domain | Components |
|--------|------------|
| Map | `TilemapMapSource`, `TilemapComponent`, `TilemapGameplayGrid`, `TilemapCollider2D`, `TilemapObjectLayer`, `TilemapObjectSpawn`, `TilemapTileAnimator`, `TilemapAutotile` |
| Rendering | `SpriteComponent`, `SpriteLighting2D`, `SpriteAnimator`, `Scene2DCompositeView`, `Camera2D`, `Camera2DRig`, `ParallaxLayer`, `ScreenShake` |
| Physics | `Rigidbody2D`, `BoxCollider2D`, `CircleCollider2D`, `TriggerVolume2D`, `CharacterController2D` |
| AI | `GridNavAgent2D`, `GridNavTarget2D`, `GridPathFollower2D`, `AiAgent` (3D steering plane / legacy) |
| Gameplay | `Pickup`, `Interactable`, `Health`, `GameState`, `GameFlowTrigger`, `PlayerInput`, `InputActionMap` |
| Input | `PlayerInputComponent` + `InputActionMapComponent` |

Full signatures and signal IDs: [Game component reference](../1-overview-architecture/07-game-component-reference.md).

---

## 13. Checklist for a new 2D grid level

1. Add `.sparkscene` + TMX reference; load with `SceneLevelLoader` / `SceneLoadSession`.
2. On level root: `TilemapGameplayGridComponent`, `TilemapCollider2DComponent`, object layer + spawn + registry.
3. Register spawn handlers; place markers or seed from layout.
4. Spawn player with sprite, `Rigidbody2D`, `GridNavAgent2D` pointing at level root.
5. Each frame: assign nav goals → `ProcessGridNavAgents2D` → apply motion → `Simulate2D`.
6. Optional: `Scene2DCompositeViewComponent` minimap, `GameplayDataTable`, pools for pickups.
7. Wire `GameState` / triggers / save on victory.

Next: [P0 product path](07-p0-2d-product-path.md) · [Components handbook](10-2d-components-handbook.md).

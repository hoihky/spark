# Tilemap level authoring

This chapter is an **end-to-end recipe** for turning tile data into a playable 2D level: import or paint tiles, bake walkability and collision, place spawns, register object handlers, and load through `.sparkscene` or runtime session.

**Prerequisites:** [Tilemaps](03-tilemaps.md) · [Scenes and saves](../1-overview-architecture/11-scenes-and-saves.md) · [Part 7 — Tilemap](../7-game-components-usage/03-tilemap.md).

**References:** SparkDemo **#19** (`TilemapShowcase2DDemo`), **#26 / F** (`GameFlow2DProductPathDemo`).

## Two authoring pipelines

| Pipeline | Best for | Entry |
|----------|----------|--------|
| **A — Tiled TMX + `.sparkscene`** | Designer-owned maps, versioned levels | `TilemapMapSourceComponent::ImportNow` |
| **B — Code / edit session** | Tools, generators, in-game editor | `TilemapEditSession` + `TilemapBrush` |

Both converge on the same **runtime component stack** on the level root.

## Level root checklist

Attach on one `GameObject` (name e.g. `Level`):

| Component | Purpose |
|-----------|---------|
| `TilemapComponent` | Layers, cells, tileset |
| `TilemapMapSourceComponent` | Optional TMX path + import |
| `TilemapGameplayGridComponent` | Walkability / A* / spawn validation |
| `TilemapCollider2DComponent` | Static physics from tile defs |
| `TilemapObjectLayerComponent` | Markers (gems, goals, spawns) |
| `TilemapObjectSpawnComponent` | `RespawnAll` after registry setup |
| `TilemapAutotileComponent` | Optional terrain autotile |
| `TilemapTileAnimatorComponent` | Optional animated tiles |

Decor-only maps may omit grid/collider; gameplay levels need all three derived systems.

---

## Step 1 — Tileset and per-tile definitions

Every gameplay tile needs a **`TileDefinition`**: collision shape, path flags, optional animation.

### C++

```cpp
SharedPtr<Tileset> tileset = LoadKenneyDungeonTileset(world);  // demo helper pattern
TileDefinition& wall = tileset->Definition(wallId);
wall.collisionShape = TileCollisionShape::FullCell;
wall.flags = TileDefinitionFlags::BlocksPathfinding;

TileDefinition& floor = tileset->Definition(floorId);
floor.collisionShape = TileCollisionShape::None;
floor.flags = TileDefinitionFlags::None;  // walkable
```

### C\#

```csharp
tileset.Definition(wallId).CollisionShape = TileCollisionShape.FullCell;
tileset.Definition(wallId).Flags = TileDefinitionFlags.BlocksPathfinding;
```

### Python

```python
# Tileset setup usually C++ or import-time; definitions come from TMX importer rules for Kenney packs.
```

**Rule of thumb:** terrain layer tiles that block movement set `FullCell` + `BlocksPathfinding`; cosmetics set `None` and `contributeGameplayGrid = false` on the layer.

---

## Step 2 — Import TMX (pipeline A)

### C++

```cpp
#include "spark/ecs/components/tilemap/TilemapMapSourceComponent.hpp"

GameObject* level = world.CreateGameObject("Level");
auto* source = level->AddComponent<TilemapMapSourceComponent>();
source->SetTmxPath("sprites/kenney_tiny-dungeon/Tiled/sampleMap.tmx");
source->SetPixelsPerWorldUnit(16.0F);
source->SetImportOnAttach(false);
source->SetHotReload(true);

if (!source->ImportNow(*level, world)) {
    // log source->GetLastErrorMessage() — fix path or PPU
}
```

`ImportNow` runs `TmxImporter` → `ApplyTilemapDocument`: creates `TilemapComponent`, loads atlases, applies Kenney gameplay definitions when recognized.

### C\#

```csharp
var source = level.AddTilemapMapSource();
source.SetTmxPath("sprites/kenney_tiny-dungeon/Tiled/sampleMap.tmx");
source.SetPixelsPerWorldUnit(16f);
source.ImportNow(level, world);
```

### Python

```python
source = level.get_or_add_default_component(ComponentKind.TilemapMapSource)
# import_now when bound
```

**Coordinates:** TMX rows map to Spark **bottom-left** cell origin — see [Tilemaps — coordinate system](03-tilemaps.md#coordinate-system).

---

## Step 3 — Layer roles (visual vs gameplay)

After import, verify each `TilemapLayer`:

### C++

```cpp
TilemapComponent* tm = level->GetComponent<TilemapComponent>();

TilemapLayer& terrain = tm->GetLayer(0);
terrain.contributeCollision = true;
terrain.contributeGameplayGrid = true;

TilemapLayer& decor = tm->GetLayer(2);
decor.contributeCollision = false;
decor.contributeGameplayGrid = false;
decor.orderInLayerOffset = 12;
decor.sortMode = TilemapLayerSortMode::WorldY;
```

### C\#

```csharp
terrain.ContributeCollision = true;
terrain.ContributeGameplayGrid = true;
decor.ContributeGameplayGrid = false;
```

Mis-flagged decor on the terrain layer is the most common cause of “blocked” spawn cells.

---

## Step 4 — Gameplay grid (pathfinding + spawn validation)

### C++

```cpp
#include "spark/ecs/components/tilemap/TilemapGameplayGridComponent.hpp"

auto* grid = level->AddComponent<TilemapGameplayGridComponent>();
grid->SetWalkRule(TilemapGameplayWalkRule::DefinitionAndFlags);
grid->SetAutoRebake(true);
grid->RequestRebake();
grid->RebakeIfNeeded(*level);

const TilemapGridFrame& frame = grid->GetGridFrame();
if (!frame.IsWalkableCell({spawnCellX, spawnCellY})) {
    // pick fallback cell — P0 demo does this when object layer missing
}
```

### C\#

```csharp
var grid = level.AddTilemapGameplayGrid();
grid.SetWalkRule(TilemapGameplayWalkRule.DefinitionAndFlags);
grid.SetAutoRebake(true);
grid.RebakeIfNeeded(level);
```

### Python

```python
grid = level.get_or_add_default_component(ComponentKind.TilemapGameplayGrid)
```

Use `GetGridFrame().WorldXYToCell` / `CellCenterToWorldXY` for click-to-move and NPC goals ([Pathfinding](../4-ai/05-pathfinding.md)).

---

## Step 5 — Tilemap collision (physics)

### C++

```cpp
#include "spark/ecs/components/physics/2d/TilemapCollider2DComponent.hpp"

auto* collider = level->AddComponent<TilemapCollider2DComponent>();
collider->Rebuild(*level);

// After editing tiles at runtime:
collider->Rebuild(*level);
physics.Simulate2D(world, timing);
```

`PhysicsWorld2D` also picks up baked statics each step; `Rebuild` is required immediately after import or edit session commits.

### C\#

```csharp
level.AddTilemapCollider2D().Rebuild(level);
```

### Python

```python
level.get_or_add_default_component(ComponentKind.TilemapCollider2D)
```

---

## Step 6 — Object markers and spawn registry

Markers are **not** painted tiles — they live in `TilemapObjectLayerComponent`.

### C++

```cpp
#include "spark/scene/tilemap/TilemapObjectSpawnRegistry.hpp"

void RegisterLevelSpawns() {
    TilemapObjectSpawnRegistry::Default().Register("p0_gem", &SpawnGemHandler);
    TilemapObjectSpawnRegistry::Default().Register("p0_goal", &SpawnGoalHandler);
}

// After level exists:
auto* spawn = level->AddComponent<TilemapObjectLayerComponent>();
const std::uint32_t li = spawn->AddObjectLayer("GameplayObjects");
TilemapObjectMarker m{};
m.typeId = Utf8String("p0_gem");
m.cellX = 14;
m.cellY = 8;
spawn->AddMarker(li, m);

level->AddComponent<TilemapObjectSpawnComponent>()->RespawnAll(*level, world);
```

Handlers receive marker data and create pooled entities (gems, enemies). P0 uses `GameObjectPool` + `prefabs/p0_gem.sparkscene`.

### C\#

```csharp
TilemapObjectSpawnRegistry.Default.Register("p0_gem", SpawnGemHandler);
```

### Python

```python
# Registry handlers are C++ function pointers today — register from native module.
```

If TMX has no object layer yet, **seed markers in code** (P0 bridge) until `.sparkmap` exports include objects.

---

## Step 7 — Player spawn and scene file

Author `levels/my_level.sparkscene` with:

- **Level** entity — components or `tilemap_map_source` pointing at TMX  
- **PlayerSpawn** — `spawn_point` named `Player`

### C++

```cpp
#include "spark/scene/editor/SceneLevelLoader.hpp"

SceneLoadSession session(sceneManager);
SceneLevelLoadResult r = SceneLevelLoader(session).Load(
    world,
    "levels/my_level.sparkscene",
    [](GameWorld& w, SceneInstanceId) { return w.FindGameObjectByName("Level"); },
    "PlayerSpawn");

if (r.success && r.primaryEntity) {
    levelRoot = r.primaryEntity;
    playerTr->SetTranslation(r.spawnPose.translation);
}
```

See [Scenes and saves](../1-overview-architecture/11-scenes-and-saves.md).

### C\#

```csharp
// SceneLevelLoader C++-first; templates host load in OnAttach.
```

---

## Step 8 — Runtime edit session (pipeline B)

For tools and procedural maps without TMX, mutate through **`TilemapEditSession`** (syncs to `TilemapComponent`, undo/redo).

### C++

```cpp
#include "spark/scene/tilemap/TilemapEditSession.hpp"
#include "spark/scene/tilemap/TilemapBrush.hpp"

TilemapEditSession session;
TilemapEditSessionOptions opts{};
opts.autoRebakePolicy = TilemapEditAutoRebakePolicy::AfterGestureOnly;
opts.validateAfterCommit = true;
session.Attach(*level, opts);
session.SetDerivedRebakeContext(&world, &physics);

TilemapBrush brush{};
brush.mode = TilemapBrush::Mode::Single;
brush.single = TileCell::FromTileId(floorId);
session.SetBrush(brush);

session.BeginGesture();
session.PaintRect(0, 0, mapW - 1, mapH - 1);
session.EndGesture();

TilemapDerivedDataRebaker::Options rebake{};
rebake.physicsQueryStatics = true;
session.RebakeDerivedDataForLastEdit(*level, rebake, &world, &physics);
```

`TilemapShowcase2DDemo` keeps a session for interactive paint; see also [Tilemap editor roadmap](04-tilemap-editor-roadmap.md) Phases A–C.

### C\#

```csharp
// Edit session C++-only; call from native tool or future bindings.
```

### Python

```python
pass
```

**Brush modes:** `Single`, `Stamp`, `RandomPalette`, `TerrainPaint` (`TilemapBrush::Mode`).

---

## Step 9 — Validate and export

Before checking in a level:

### C++

```cpp
#include "spark/scene/tilemap/TilemapEditValidator.hpp"
#include "spark/scene/tilemap/TilemapSparkMapExporter.hpp"

TilemapEditValidator validator{};
const auto report = validator.ValidateOwner(*level);
// fix orphan tile ids, layer size mismatches

TilemapSparkMapExporter exporter{};
exporter.Save(*level, "levels/my_level.sparkmap");
```

Run validator after TMX import and after edit-session gestures when `validateAfterCommit` is enabled.

---

## Step 10 — Wire into game loop

Typical playing state:

1. `SceneLevelLoader` or `ImportNow` completes.  
2. Register spawn handlers **once** at boot.  
3. `RespawnAll` for object layer.  
4. Each frame: input → `ProcessGridNavAgents2D` (if grid game) → `physics.Simulate2D`.  
5. Optional: `source->ImportNow` again when `SetHotReload(true)` and file changes.

Full frame recipe: [2D gameplay API](../8-2d-game/09-2d-gameplay-api-guide.md) · [P0 product path](../8-2d-game/07-p0-2d-product-path.md).

## Autotile and animated tiles (polish)

### C++

```cpp
auto* autotile = level->AddComponent<TilemapAutotileComponent>();
autotile->SetTerrainLayerIndex(0);
autotile->RequestRebuild();

auto* anim = level->AddComponent<TilemapTileAnimatorComponent>();
anim->SetAdvanceSceneTime(true);
```

Rebake autotile after terrain edits (`TilemapEditSession` incremental region API in Phase C).

---

## Top-down presentation

If the level is grid-based with many overlapping sprites, enable [Render layers and Y-sort](07-render-layers-and-sorting.md) on submit.

## Quick debug checklist

| Problem | Fix |
|---------|-----|
| Player falls through floor | `TilemapCollider2D::Rebuild`; terrain `contributeCollision` |
| Nav says unwalkable spawn | `GameplayGrid` rebake; tile `BlocksPathfinding` |
| Gems never appear | `Register` + `RespawnAll`; marker `typeId` spelling |
| TMX pink tiles | Atlas path / PPU; check import log |
| Edit paint invisible | `session.Attach` on correct owner; active layer index |

Next: [2D Animation](04-2d-animation.md) · [Gameplay API](../8-2d-game/09-2d-gameplay-api-guide.md).

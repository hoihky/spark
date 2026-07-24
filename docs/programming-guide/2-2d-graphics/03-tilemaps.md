# Tilemaps

Spark tilemaps are **multi-layer grids** backed by a shared **`Tileset`** (atlas + per-tile `TileDefinition` metadata). They render through the 2D tilemap pass (`VulkanTilemapPass`) with per-layer sort offsets, optional world-Y sort, flip/rotate flags, tint, and clip animation.

**Try it:** SparkDemo launcher item **19 — Tilemap layers, animation & pathfinding** (`TilemapShowcase2DDemo`).

## Coordinate system

| Space | Convention |
|-------|------------|
| World | +X right, +Y up (same as `Camera2D`) |
| Grid cell `(x, y)` | Origin at the **bottom-left** corner of the map; cell `(0, 0)` sits on the bottom row |
| TMX import | CSV rows are flipped on import so Tiled’s top row maps to high `y` in Spark storage |

Object markers from TMX use the same grid: Tiled top-down pixel `y` is converted to Spark `cellY` when parsing `objectgroup` layers.

## `TilemapComponent` and `Tileset`

```cpp
#include "spark/ecs/components/rendering/TilemapComponent.hpp"
#include "spark/scene/tilemap/Tileset.hpp"

SharedPtr<Tileset> tileset = CreateTilesetFromAtlas(atlas, atlasCols, atlasRows);
tileset->Definition(wallTileId).collisionShape = TileCollisionShape::FullCell;
tileset->Definition(wallTileId).flags = TileDefinitionFlags::BlocksPathfinding;
tileset->Definition(floorTileId).collisionShape = TileCollisionShape::None;

auto* mapGo = world.CreateGameObject();
auto* tilemap = mapGo->AddComponent<TilemapComponent>(
        tileset, mapW, mapH, 1.0F, /*sortOrderBase=*/0);
static_cast<void>(tilemap->AddLayer("Terrain"));
tilemap->SetPaintTile(0, x, y, grassPaintId);
tilemap->SetTileCell(1, x, y, TileCell::FromTileId(waterTileId));
```

| API | Role |
|-----|------|
| `SetPaintTile` / `GetPaintTileId` | Terrain paint id (autotile source) |
| `SetTileCell` / `GetTileCell` | Full `TileCell` (display id, flips, tint, paint id) |
| `GetLayer` | Per-layer flags: `contributeCollision`, `contributeGameplayGrid`, `sortMode` |
| `Tileset::Definition(id)` | Collision shape, path flags, autotile group, animation clip index |

Kenney *Tiny Dungeon* packed atlases use **bottom-row tile indexing**; TMX import calls `TiledLocalTileIndexToSparkAtlasIndex` so atlas UVs match the packed sheet.

## Stacked layers

```cpp
TilemapLayer& props = tilemap->GetLayer(2);
props.orderInLayerOffset = 16;
props.contributeGameplayGrid = false;  // decor only
props.sortMode = TilemapLayerSortMode::WorldY;
```

Pair with:

- **`TilemapTileAnimatorComponent`** — advances clip time for animated tiles on the map.
- **`TilemapAutotileComponent`** — rebuilds display tiles from painted terrain groups on a chosen layer.
- **`TilemapCollider2DComponent`** — static colliders from `TileDefinition` shapes on layers with `contributeCollision`.

## Gameplay grid and pathfinding

Bake walkability once (or auto-rebake each frame) from tile layers:

```cpp
#include "spark/ecs/components/tilemap/TilemapGameplayGridComponent.hpp"

auto* grid = mapGo->AddComponent<TilemapGameplayGridComponent>();
grid->SetWalkRule(TilemapGameplayWalkRule::DefinitionAndFlags);
grid->SetAutoRebake(true);

// After editing tiles:
grid->RequestRebake();
grid->RebakeIfNeeded(*mapGo);

const TilemapGridFrame& frame = grid->GetGridFrame();
Vector2 world = frame.CellCenterToWorldXY({cellX, cellY});
```

| `TilemapGameplayWalkRule` | Behavior |
|---------------------------|----------|
| `OccupiedWalkable` | Any non-empty gameplay-layer cell is walkable |
| `DefinitionAndFlags` | Respects `BlocksPathfinding` / `ForceWalkable`, then tile collision |
| `CollisionAligned` | Blocked when the tile would contribute physics collision |

See [Pathfinding](../4-ai/05-pathfinding.md) for `GridPathfinder::FindPath4` and mouse picking.

## TMX / `.sparkmap` import

```cpp
#include "spark/ecs/components/tilemap/TilemapMapSourceComponent.hpp"

auto* source = mapGo->AddComponent<TilemapMapSourceComponent>();
source->SetImportOnAttach(false);
source->SetTmxPath("sprites/kenney_tiny-dungeon/Tiled/sampleMap.tmx");
source->SetPixelsPerWorldUnit(16.0F);  // tileWorldSize = tilePixelWidth / PPU
source->SetHotReload(true);
source->ImportNow(*mapGo, world);
```

`ApplyTilemapDocument` (`spark/scene/tilemap/TilemapDocumentApply.hpp`):

- Loads the primary tileset texture via `GameWorld::LoadTexture` and `ResolveTilemapAssetPath`.
- For Kenney packed dungeon atlases, swaps to `tilemap_packed.png`, clears margin/spacing, applies gameplay tile definitions, and disables gameplay-grid contribution on **Objects** / **Carts** layers.
- Leaves `tileWorldSize` at **0** in the TMX document so apply derives size from `pixelsPerWorldUnit`.

Importer: `TmxImporter` (CSV layers, external `.tsx` tilesets, GID flip bits).

## Object layers (markers, spawns, gizmos)

```cpp
auto* objects = mapGo->AddComponent<TilemapObjectLayerComponent>();
const std::uint32_t li = objects->AddObjectLayer("GameplayObjects");

TilemapObjectMarker chest{};
chest.typeId = Utf8String("chest");
chest.cellX = 10;
chest.cellY = 6;
static_cast<void>(objects->AddMarker(li, chest));

mapGo->AddComponent<TilemapObjectSpawnComponent>();  // uses TilemapObjectSpawnRegistry
mapGo->AddComponent<TilemapObjectGizmoComponent>();  // editor-style markers
```

`TilemapObjectMarkerWorldPosition` maps cell + normalized offset through `TilemapGridFrame`.

## Screen → cell picking (2D)

Use the same NDC convention as the rest of Spark demos (`TerrainScreenToWorldRay` in `spark/demo/ShellDemoSceneUtil.hpp`): framebuffer Y grows **downward** (do not apply an OpenGL-style `1 - y` flip on top of `OrthographicVulkan`).

```cpp
Matrix4 invVp{};
camera.ViewProjection(fbW, fbH).TryInvert(invVp);
Vector3 ro{}, rd{};
TerrainScreenToWorldRay(fbW, fbH, mx, my, invVp, ro, rd);
const float t = -ro.z / rd.z;
const Vector2 world{ro.x + rd.x * t, ro.y + rd.y * t};
GridPathfinder::Cell cell = grid->GetGridFrame().WorldXYToCell(world);
```

Reference: `TilemapShowcase2DDemo`, `Connect3Demo`.

## Collision pairing

`TilemapCollider2DComponent` bakes **per-tile** shapes from definitions (full cell, half cell, custom convex). Visual-only decor should use `TileCollisionShape::None` on the tileset.

For manual platforms (no tilemap), use scaled sprites + `BoxCollider2DComponent` as in the platformer sample.

Next: [2D Animation](04-2d-animation.md).

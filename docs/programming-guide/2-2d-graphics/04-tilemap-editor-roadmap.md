# Tilemap editor roadmap

Spark tilemaps already support rendering, TMX/`.sparkmap` import (`TilemapMapSourceComponent`), gameplay grid, collision bake, autotile, animation, and object markers. This document is the plan for **editor-ready APIs** without coupling to ImGui or the scene editor shell.

**Related:** [Tilemaps](03-tilemaps.md), `TilemapDocument`, `ApplyTilemapDocument`.

## Design principles

| Principle | Application |
|-----------|-------------|
| **Document as serialization truth** | `TilemapDocument` is the portable format; ECS components are the runtime view. |
| **Edit session** | Mutations go through `TilemapEditSession` commands + revision bumps, not ad-hoc component pokes. |
| **Derived data sinks** | Gameplay grid, collider bake, and autotile subscribe to `TilemapEditRevision` regions (`TilemapDerivedDataRebaker`, optional auto-rebake on session). |
| **Coordinates in one place** | `TilemapGridFrame` + pick helpers; editors never re-derive floor math. |

## Phases

### Phase A — Capture, pick, revision (implemented)

- **`CaptureTilemapDocument`** — `TilemapComponent` (+ optional object layers) → `TilemapDocument`.
- **`TilemapPick`** — world position → cell / top-most tile / object marker.
- **`TilemapEditRevision` + `TilemapRevisionTracker`** — monotonic revision id and dirty cell regions for incremental rebake.

### Phase B — Edit session & brushes (implemented)

- `TilemapEditSession` with undoable commands (paint, erase, fill, line, rect, resize map).
- Brush types: single tile, stamp, random palette, terrain paint id (`TilemapBrush`).
- `BeginGesture` / `EndGesture` coalesce drag strokes; revision tracker bumps after each committed command.

### Phase C — Incremental derived data (implemented)

- `TilemapCellRegion::FromRevision` — dirty rect + neighbor margin for autotile.
- `BakeTilemapGameplayGridRegion`, `RebuildTilemapAutotileLayerRegion`.
- `RebakeTilemapDerivedData` / `TilemapEditSession::RebakeDerivedDataForLastEdit` (optional `PhysicsSubsystem` query static rebuild).
- **`TilemapEditValidator`** — orphan tile/paint ids, missing atlas paths, layer cell count vs map size, object markers off-map. Run after TMX import (`TilemapMapSourceComponent`), before `.sparkmap` save (`TilemapSparkMapExporter`), and optionally after `TilemapEditSession::EndGesture` (`validateAfterCommit`).

### Phase D — Editor UX & pipeline

- Palette / tile metadata batch edit, object move with stable marker ids.
- TMX export parity, optional chunked maps, debug overlay geometry API.

## Phase A API reference

```cpp
#include "spark/scene/tilemap/TilemapServices.hpp"

// Runtime → document (save, diff, editor model)
const TilemapDocumentCapturer capturer{};
const auto captured = capturer.CaptureFromOwner(*mapGo);

// Mouse / tool hit testing
const TilemapPicker picker{};
const TilemapCellPick cellPick = picker.PickTopmostTile(*mapGo, *tilemap, {worldX, worldY});

// After editor mutations (Phase B will call this automatically)
TilemapRevisionTracker revisions;
const TilemapEditRevision rev = revisions.RecordCellChange(layerIndex, cellX, cellY);
gameplayGrid->RebakeRegion(rev);  // Phase C
```

## Phase C API reference

```cpp
#include "spark/scene/tilemap/TilemapServices.hpp"

TilemapDerivedDataRebaker::Options opts{};
opts.neighborMargin = 1U;
opts.physicsQueryStatics = true; // immediate broad-phase queries after edit

const auto rebaked = session.RebakeDerivedDataForLastEdit(*mapGo, opts, &world, &physics);
// rebaked.rebakedRegion — cells touched (margin included)
```

`PhysicsWorld2D` already rebakes tilemap collision each simulation step; `physicsQueryStatics` is for overlap/raycast query caches.

```cpp
TilemapEditSessionOptions opts{};
opts.autoRebakePolicy = TilemapEditAutoRebakePolicy::AfterGestureOnly;
opts.validateAfterCommit = true;
session.Attach(*mapGo, opts);
session.SetDerivedRebakeContext(&world, &physics);

TilemapSparkMapExporter exporter{};
const auto saved = exporter.Save(*mapGo, "levels/my_level.sparkmap");
```

## Phase B API reference

```cpp
#include "spark/scene/tilemap/TilemapEditSession.hpp"
#include "spark/scene/tilemap/TilemapBrush.hpp"

TilemapEditSession session;
session.Attach(*mapGo);

TilemapBrush brush{};
brush.mode = TilemapBrushMode::Single;
brush.single = TileCell::FromTileId(wallTileId);
session.SetBrush(brush);

session.BeginGesture();
PickTopmostTileCell(...); // Phase A
session.PaintCell(cell.x, cell.y);
session.EndGesture();

if (session.CanUndo()) {
    session.Undo();
}
const TilemapEditRevision& rev = session.GetLastRevision();
```

| Tool | API |
|------|-----|
| Paint / erase | `PaintCell`, `EraseCell` |
| Line / rect | `PaintLine`, `PaintRect` (outline or filled) |
| Fill | `FloodFill` (matches full `TileCell` at seed) |
| Map size | `ResizeMap` (preserves overlapping cells, undoable) |

## File layout

| Path | Role |
|------|------|
| `TilemapServices.hpp` | Facade include for editor services |
| `TilemapDocumentCapturer` | ECS → document |
| `TilemapPicker` / `TilemapObjectQuery` | Hit testing & object queries |
| `TilemapEditRevision` / `TilemapRevisionTracker` | Dirty regions |
| `TilemapBrush` | Brush resolution (`ResolveCell`) |
| `TilemapEditSession` | Undo + tools (composes capturer + rebaker) |
| `TilemapDocumentApplier` | Document → ECS |
| `TilemapGameplayGridBaker` / `TilemapAutotileBaker` | Derived data |
| `TilemapDerivedDataRebaker` | Revision-driven partial rebake |
| `TilemapDocumentSerializer` / `TilemapSparkMapExporter` | `.sparkmap` I/O + validate-before-save |
| `TilemapEditValidator` | Document consistency checks |
| `TmxImporter` (+ nested `Gid`) | Tiled `.tmx` → `TilemapDocument` |
| `TileAnimationResolver` | Animated tile id at submit time |
| `TilemapObjectSpawnRegistry` | Object `typeId` → spawn handler (`Default()`) |

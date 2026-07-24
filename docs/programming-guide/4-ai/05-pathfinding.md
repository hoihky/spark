# Pathfinding

## Class Design: `GridPathfinder`

```cpp
#include "spark/ai/path/GridPathfinder.hpp"

class IGridWalkability {
public:
    virtual bool IsWalkable(int cellX, int cellY) const = 0;
};

class GridBitmapWalkability final : public IGridWalkability { /* ... */ };

struct Cell { int x = 0; int y = 0; };

static bool FindPath4(const IGridWalkability& grid, const Cell& start,
                      const Cell& goal, Array<Cell>& outCells);
```

4-connected A* on a grid abstraction. **Start and goal must both be walkable** or `FindPath4` returns false.

## Tilemap gameplay grid (recommended)

Prefer baking from `TilemapComponent` instead of hand-maintaining a bitmap:

```cpp
#include "spark/ecs/components/tilemap/TilemapGameplayGridComponent.hpp"
#include "spark/scene/tilemap/TilemapGridCoordinates.hpp"

auto* gridComp = mapGo->AddComponent<TilemapGameplayGridComponent>();
gridComp->SetWalkRule(TilemapGameplayWalkRule::DefinitionAndFlags);
gridComp->SetAutoRebake(true);
gridComp->RebakeIfNeeded(*mapGo);

const TilemapGameplayGrid& walk = gridComp->GetGrid();
const TilemapGridFrame& frame = gridComp->GetGridFrame();

GridPathfinder::Cell start = frame.WorldXYToCell(playerWorldXY);
GridPathfinder::Cell goal{targetX, targetY};
Array<GridPathfinder::Cell> cells;
if (GridPathfinder::FindPath4(gridComp->GetWalkability(), start, goal, cells)) {
    for (std::size_t i = 0; i < cells.GetSize(); ++i) {
        Vector2 wp = frame.CellCenterToWorldXY(cells[i]);
        // move agent toward wp
    }
}
```

Walkability honors per-layer `contributeGameplayGrid` and per-tile `TileDefinition` flags (see [Tilemaps](../2-2d-graphics/03-tilemaps.md)).

One-shot bake without the component:

```cpp
TilemapGameplayGrid grid;
tilemap->BakeGameplayGrid(grid, TilemapGameplayWalkRule::DefinitionAndFlags);
```

## Manual `GridBitmapWalkability`

```cpp
GridBitmapWalkability walk;
walk.Resize(mapW, mapH);
for (std::int32_t y = 0; y < mapH; ++y) {
    for (std::int32_t x = 0; x < mapW; ++x) {
        walk.SetBlocked(x, y, /* blocked if wall */);
    }
}
```

## World polyline helpers

```cpp
Array<Vector2> poly;
TilemapGameplayGrid::CellsToWorldPolylineXY(
    cells, gridOriginXY, cellSize, poly);

// Legacy XZ naming (same math, Y stored in Vector2::y):
GridPathfinder::CellsToWorldPolyline(cells, gridOriginXZ, cellSize, outWorldXZ);
```

## NavMesh agents (3D / hybrid)

Or use **NavMeshAgentComponent** + **PatrolPathComponent** — `ProcessNavMeshAgents` fills the polyline before steering runs (see [Game Component Reference](../1-overview-architecture/07-game-component-reference.md#patrolpathcomponent--navmeshagentcomponent)).

## Follow polyline

Advance `pathIndex` when within `arriveRadius` of each waypoint; steer toward `poly[pathIndex]`.

## Fuzzy logic (optional)

`FuzzyAdvisoryModule` (`spark/ai/fuzzy/FuzzyLogic.hpp`) blends continuous inputs (health, distance) into action weights — enable via `agent->SetFuzzyEnabled(true)`.

Part 4 complete → **Part 5**: [Physics Overview](../5-physics/01-physics-overview.md).

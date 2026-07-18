---
title: Pathfinding
order: 5
---

# Pathfinding

## Class Design: `GridPathfinder`

```cpp
#include "spark/ai/path/GridPathfinder.hpp"

class IGridWalkability {
public:
    virtual bool IsWalkable(int cellX, int cellZ) const = 0;
};

class GridBitmapWalkability final : public IGridWalkability { /* ... */ };

struct Cell { int x = 0; int z = 0; };

static bool FindPath4(const IGridWalkability& grid, const Cell& start,
                      const Cell& goal, Array<Cell>& outCells);
static void CellsToWorldPolyline(const Array<Cell>& cells, const Vector2& gridOriginXZ,
                                 float cellSize, Array<Vector2>& outWorldXZ);
```

4-connected A* on a grid abstraction.

## Build Walkability from Tilemap

```cpp
GridBitmapWalkability walk;
walk.Resize(mapW, mapH);
for (uint32_t y = 0; y < mapH; ++y)
  for (uint32_t x = 0; x < mapW; ++x)
    walk.SetWalkable(x, y, map->GetTile(x, y) != TilemapComponent::kEmptyTile);
```

## Store Path on Agent

Manual grid path:

```cpp
Array<GridPathfinder::Cell> cells;
GridPathfinder::Cell start{playerCellX, playerCellZ};
GridPathfinder::Cell goal{targetCellX, targetCellZ};
if (GridPathfinder::FindPath4(walk, start, goal, cells)) {
    auto& poly = agent->GetPathWorldPolylineXZ();
    GridPathfinder::CellsToWorldPolyline(cells, Vector2{-20, -5}, 1.0F, poly);
    agent->SetPathIndex(0);
}
```

Or use **NavMeshAgentComponent** + **PatrolPathComponent** (see [Game Component Reference](../1-overview-architecture/07-game-component-reference.html#patrolpathcomponent--navmeshagentcomponent)) — `ProcessNavMeshAgents` fills the polyline before steering runs.

## Follow Polyline

Advance `pathIndex` when within `arriveRadius` of each waypoint; steer toward `poly[pathIndex]`.

## Fuzzy Logic (Optional)

`FuzzyAdvisoryModule` (`spark/ai/fuzzy/FuzzyLogic.hpp`) blends continuous inputs (health, distance) into action weights — enable via `agent->SetFuzzyEnabled(true)`.

Part 4 complete → **Part 5**: [Physics Overview](5-physics/01-physics-overview.html).

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

4-connected A* on a grid abstraction. **Start and goal must both be walkable** (or resolved to the nearest walkable cell by your caller) or `FindPath4` returns false.

---

## Tilemap gameplay grid (data source)

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

---

## ECS: 2D grid navigation components (recommended)

For gameplay code, use the dedicated 2D nav stack instead of manual `FindPath4` loops:

| Component | Purpose |
|-----------|---------|
| `TilemapGameplayGridComponent` | Walkability source + `TilemapGridFrame` (on tilemap object) |
| `GridNavAgent2DComponent` | Replans A* path each tick / interval |
| `GridPathFollower2DComponent` | Moves owner along agent waypoints |
| `GridNavTarget2DComponent` | Marks a goal entity for `TargetObject` mode |

### Click-to-move setup

```cpp
#include "spark/ecs/components/ai/GridNavAgent2DComponent.hpp"
#include "spark/ecs/components/ai/GridPathFollower2DComponent.hpp"
#include "spark/ai/NavigationSubsystem.hpp"

boardGo->AddComponent<TilemapGameplayGridComponent>()->SetAutoRebake(true);

playerGo->AddComponent<GridNavAgent2DComponent>();
auto* nav = playerGo->GetComponent<GridNavAgent2DComponent>();
nav->SetGridSourceObject(boardGo);
nav->SetGoalMode(GridNavGoalMode2D::WorldPosition);
nav->SetSyncToAiAgent(false);

playerGo->AddComponent<GridPathFollower2DComponent>()->SetMaxSpeed(6.5F);

// On mouse pick (world XY from screen):
nav->SetGoalWorldPosition(pickedWorldXY);
nav->RequestRepath();

// Each frame:
ProcessGridNavAgents2D(world, timing.deltaTimeSeconds);
world.UpdateGameObjects(timing, context);  // GridPathFollower2D OnUpdate @ priority 125
```

### Chase target on grid

```cpp
playerGo->AddComponent<GridNavTarget2DComponent>();
enemyGo->AddComponent<AiAgentComponent>()->SetSteeringPlane(AiSteeringPlane::XyRigidbody2D);

auto* chase = enemyGo->AddComponent<GridNavAgent2DComponent>();
chase->SetGridSourceObject(boardGo);
chase->SetGoalMode(GridNavGoalMode2D::TargetObject);
chase->SetGoalTarget(playerGo);
chase->SetRepathIntervalSeconds(0.2F);
chase->SetRepathEveryFrame(false);

SimulateGameAi(world, timing, context);  // includes ProcessGridNavAgents2D + AiAgent steering
```

`GridNavAgent2DComponent` snaps start/goal to the nearest walkable cell when blocked. `GridNavTarget2DComponent::SetSnapToWalkableCell` controls goal snapping.

---

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

---

## NavMesh agents (3D / XZ patrol)

**NavMeshAgentComponent** + **PatrolPathComponent** — `ProcessNavMeshAgents` fills the polyline before 3D steering runs. The built-in `UseGridPathfinding()` mode uses a flat `GridBitmapWalkability` on the **XZ** plane (blackboard slots 0/1 for goal). For **tilemap XY** games, use **GridNavAgent2DComponent** instead.

See [Game Component Reference](../1-overview-architecture/07-game-component-reference.md#gridnavagent2dcomponent--gridpathfollower2dcomponent--gridnavtarget2dcomponent).

---

## Follow polyline manually

Advance `pathIndex` when within `arriveRadius` of each waypoint; steer toward `poly[pathIndex]`. `GridPathFollower2DComponent` encapsulates this for transform and rigidbody modes.

## Fuzzy logic (optional)

`FuzzyAdvisoryModule` (`spark/ai/fuzzy/FuzzyLogic.hpp`) blends continuous inputs (health, distance) into action weights — enable via `agent->SetFuzzyEnabled(true)`.

Part 4 complete → **Part 5**: [Physics Overview](../5-physics/01-physics-overview.md).

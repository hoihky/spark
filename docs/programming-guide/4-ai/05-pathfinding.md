# Pathfinding

Spark provides **grid pathfinding** for 2D tilemaps and **nav mesh** agents for 3D. Both feed movement targets into `AiAgentComponent` or `GridPathFollower2DComponent`.

**Deep dive:** [Tilemap gameplay API](../8-2d-game/09-2d-gameplay-api-guide.md), [Tilemap components](../7-game-components-usage/03-tilemap.md).

## 2D grid path (`GridPathfinder`)

Bake walkability with `TilemapGameplayGridComponent`, then query A* on the grid frame.

### C++

```cpp
#include "spark/scene/tilemap/GridPathfinder.hpp"

const TilemapGridFrame& frame = grid->GetGridFrame();
GridPathfinder::Cell start = frame.WorldXYToCell(playerPos);
GridPathfinder::Cell goal = frame.WorldXYToCell(clickWorld);

Array<GridPathfinder::Cell> path;
if (GridPathfinder::FindPath4(frame, start, goal, path)) {
    agent2d->SetPathFromCells(frame, path);
}
```

### C\#

```csharp
var frame = grid.GetGridFrame();
var path = new List<GridCell>();
if (GridPathfinder.FindPath4(frame, start, goal, path))
    follower.SetPathFromCells(frame, path);
```

### Python

```python
# grid_nav_agent_2d + pathfinder when interop exposes GridPathfinder helpers.
```

## `GridNavAgent2DComponent` + `GridPathFollower2D`

**Follower priority 125** — after grid agent planning, before animators.

### C++

```cpp
player->AddComponent<GridNavAgent2DComponent>();
auto* follower = player->AddComponent<GridPathFollower2DComponent>();
follower->SetDestinationCell({12, 8});
```

Each frame: `ProcessGridNavAgents2D` inside `SimulateGameAi`.

### C\#

```csharp
player.AddGridNavAgent2D();
player.GetOrAddComponent<GridPathFollower2DComponent>()?.SetDestinationCell(cell);
```

### Python

```python
player.get_or_add_default_component(ComponentKind.GridNavAgent2D)
player.get_or_add_default_component(ComponentKind.GridPathFollower2D)
```

## 3D nav mesh

### C++

```cpp
NavMeshAgentComponent* nav = guard->AddComponent<NavMeshAgentComponent>();
nav->SetDestination({10.0F, 0.0F, 5.0F});
// ProcessNavMeshAgents in SimulateGameAi fills AiAgent path polyline
```

### C\#

```csharp
guard.AddNavMeshAgent().SetDestination(dest);
```

### Python

```python
guard.get_or_add_default_component(ComponentKind.NavMeshAgent)
```

## Screen pick → grid cell

Use `TerrainScreenToWorldRay` + `WorldXYToCell` — same convention as [Tilemaps](../2-2d-graphics/03-tilemaps.md#screen--cell-picking-2d).

### C++

```cpp
GridPathfinder::Cell cell = grid->GetGridFrame().WorldXYToCell(world.xy);
```

### C\#

```csharp
var cell = grid.GetGridFrame().WorldXYToCell(world);
```

### Python

```python
cell = grid.get_grid_frame().world_xy_to_cell(world_x, world_y)
```

**Demo:** `TilemapShowcase2DDemo` (#19), `GameFlow2DProductPathDemo` (F).

Next: [Perception, nav, and blackboard](06-perception-nav-and-blackboard.md).

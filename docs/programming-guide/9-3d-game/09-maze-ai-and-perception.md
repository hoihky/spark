# Maze AI and perception

`Maze3DDemo` (**#8**) combines **procedural maze geometry**, a **patrol guard** with nav mesh pursuit, **perception** (line-of-sight style sensing), and **collectible gems** — a compact alternative to the FPS arena for 3D gameplay learning.

**Part 4:** [AI overview](../4-ai/01-ai-overview.md) · [Perception & nav](../4-ai/06-perception-nav-and-blackboard.md) · [Pathfinding](../4-ai/05-pathfinding.md) · **Part 7:** [AI / nav](../7-game-components-usage/10-ai-audio-world.md).

## Systems map

| Layer | Components |
|-------|------------|
| World | Static mesh maze, `FlyCamera` or future CC3D |
| Guard | `AiAgentComponent`, `NavMeshAgentComponent`, `PerceptionSensorComponent` |
| Player | Transform + move (demo-specific) |
| Pickups | triggers or overlap queries on gems |

## Concept: enable game AI pass

### C++

```cpp
#include "spark/ai/GameAiSubsystem.hpp"

void OnUpdate(const FrameTiming& timing, IEngineContext& ctx) override {
    TickPlayer(timing, ctx);
    Game::OnUpdate(timing, ctx);
    physics.SimulateAll3D(GetWorld(), timing);
    SimulateGameAi(GetWorld(), timing, ctx);
}
```

Order matters: physics resolves positions **before** steering/nav reads transforms.

### C\#

```csharp
SimulateGameAi(World, timing, context);
```

### Python

```python
# SimulateGameAi C++-hosted in maze demo template.
```

## Concept: nav mesh agent on guard

### C++

```cpp
guard->AddComponent<NavMeshAgentComponent>();
guard->AddComponent<AiAgentComponent>();
agent->SetMaxSpeed(3.5F);

// When player spotted (blackboard slot set by perception):
nav->SetDestination(playerTr->GetWorldMatrix().GetTranslation());
```

### C\#

```csharp
guard.AddNavMeshAgent();
guard.AddAiAgent().SetMaxSpeed(3.5f);
```

### Python

```python
from spark._generated.component_kind import ComponentKind
guard.get_or_add_default_component(ComponentKind.NavMeshAgent)
guard.get_or_add_default_component(ComponentKind.AiAgent)
```

Bake or load nav data as the demo does before expecting `SetDestination` to succeed.

## Concept: perception sensor

### C++

```cpp
#include "spark/ecs/components/ai/PerceptionSensorComponent.hpp"

auto* sense = guard->AddComponent<PerceptionSensorComponent>();
sense->SetSightRange(12.0F);
sense->SetFieldOfViewDegrees(110.0F);
// Blackboard slots updated each SimulateGameAi tick when target visible
```

Wire FSM or GOAP transitions on blackboard floats/flags ([FSM](../4-ai/03-fsm.md)).

### C\#

```csharp
guard.AddPerceptionSensor().SetSightRange(12f).SetFieldOfViewDegrees(110f);
```

### Python

```python
guard.get_or_add_default_component(ComponentKind.PerceptionSensor)
```

## Concept: patrol before chase

For guards that patrol until alerted:

1. `PatrolPathComponent` or manual waypoints on nav mesh.  
2. Perception promotes player to **target** blackboard slot.  
3. `NavMeshAgent` destination tracks player until lost.

Steering-only showcase: demo **#14** (`SteeringShowcase3DDemo`) — no nav mesh required.

## Fly camera vs character mode

Maze demo uses **fly camera** for traversal teaching; swap to [Character camera](07-character-and-third-person.md) for floor-constrained gameplay on the same layout.

## Gems and win condition

Reuse `PickupComponent` / trigger patterns from [2D gameplay kit](../1-overview-architecture/09-gameplay-kit.md) or simple overlap counters in C++ until bound in scripts.

---

**Part 9 wrap-up:** You now have FPS (**01–06**), character (**07**), physics integration (**08**), and AI (**09**) paths. Return to [3D overview](00-3d-game-systems-map.md) or [SparkDemo catalog](../1-overview-architecture/10-sparkdemo-catalog.md) for the next demo.

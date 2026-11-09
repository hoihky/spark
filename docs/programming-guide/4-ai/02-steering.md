# Steering Behaviors

Steering computes **desired acceleration** from position/velocity and blackboard targets. `AiAgentComponent::SubsystemTick` applies the composed acceleration to the steering plane (XZ world or XY rigidbody).

**Headers:** `spark/ai/steering/SteeringBehaviors.hpp` (2D XZ as Vector2), `SteeringBehaviors3D.hpp`.

## Concept: weighted behaviors

### C++

```cpp
#include "spark/ai/steering/SteeringBehaviors.hpp"

SteeringSeek seek{};
SteeringSeparation separation{};
SteeringComposer composer;
composer.Add(&seek, 1.0F);
composer.Add(&separation, 0.8F);

AiBlackboard& bb = agent->GetBlackboard();
bb.SetFloat(0, targetPos.x);
bb.SetFloat(1, targetPos.z);

Vector2 accel = composer.Compose(posXZ, velXZ, bb);
```

### C\#

```csharp
var bb = agent.GetBlackboard();
bb.SetFloat(0, targetPos.X);
bb.SetFloat(1, targetPos.Z);
// Compose via game-specific steering helper or native tick
```

### Python

```python
# Steering composition runs inside AiAgentComponent native tick when enabled.
agent.get_or_add_default_component(ComponentKind.AiAgent)
```

## Built-in 2D (XZ) behaviors

| Class | Effect |
|-------|--------|
| `SteeringSeek` | Toward blackboard target |
| `SteeringFlee` | Away from threat |
| `SteeringArrive` | Slow near target |
| `SteeringWander` | Random walk |
| `SteeringPursuit` / `Evade` | Moving target |
| `SteeringSeparation` | Crowd spacing |

## 3D behaviors

`SteeringSeek3D`, `SteeringFlee3D`, `SteeringObstacleAvoidance3D`, `SteeringFlocking3D` — see `SteeringShowcase3DDemo`.

### C++

```cpp
#include "spark/ai/steering/SteeringBehaviors3D.hpp"

SteeringObstacleAvoidance3D avoid{};
SteeringComposer3D composer3d;
composer3d.Add(&avoid, 1.2F);
```

### C\#

```csharp
// Same pattern in C++-ported demo games under spark-sharp templates.
```

### Python

```python
pass  # enable AiAgent + native steering in 3D demos
```

## Concept: blackboard slots

Slots are **game-defined indices** — document your layout in one header per enemy type.

### C++

```cpp
namespace BbSlots {
    constexpr int TargetX = 0;
    constexpr int TargetZ = 1;
    constexpr int ThreatX = 2;
}
bb.SetFloat(BbSlots::TargetX, playerX);
```

### C\#

```csharp
const int TargetX = 0;
bb.SetFloat(TargetX, playerX);
```

### Python

```python
# Set blackboard via generated ai_agent_* methods when bound.
```

## ECS patrol without nav mesh

Manual waypoint advance in FSM or demo code; nav mesh path sets polyline on `AiAgent` automatically — [Pathfinding](05-pathfinding.md).

Next: [Finite State Machines](03-fsm.md).

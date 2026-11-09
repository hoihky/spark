# Finite State Machines

FSM logic lives in `FsmStateMachine` (`spark/ai/fsm/`) attached to `AiAgentComponent` when `SetFsmEnabled(true)`.

**Also used for animation (non-AI):** `Sprite2DCharacterAnimFsmComponent`, `Character3DAnimFsmComponent` — separate from AI FSM but same priority rules ([Animation guide](../7-game-components-usage/08-animation.md)).

## Concept: attach FSM to agent

### C++

```cpp
#include "spark/ai/fsm/FsmStateMachine.hpp"

auto fsm = MakeUnique<FsmStateMachine>();
fsm->AddState("Idle", /* onEnter, onTick, onExit */);
fsm->AddState("Chase", /* ... */);
fsm->SetInitialState("Idle");

auto* agent = enemy->AddComponent<AiAgentComponent>();
agent->SetFsmEnabled(true);
agent->SetFsm(MoveTemp(fsm));
```

Tick occurs inside `SimulateGameAi` → `AiAgentComponent::SubsystemTick`.

### C\#

```csharp
var agent = enemy.AddAiAgent();
agent.SetFsmEnabled(true);
// FSM graph often built in C++ demo; C# games mirror states in managed layer or native module.
```

### Python

```python
enemy.get_or_add_default_component(ComponentKind.AiAgent)
```

## Concept: transition on perception

Wire `PerceptionSensorComponent` results into blackboard; FSM reads slots each tick.

### C++

```cpp
void OnTick(AiBlackboard& bb, float dt) {
    if (bb.GetFloat(SlotSeenPlayer) > 0.5F)
        RequestTransition("Chase");
}
```

### C\#

```csharp
if (bb.GetFloat(seenSlot) > 0.5f) { /* transition */ }
```

### Python

```python
# FSM tick is native; script games use simpler GridNav or direct velocity.
```

## Platformer enemy FSM

`Platformer2DDemo` uses lightweight chase/idle on `AiAgentComponent` with steering plane `XyRigidbody2D` — inspect `platformer2d/` spawn helpers.

## Debugging

- Disable GOAP when prototyping: `agent->SetGoapEnabled(false)`
- Log state name each transition
- SparkDemo **Steering** showcase visualizes velocity vectors

Next: [GOAP](04-goap.md).

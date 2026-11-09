# GOAP

Goal-Oriented Action Planning (`spark/ai/goap/`) plans sequences of actions to satisfy world/goal bitmasks on `AiAgentComponent`.

Enable with `SetGoapEnabled(true)` and register actions with preconditions/effects.

## Concept: world and goal bits

### C++

```cpp
AiAgentComponent* agent = npc->AddComponent<AiAgentComponent>();
agent->SetGoapEnabled(true);

std::uint32_t worldBits = 0;
worldBits |= (1u << 0);  // has_key
agent->SetGoapWorldBits(worldBits);

std::uint32_t goalMask = 0;
goalMask |= (1u << 2);  // door_open
agent->SetGoapGoalMask(goalMask);
agent->SetGoapGoalValue(goalMask);
```

### C\#

```csharp
var agent = npc.GetComponent<AiAgentComponent>();
agent?.SetGoapEnabled(true);
agent?.SetGoapWorldBits(worldBits);
agent?.SetGoapGoalMask(goalMask);
```

### Python

```python
agent = npc.get_or_add_default_component(ComponentKind.AiAgent)
# agent.set_goap_enabled(True) via generated binding
```

## Concept: register actions

### C++

```cpp
GoapAction openDoor{};
openDoor.name = Utf8String("OpenDoor");
openDoor.cost = 2.0F;
openDoor.preconditions = /* bits required */;
openDoor.effects = /* bits set */;
agent->GetGoapActions().PushBack(openDoor);
```

Planning runs in `SubsystemTick` when enabled and FSM allows planner control.

### C\#

```csharp
// Actions defined in native gameplay module or ported from C++ samples.
```

### Python

```python
# GOAP surface exposed via catalog methods on AiAgentComponent when generated.
```

## When to use GOAP vs FSM

| Approach | Best for |
|----------|----------|
| FSM | Combat phases, simple patrol/chase |
| GOAP | RPG item use, multi-step quests |
| Grid nav only | Tilemap ARPG (`GridNavAgent2D`) |

Demos with GOAP bits: inspect AI test scenes in `tests/ai/` and agent setup in advanced 3D demos.

Next: [Pathfinding](05-pathfinding.md).

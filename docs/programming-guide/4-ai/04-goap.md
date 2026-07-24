# Goal-Oriented Action Planning

## Class Design: `GoapActionSpec`

Symbolic actions with bitmask preconditions and effects:

```cpp
struct GoapActionSpec {
    std::uint64_t preMask, preValue;       // required world bits
    std::uint64_t effectSetMask, effectClearMask;
    float cost = 1.0F;
    std::uint32_t nameId = 0;
};
```

World state is a `uint64_t` bitfield — semantics are **game-defined**.

## Planner API

```cpp
#include "spark/ai/goap/GoapPlanner.hpp"

std::uint64_t world = agent->GetGoapWorldBits();
std::uint64_t goalMask = (1ull << 3);  // bit 3 must be set
std::uint64_t goalValue = (1ull << 3);

Array<std::uint32_t> plan;
bool ok = GoapPlanner::Plan(world, goalMask, goalValue,
                            agent->GetGoapActions(), plan);
```

## Register Actions

```cpp
agent->SetGoapEnabled(true);

GoapActionSpec pickup{};
pickup.preMask  = (1ull << 0);  // must be "near item"
pickup.preValue = (1ull << 0);
pickup.effectSetMask = (1ull << 1);  // has item
pickup.cost = 2.0F;
agent->GetGoapActions().PushBack(pickup);

GoapActionSpec deliver{};
deliver.preMask = (1ull << 1);
deliver.preValue = (1ull << 1);
deliver.effectSetMask = (1ull << 3);  // quest complete
deliver.cost = 1.0F;
agent->GetGoapActions().PushBack(deliver);

agent->SetGoapGoal((1ull << 3), (1ull << 3));
```

GOAP plans **what** to do; combine with pathfinding/steering for **how** to move.

See `GoapDemo` in `SparkDemo`.

Next: [Pathfinding](05-pathfinding.md).

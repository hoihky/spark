# Gameplay kit (`spark/gameplay/`)

Spark ships a small **public gameplay kit** so demos and games share the same conventions for **teams**, **damage**, **interaction**, and **inventory hooks**. Components still own data (`HealthComponent`, `PickupComponent`, …); the kit provides **routing** and **policy** in plain C++ (no separate scripting layer required).

**Umbrella include:** `#include "spark/gameplay/GameplayKit.hpp"`

**Tests:** `tests/gameplay/GameplayKitTest.cpp` (teams, damage events, interaction, pickup delivery).

---

## Teams

| Type | Role |
|------|------|
| `GameplayTeamId` | Stable `int32` team id (`0` = neutral / unassigned). |
| `GameplayTeamComponent` | Team id on a `GameObject` (walk parents via `GameplayTeamAffiliation::Resolve`). |
| `GameplayTeamPolicy` | Friendly-fire and hostility rules (`AllowsHostileAction`). |
| `GameplayTeamRelation` | `Self`, `Ally`, `Enemy`, `Neutral` classification. |

`Combat2DTarget` and `Combat3DTarget` apply damage through `GameplayDamageDeal` with a default team policy (friendly fire **off**). Override with `SetTeamPolicy()` when your game needs custom rules.

```cpp
#include "spark/ecs/components/gameplay/GameplayTeamComponent.hpp"
#include "spark/gameplay/Combat3D.hpp"

player->AddComponent<GameplayTeamComponent>()->SetTeamId(1);
enemy->AddComponent<GameplayTeamComponent>()->SetTeamId(2);

Combat3DTarget target{*enemy};
const float applied = target.ApplyDamage(player, 25.0F);  // 0 if same team
```

**Demo:** `CharacterCameraDemo` — player team `1`, melee dummies team `2`.

---

## Damage pipeline

| Type | Role |
|------|------|
| `GameplayDamageApplier` | Walks hierarchy for `DamageableComponent` / `HealthComponent`; optional hurtbox notifier. |
| `GameplayDamageDeal` | Instigator + optional `GameplayTeamPolicy` + raw damage → applied amount. |
| `GameplayDamageEvent` | Read-only view of `SignalId::DamageApplied` (`DamageSignalPayload`). |
| `Combat2DTarget` / `Combat3DTarget` | Combat entry points (hurtbox invulnerability, then `GameplayDamageDeal`). |

Components still emit **`SignalId::DamageApplied`** and **`SignalId::Died`** as before (`spark/gameplay/DamageTypes.hpp`).

```cpp
#include "spark/gameplay/GameplayDamageEvent.hpp"

void OnSignal(GameObject& owner, SignalId id, const SignalPayload& payload) override {
    GameplayDamageEvent evt{};
    if (GameplayDamageEvent::TryFromSignal(id, payload, evt)) {
        // evt.GetAppliedAmount(), GetInstigator(), GetTarget()
    }
}
```

**Not in the kit (game-specific):** damage types, armor curves, crit tables, hitstop, status effects — build on top of `GameplayDamageDeal` or component multipliers.

---

## Interaction

| Type | Role |
|------|------|
| `GameplayInteractionQuery` | Nearest `InteractableComponent` that `CanInteract(instigator)` (3D distance on transforms). |
| `GameplayInteractionProcessor` | `ProcessInteractPress(world, instigator)` → `TryInteract` on best candidate. |
| `ProcessInteractables2D` | Thin wrapper used by 2D demos (`Interaction2D.hpp`). |

Pair `InteractableComponent` with `TriggerVolume2DComponent` for overlap-based range, or rely on `SetInteractionRadius`.

---

## Inventory hooks

| Type | Role |
|------|------|
| `IGameplayInventorySink` | Game backend (`TryReceive(GameplayItemStack)`). |
| `GameplayInventoryCollectorComponent` | Default collector on player; emits **`SignalId::ItemCollected`** (`GameplayItemCollectedPayload`). |
| `GameplayItemStack` | `itemId` + `quantity`. |
| `GameplayPickupDelivery` | Finds collector on collector hierarchy. |

`PickupComponent::TryCollect` calls your `SetOnCollected` callback when set; otherwise it delivers through `GameplayPickupDelivery`.

```cpp
#include "spark/ecs/components/gameplay/GameplayInventoryCollectorComponent.hpp"
#include "spark/ecs/components/gameplay/PickupComponent.hpp"

player->AddComponent<GameplayInventoryCollectorComponent>();
player->GetComponent<GameplayInventoryCollectorComponent>()->SetOnItemReceived(
        [](GameObject& owner, const GameplayItemStack& stack) {
            // push into bag UI, stats, quests…
        });

// Pickup without custom callback auto-delivers to collector on hierarchy.
gem->AddComponent<PickupComponent>()->SetItemId("gem");
```

---

## Related combat headers

The kit re-exports common combat helpers:

- **2D:** `Combat2D.hpp`, `Interaction2D.hpp`, hurtbox queries, `DamageZone2DComponent` callers.
- **3D:** `Combat3D.hpp`, `Hurtbox3DComponent`, `AnimationMeleeHitComponent` → `Combat3DTarget`.

See also [Game component reference — Gameplay](07-game-component-reference.md#gameplay) and [2D gameplay API guide](../7-2d-game/09-2d-gameplay-api-guide.md).

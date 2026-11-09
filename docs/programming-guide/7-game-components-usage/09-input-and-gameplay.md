# Component Guide: Input and Gameplay

Semantic input, game flow, combat vitals, teams, and pickups. These components emit **`SignalId`** events consumed by siblings on the same `GameObject`.

**Related:** [Gameplay kit](../1-overview-architecture/09-gameplay-kit.md), [2D gameplay API](../8-2d-game/09-2d-gameplay-api-guide.md).

| Component | Role |
|-----------|------|
| `InputActionMapComponent` | Action definitions |
| `PlayerInputComponent` | Device → actions |
| `GameStateComponent` | High-level mode |
| `GameFlowTriggerComponent` | Triggers state changes |
| `HealthComponent` | HP pool |
| `DamageableComponent` | Damage filter |
| `DamageZone2DComponent` | Environmental hurt |
| `InteractableComponent` | Use / talk |
| `PickupComponent` | Collectible |
| `GameplayTeamComponent` | Friendly fire |
| `GameplayInventoryCollectorComponent` | Bag receiver |

---

## InputActionMapComponent

#### C++

```cpp
InputActionMapComponent* map = player->AddComponent<InputActionMapComponent>();
map->AddAction("Jump", {GLFW_KEY_SPACE});
map->AddAction("Move", InputActionType::Axis2D);
```

#### C\#

```csharp
player.GetOrAddComponent<InputActionMapComponent>();
```

#### Python

```python
player.get_or_add_default_component(ComponentKind.InputActionMap)
```

---

## PlayerInputComponent

**Priority 50** — emits `SignalId::InputActionTriggered`.

#### C++

```cpp
player->AddComponent<PlayerInputComponent>()->SetActionMap(map);
```

#### C\#

```csharp
player.GetOrAddComponent<PlayerInputComponent>();
```

#### Python

```python
player.get_or_add_default_component(ComponentKind.PlayerInput)
```

#### Listening on another component

```cpp
if (id == SignalId::InputActionTriggered && phase == InputActionPhase::Pressed) {
    const char* name = static_cast<const char*>(payload.ptr);
}
```

---

## GameStateComponent

#### C++

```cpp
manager->AddComponent<GameStateComponent>()->SetState(GameFlowState::Playing);
```

#### C\#

```csharp
manager.GetOrAddComponent<GameStateComponent>()?.SetState(GameFlowState.Playing);
```

#### Python

```python
manager.get_or_add_default_component(ComponentKind.GameState)
```

---

## GameFlowTriggerComponent

Attach on **trigger object**, not player.

#### C++

```cpp
goal->AddComponent<GameFlowTriggerComponent>()->SetTargetState(GameFlowState::Victory);
```

#### C\#

```csharp
goal.GetOrAddComponent<GameFlowTriggerComponent>();
```

#### Python

```python
goal.get_or_add_default_component(ComponentKind.GameFlowTrigger)
```

---

## HealthComponent + DamageableComponent

#### C++

```cpp
enemy->AddComponent<HealthComponent>()->SetMaxHealth(50.0F);
enemy->AddComponent<DamageableComponent>();
```

#### C\#

```csharp
enemy.AddHealth().SetMaxHealth(50f);
enemy.GetOrAddComponent<DamageableComponent>();
```

#### Python

```python
enemy.get_or_add_default_component(ComponentKind.Health)
```

---

## DamageZone2DComponent

#### C++

```cpp
spikes->AddComponent<DamageZone2DComponent>()->SetDamagePerSecond(15.0F);
```

#### C\#

```csharp
spikes.GetOrAddComponent<DamageZone2DComponent>();
```

#### Python

```python
spikes.get_or_add_default_component(ComponentKind.DamageZone2D)
```

---

## InteractableComponent

#### C++

```cpp
npc->AddComponent<InteractableComponent>()->SetInteractionRadius(2.0F);
```

#### C\#

```csharp
npc.GetOrAddComponent<InteractableComponent>()?.SetInteractionRadius(2f);
```

#### Python

```python
npc.get_or_add_default_component(ComponentKind.Interactable)
```

---

## PickupComponent

#### C++

```cpp
gem->AddComponent<PickupComponent>()->SetItemId("gem");
player->AddComponent<GameplayInventoryCollectorComponent>();
```

#### C\#

```csharp
gem.GetOrAddComponent<PickupComponent>()?.SetItemId("gem");
```

#### Python

```python
gem.get_or_add_default_component(ComponentKind.Pickup)
```

---

## GameplayTeamComponent

#### C++

```cpp
player->AddComponent<GameplayTeamComponent>()->SetTeamId(1);
```

#### C\#

```csharp
player.AddGameplayTeam().SetTeamId(1);
```

#### Python

```python
player.get_or_add_default_component(ComponentKind.GameplayTeam)
```

Next: [AI, audio, and world components](10-ai-audio-world.md).

# Rigidbody 2D

`Rigidbody2DComponent` pairs with a **2D collider** (`BoxCollider2D`, `CircleCollider2D`, or `PolygonCollider2D`). Simulation writes back to `TransformComponent`.

**Platformers:** prefer `CharacterController2DComponent` on top of dynamic body — [Player controller](../8-2d-game/04-player-controller.md).

## Body types

| `RigidbodyBodyType2D` | Behavior |
|-----------------------|----------|
| `Static` | Immovable (platforms) |
| `Dynamic` | Forces + gravity |
| `Kinematic` | Transform-driven, pushes dynamics |

## Concept: dynamic crate

### C++

```cpp
#include "spark/ecs/components/physics/2d/Rigidbody2DComponent.hpp"
#include "spark/ecs/components/physics/2d/BoxCollider2DComponent.hpp"

GameObject* crate = world.CreateGameObject();
crate->AddComponent<TransformComponent>()->SetTranslation({2, 3, 0});
crate->AddComponent<Rigidbody2DComponent>();
crate->AddComponent<BoxCollider2DComponent>(Vector2{0.5F, 0.5F});
```

### C\#

```csharp
crate.GetOrAddTransform().SetTranslation(new Vector3(2, 3, 0));
crate.AddRigidbody2D(RigidbodyBodyType2D.Dynamic);
crate.AddBoxCollider2D(new Vector2(0.5f, 0.5f));
```

### Python

```python
crate = world.create_game_object("Crate")
crate.get_or_add_transform().translation = Vector3(2, 3, 0)
crate.add_rigidbody_2d(native.RigidbodyBodyType2D.Dynamic)
crate.add_box_collider_2d(Vector2(0.5, 0.5), Vector2(0, 0))
```

## Concept: apply impulse

### C++

```cpp
Rigidbody2DComponent* rb = crate->GetComponent<Rigidbody2DComponent>();
rb->SetVelocity({4.0F, 8.0F});
```

### C\#

```csharp
rb.SetVelocity(new Vector2(4f, 8f));
```

### Python

```python
rb = crate.get_component_rigidbody_2d()  # when wrapper exists
rb.set_velocity(Vector2(4.0, 8.0))
```

## Concept: character controller stack

### C++

```cpp
auto* motor = player->AddComponent<CharacterController2DComponent>();
motor->SetMaxRunSpeed(7.5F);
motor->SetJumpImpulse(12.0F);
player->AddComponent<Rigidbody2DComponent>();
player->AddComponent<BoxCollider2DComponent>(Vector2{0.35F, 0.55F});

void OnUpdate(...) {
    motor->SetMoveInputX(input->GetActionAxis1D("MoveX"));
    if (input->WasActionPressedThisFrame("Jump")) motor->RequestJump();
    physics.Simulate2D(world, timing);
}
```

### C\#

```csharp
player.GetOrAddComponent<CharacterController2DComponent>()?.SetMaxRunSpeed(7.5f);
```

### Python

```python
player.get_or_add_default_component(ComponentKind.CharacterController2D)
```

## One-way platforms

Add `OneWayPlatform2DComponent` on platform; motor drops through with down+ jump — platformer demo.

Next: [Colliders](03-colliders.md).

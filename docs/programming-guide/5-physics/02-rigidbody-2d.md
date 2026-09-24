# Rigidbody 2D

## Class Design: `Rigidbody2DComponent`

```cpp
enum class RigidbodyBodyType2D { Kinematic, Static, Dynamic };

Rigidbody2DComponent(RigidbodyBodyType2D bodyType = Dynamic, float gravityScaleIn = 1.0F);

Vector2& GetVelocity();
void SetVelocity(const Vector2& v);
bool IsGrounded() const noexcept;
float GetGravityScale() const noexcept;
```

| Body type | Behavior |
|-----------|----------|
| `Static` | Immovable collider (platforms) |
| `Dynamic` | Simulated velocity + gravity |
| `Kinematic` | Moved by transform, pushes dynamics |

## Player Controller (Platformer)

Prefer `CharacterController2DComponent` over direct velocity writes. It adds coyote time, jump buffer, ground snap, slope limits, and one-way platform drop-through on top of `Rigidbody2DComponent`.

```cpp
#include "spark/physics/PhysicsSubsystem.hpp"
#include "spark/ecs/components/physics/2d/CharacterController2DComponent.hpp"
#include "spark/ecs/components/input/PlayerInputComponent.hpp"

PhysicsSubsystem physics;

void OnAttach(IEngineContext& context) override {
    physics.GetWorld2D().GetSettings().gravityY = -30.0F;
    physics.GetWorld2D().GetSettings().maxFallSpeed = 42.0F;
}

void OnUpdate(const FrameTiming& timing) {
    playerController->SetMoveInputX(playerInput->GetActionAxis1D("MoveX"));
    if (playerInput->WasActionPressedThisFrame("Jump")) {
        playerController->RequestJump();
    }
    physics.Simulate2D(GetWorld(), timing);
    PickupComponent::ProcessDeferredDestroys(GetWorld());
}
```

### One-way platforms

Mark static platforms with `OneWayPlatform2DComponent`. The character controller ignores them from below unless landing from above:

```cpp
platform->AddComponent<BoxCollider2DComponent>(Vector2{4.0F, 0.15F});
platform->AddComponent<Rigidbody2DComponent>(RigidbodyBodyType2D::Static, 0.0F);
platform->AddComponent<OneWayPlatform2DComponent>();
```

Press **S** / **Down** (via `SetDropThroughOneWay(true)`) to fall through for one frame.

### Legacy: direct velocity control

You can still drive `Rigidbody2DComponent` velocity manually for prototypes, but you lose coyote time and one-way semantics:

```cpp
Vector2 v = playerRb->GetVelocity();
v.x = run * kRunSpeed;
if (playerRb->IsGrounded() && jumpPressed) v.y = kJumpSpeed;
playerRb->SetVelocity(v);
```

See `Platformer2DDemo` for the full component-based sample.

## Static Platform

```cpp
go->AddComponent<BoxCollider2DComponent>();
go->AddComponent<Rigidbody2DComponent>(RigidbodyBodyType2D::Static, 0.0F);
```

Transform scale defines collider world size when using default `BoxCollider2DComponent` half-extents.

Next: [Colliders](03-colliders.md).

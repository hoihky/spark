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

Configure physics once, then step each frame:

```cpp
#include "spark/physics/PhysicsSubsystem.hpp"

PhysicsSubsystem physics;

void OnAttach(IEngineContext& context) override {
    physics.GetWorld2D().GetSettings().gravityY = -30.0F;
    physics.GetWorld2D().GetSettings().maxFallSpeed = 42.0F;
}

void OnUpdate(const FrameTiming& timing) {
    Vector2 v = playerRb->GetVelocity();
    float run = 0.0F;
    if (in.IsKeyDown(GLFW_KEY_A)) run -= 1.0F;
    if (in.IsKeyDown(GLFW_KEY_D)) run += 1.0F;
    v.x = run * kRunSpeed;

    if (playerRb->IsGrounded() && in.IsKeyPressedThisFrame(GLFW_KEY_SPACE))
        v.y = kJumpSpeed;

    playerRb->SetVelocity(v);
    physics.Simulate2D(GetWorld(), timing);
}
```

See `Platformer2DDemo` for a full sample using `PhysicsSubsystem`.

## Static Platform

```cpp
go->AddComponent<BoxCollider2DComponent>();
go->AddComponent<Rigidbody2DComponent>(RigidbodyBodyType2D::Static, 0.0F);
```

Transform scale defines collider world size when using default `BoxCollider2DComponent` half-extents.

Next: [Colliders](03-colliders.md).

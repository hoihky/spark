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

```cpp
Vector2 v = playerRb->GetVelocity();
float run = 0.0F;
if (in.IsKeyDown(GLFW_KEY_A)) run -= 1.0F;
if (in.IsKeyDown(GLFW_KEY_D)) run += 1.0F;
v.x = run * kRunSpeed;  // kRunSpeed = 9.0F in sample

if (playerRb->IsGrounded() && in.IsKeyPressedThisFrame(GLFW_KEY_SPACE))
    v.y = kJumpSpeed;   // kJumpSpeed = 11.5F

playerRb->SetVelocity(v);

PhysicsWorld2DSettings phys{};
phys.gravityY = -30.0F;
phys.maxFallSpeed = 42.0F;
SimulatePhysics2D(GetWorld(), timing, phys);
```

## Static Platform

```cpp
go->AddComponent<BoxCollider2DComponent>();
go->AddComponent<Rigidbody2DComponent>(RigidbodyBodyType2D::Static, 0.0F);
```

Transform scale defines collider world size when using default `BoxCollider2DComponent` half-extents.

Next: [Colliders](03-colliders.md).

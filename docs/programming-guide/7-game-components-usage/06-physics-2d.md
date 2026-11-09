# Component Guide: Physics 2D

2D physics runs through **`PhysicsSubsystem::Simulate2D`** after component updates. Colliders sync transforms from `Rigidbody2D` or static bodies.

**Related:** [Rigidbody 2D](../5-physics/02-rigidbody-2d.md), [Colliders](../5-physics/03-colliders.md).

| Component | Role |
|-----------|------|
| `Rigidbody2DComponent` | Dynamic / kinematic body |
| `BoxCollider2DComponent` | AABB collider |
| `CircleCollider2DComponent` | Circle collider |
| `PolygonCollider2DComponent` | Convex polygon |
| `PhysicsMaterial2DComponent` | Friction / restitution |
| `CharacterController2DComponent` | Platformer motor |
| `OneWayPlatform2DComponent` | One-way platforms |
| `TriggerVolume2DComponent` | Trigger messages |
| `Hurtbox2DComponent` | Combat receive |
| `Projectile2DComponent` | Straight-line mover |
| `DistanceJoint2DComponent` | Rope / link |
| `HingeJoint2DComponent` | Hinged prop |

---

## Rigidbody2DComponent

#### C++

```cpp
Rigidbody2DComponent* rb = player->AddComponent<Rigidbody2DComponent>();
rb->SetBodyType(RigidbodyBodyType2D::Dynamic);
rb->SetGravityScale(1.0F);
rb->SetLinearVelocity({2.0F, 0.0F});
```

#### C\#

```csharp
var rb = player.AddRigidbody2D(RigidbodyBodyType2D.Dynamic, gravityScale: 1f);
rb.SetLinearVelocity(new Vector2(2f, 0f));
```

#### Python

```python
from spark import native
rb = player.add_rigidbody_2d(native.RigidbodyBodyType2D.Dynamic, gravity_scale=1.0)
```

---

## BoxCollider2DComponent

#### C++

```cpp
player->AddComponent<BoxCollider2DComponent>(Vector2{0.4F, 0.6F}, Vector2{0.0F, 0.0F});
```

#### C\#

```csharp
player.AddBoxCollider2D(new Vector2(0.4f, 0.6f));
```

#### Python

```python
player.add_box_collider_2d(Vector2(0.4, 0.6), Vector2(0, 0))
```

---

## CircleCollider2DComponent

#### C++

```cpp
coin->AddComponent<CircleCollider2DComponent>(0.25F);
```

#### C\#

```csharp
coin.AddCircleCollider2D(0.25f);
```

#### Python

```python
coin.get_or_add_default_component(ComponentKind.CircleCollider2D)
```

---

## CharacterController2DComponent

**When to use:** Platformer player instead of raw dynamic rigidbody (`Platformer2DDemo`).

#### C++

```cpp
auto* cc = player->AddComponent<CharacterController2DComponent>();
cc->SetMaxRunSpeed(7.5F);
cc->SetJumpImpulse(12.0F);
cc->SetGravityScale(2.2F);
```

#### C\#

```csharp
player.GetOrAddComponent<CharacterController2DComponent>()?.SetMaxRunSpeed(7.5f);
```

#### Python

```python
player.get_or_add_default_component(ComponentKind.CharacterController2D)
```

---

## OneWayPlatform2DComponent

#### C++

```cpp
plat->AddComponent<OneWayPlatform2DComponent>()->SetSurfaceNormal({0.0F, 1.0F});
```

#### C\#

```csharp
plat.GetOrAddComponent<OneWayPlatform2DComponent>();
```

#### Python

```python
plat.get_or_add_default_component(ComponentKind.OneWayPlatform2D)
```

---

## TriggerVolume2DComponent

Emits `SignalId::Physics2DTriggerEnter` to **sibling** components on the same object.

#### C++

```cpp
goal->AddComponent<BoxCollider2DComponent>();
goal->AddComponent<TriggerVolume2DComponent>();
goal->AddComponent<GameFlowTriggerComponent>();
```

#### C\#

```csharp
goal.AddBoxCollider2D(half);
goal.GetOrAddComponent<TriggerVolume2DComponent>();
```

#### Python

```python
goal.get_or_add_default_component(ComponentKind.TriggerVolume2D)
```

---

## Projectile2DComponent

**Priority 220**

#### C++

```cpp
bullet->AddComponent<Projectile2DComponent>()->SetSpeed(18.0F);
```

#### C\#

```csharp
bullet.GetOrAddComponent<Projectile2DComponent>()?.SetSpeed(18f);
```

#### Python

```python
bullet.get_or_add_default_component(ComponentKind.Projectile2D)
```

---

## Hurtbox2DComponent

#### C++

```cpp
enemy->AddComponent<Hurtbox2DComponent>()->SetInvulnerable(false);
```

#### C\#

```csharp
enemy.GetOrAddComponent<Hurtbox2DComponent>();
```

#### Python

```python
enemy.get_or_add_default_component(ComponentKind.Hurtbox2D)
```

---

## PhysicsMaterial2DComponent

#### C++

```cpp
go->AddComponent<PhysicsMaterial2DComponent>()->SetFriction(0.2F);
```

#### C\#

```csharp
go.GetOrAddComponent<PhysicsMaterial2DComponent>()?.SetFriction(0.2f);
```

#### Python

```python
go.get_or_add_default_component(ComponentKind.PhysicsMaterial2D)
```

---

## Joints

#### C++

```cpp
bridge->AddComponent<DistanceJoint2DComponent>()->SetConnectedBody(otherRb);
```

#### C\#

```csharp
bridge.GetOrAddComponent<DistanceJoint2DComponent>();
```

#### Python

```python
bridge.get_or_add_default_component(ComponentKind.DistanceJoint2D)
```

## Simulation call (required)

#### C++

```cpp
void OnUpdate(const FrameTiming& t, IEngineContext& ctx) override {
    Game::OnUpdate(t, ctx);
    physics.Simulate2D(GetWorld(), t);
}
```

#### C\#

```csharp
_physics.Simulate2D(World, timing);
```

#### Python

```python
context.physics.simulate_2d(world, timing)
```

Next: [Physics 3D components](07-physics-3d.md).

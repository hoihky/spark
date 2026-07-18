---
title: Colliders
order: 3
---

# Colliders

## 2D Colliders

**BoxCollider2DComponent:**

```cpp
BoxCollider2DComponent(Vector2 localHalfExtents = {0.5F, 0.5F},
                       Vector2 localOffset = Vector2::Zero);
```

**CircleCollider2DComponent:**

```cpp
CircleCollider2DComponent(float localRadius = 0.5F,
                          Vector2 localOffset = Vector2::Zero);
```

If both exist on a dynamic body, **circle wins** for simulation.

**PolygonCollider2DComponent** (convex, static only, max 16 vertices):

```cpp
Array<Vector2> verts;
verts.PushBack({-2.0F, 0.0F});
verts.PushBack({2.0F, 0.0F});
verts.PushBack({0.0F, 1.0F});
auto* poly = ramp->AddComponent<PolygonCollider2DComponent>();
poly->SetVertices(verts);
```

**TilemapCollider2DComponent** — one static box per non-empty tile on sibling `TilemapComponent`:

```cpp
level->AddComponent<TilemapComponent>(...);
level->AddComponent<TilemapCollider2DComponent>();
```

**PhysicsMaterial2DComponent** — friction/restitution on static 2D surfaces (baked into broad-phase, applied at contact resolve):

```cpp
platform->AddComponent<PhysicsMaterial2DComponent>(0.5F, 0.2F);
```

**2D joints** — `DistanceJoint2DComponent`, `HingeJoint2DComponent` (require `PhysicsWorld2DSettings::jointIterations > 0`).

## Layer Masks

```cpp
auto* box = go->AddComponent<BoxCollider2DComponent>();
box->SetCategoryBits(1u << 0);   // layer 0 = player
box->SetMaskBits(0xFFFF);        // collides with all by default
box->SetIsTrigger(true);         // overlap only, no resolution
```

Collision filter (`CollisionFilter2D::ShouldCollide`):

```
collide = (maskA & categoryB) && (maskB & categoryA)
```

## 3D Colliders

| Component | Shape | Dynamic? |
|-----------|-------|----------|
| `BoxCollider3DComponent` | Axis-aligned box | Static broad-phase |
| `SphereCollider3DComponent` | Sphere | Yes (preferred dynamic shape) |
| `CapsuleCollider3DComponent` | Capsule along X/Y/Z | Static or dynamic |
| `MeshCollider3DComponent` | Mesh AABB (static) | Static broad-phase from `MeshComponent` |
| `CharacterController3DComponent` | Kinematic sphere motor | Not a collider — use instead of dynamic RB |
| `TriggerVolume3DComponent` | Box / sphere / capsule | Overlap only |

```cpp
// Dynamic ball
go->AddComponent<Rigidbody3DComponent>();
go->AddComponent<SphereCollider3DComponent>(0.5F);

// Static wall
wall->AddComponent<BoxCollider3DComponent>(Vector3{1,2,0.2F});

// FPS character (no Rigidbody3D on same object)
player->AddComponent<CharacterController3DComponent>(0.4F, Vector3{0,0.9F,0});
cc->SetMoveInput({mx, 0, mz});
SimulateCharacterControllers3D(world, dt);

// Trigger zone
zone->AddComponent<TriggerVolume3DComponent>(TriggerVolume3DShape::Box, Vector3{3,2,3});
```

Optional: `PhysicsMaterial3DComponent` (friction/restitution), `DistanceJoint3DComponent`, `HingeJoint3DComponent`, `SpringJoint3DComponent` (set `jointIterations` in `PhysicsWorld3DSettings`).

## Trigger Signals

**2D:**

```cpp
void OnSignal(GameObject& owner, SignalId id, const SignalPayload& payload) override {
    if (id == SignalId::Physics2DTriggerOverlap) {
        GameObject* other = static_cast<GameObject*>(payload.ptr);
        // pickup, damage zone, goal flag
    }
}
```

**3D:**

```cpp
if (id == SignalId::Physics3DTriggerEnter) {
    GameObject* other = static_cast<GameObject*>(payload.ptr);
}
if (id == SignalId::Physics3DTriggerExit) {
    GameObject* other = static_cast<GameObject*>(payload.ptr);
}
```

Or use `TriggerVolume3DComponent::SetOnEnter` / `SetOnExit` callbacks.

See also: [Game Component Reference](../1-overview-architecture/07-game-component-reference.html).

Next: [Physics Queries](5-physics/04-queries.html).

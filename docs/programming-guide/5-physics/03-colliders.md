# Colliders

Colliders define **shape** for static and dynamic contact. Bake runs inside `PhysicsSubsystem::Simulate*`.

## 2D collider types

| Component | Shape |
|-----------|--------|
| `BoxCollider2DComponent` | AABB half extents + offset |
| `CircleCollider2DComponent` | Radius + offset |
| `PolygonCollider2DComponent` | Convex polygon vertices |

## Concept: static ground

### C++

```cpp
GameObject* ground = world.CreateGameObject();
auto* tr = ground->AddComponent<TransformComponent>();
tr->SetTranslation({0, -1.5F, 0});
tr->SetScale({20.0F, 1.0F, 1.0F});
ground->AddComponent<BoxCollider2DComponent>();  // uses unit box scaled by transform
```

### C\#

```csharp
ground.AddBoxCollider2D(new Vector2(10f, 0.5f));
```

### Python

```python
ground.add_box_collider_2d(Vector2(10.0, 0.5), Vector2(0, 0))
```

## Concept: tilemap collision

`TilemapCollider2DComponent` bakes from `TilemapComponent` + tile definitions — [Tilemap guide](../7-game-components-usage/03-tilemap.md).

### C++

```cpp
mapGo->AddComponent<TilemapCollider2DComponent>()->Rebuild(*mapGo);
```

### C\#

```csharp
mapGo.GetComponent<TilemapCollider2DComponent>()?.Rebuild(mapGo);
```

### Python

```python
map_go.get_or_add_default_component(ComponentKind.TilemapCollider2D)
```

## 3D colliders (summary)

`BoxCollider3D`, `SphereCollider3D`, `CapsuleCollider3D`, `MeshCollider3D` — see [Physics 3D](05-physics-3d.md).

### C++

```cpp
player->AddComponent<CapsuleCollider3DComponent>(0.4F, 1.8F);
```

### C\#

```csharp
player.AddCapsuleCollider3D(0.4f, 1.8f);
```

### Python

```python
player.get_or_add_default_component(ComponentKind.CapsuleCollider3D)
```

## Physics materials

### C++

```cpp
go->AddComponent<PhysicsMaterial2DComponent>()->SetFriction(0.2F);
go->AddComponent<PhysicsMaterial3DComponent>()->SetRestitution(0.3F);
```

### C\#

```csharp
mat.SetFriction(0.2f);
```

### Python

```python
go.get_or_add_default_component(ComponentKind.PhysicsMaterial2D)
```

Next: [Physics Queries](04-queries.md).

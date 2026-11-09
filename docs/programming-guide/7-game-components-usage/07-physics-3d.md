# Component Guide: Physics 3D

3D simulation via **`PhysicsSubsystem::SimulateAll3D`** — rigidbodies, character controllers, triggers, joint constraints.

**Related:** [Physics 3D](../5-physics/05-physics-3d.md), Physics showcase demo **#29**.

| Component | Role |
|-----------|------|
| `Rigidbody3DComponent` | Dynamic body |
| `BoxCollider3DComponent` | Box collider |
| `SphereCollider3DComponent` | Sphere |
| `CapsuleCollider3DComponent` | Character capsule |
| `MeshCollider3DComponent` | Mesh collider |
| `PhysicsMaterial3DComponent` | Friction / bounce |
| `CharacterController3DComponent` | FPS / third-person motor |
| `TriggerVolume3DComponent` | 3D triggers |
| `CollisionComponent` | Generic collision metadata |
| `DistanceJoint3DComponent` | 3D rope |
| `HingeJoint3DComponent` | Door hinge |
| `SpringJoint3DComponent` | Spring link |

---

## Rigidbody3DComponent

#### C++

```cpp
crate->AddComponent<Rigidbody3DComponent>()->SetMass(2.5F);
crate->AddComponent<BoxCollider3DComponent>(Vector3{0.5F, 0.5F, 0.5F});
```

#### C\#

```csharp
crate.AddRigidbody3D().SetMass(2.5f);
crate.AddBoxCollider3D(new Vector3(0.5f, 0.5f, 0.5f));
```

#### Python

```python
crate.get_or_add_default_component(ComponentKind.Rigidbody3D)
```

---

## CharacterController3DComponent

#### C++

```cpp
player->AddComponent<CharacterController3DComponent>();
player->AddComponent<CapsuleCollider3DComponent>(0.4F, 1.8F);
```

#### C\#

```csharp
player.GetOrAddComponent<CharacterController3DComponent>();
```

#### Python

```python
player.get_or_add_default_component(ComponentKind.CharacterController3D)
```

---

## TriggerVolume3DComponent

#### C++

```cpp
zone->AddComponent<SphereCollider3DComponent>(2.0F);
zone->AddComponent<TriggerVolume3DComponent>();
```

#### C\#

```csharp
zone.GetOrAddComponent<TriggerVolume3DComponent>();
```

#### Python

```python
zone.get_or_add_default_component(ComponentKind.TriggerVolume3D)
```

---

## MeshCollider3DComponent

#### C++

```cpp
static_cast<void>(meshGo->AddComponent<MeshCollider3DComponent>(meshAsset));
```

#### C\#

```csharp
meshGo.GetOrAddComponent<MeshCollider3DComponent>();
```

#### Python

```python
mesh_go.get_or_add_default_component(ComponentKind.MeshCollider3D)
```

---

## PhysicsMaterial3DComponent

#### C++

```cpp
go->AddComponent<PhysicsMaterial3DComponent>()->SetRestitution(0.3F);
```

#### C\#

```csharp
go.GetOrAddComponent<PhysicsMaterial3DComponent>()?.SetRestitution(0.3f);
```

#### Python

```python
go.get_or_add_default_component(ComponentKind.PhysicsMaterial3D)
```

---

## Joints (3D)

#### C++

```cpp
door->AddComponent<HingeJoint3DComponent>()->SetAxisLocal({0, 1, 0});
```

#### C\#

```csharp
door.GetOrAddComponent<HingeJoint3DComponent>();
```

#### Python

```python
door.get_or_add_default_component(ComponentKind.HingeJoint3D)
```

---

## Simulate

#### C++

```cpp
physics.SimulateAll3D(world, timing);
```

#### C\#

```csharp
_physics.SimulateAll3D(World, timing);
```

#### Python

```python
context.physics.simulate_all_3d(world, timing)
```

Next: [Animation components](08-animation.md).

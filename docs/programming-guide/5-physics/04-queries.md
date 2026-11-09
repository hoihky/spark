# Physics Queries

Query APIs live on `PhysicsSubsystem::GetQueries2D()` and `GetQueries3D()`. They share broad-phase cell size with simulation each step.

**2D combat:** `CombatHurtboxWorldQuery2D` — [Gameplay kit](../1-overview-architecture/09-gameplay-kit.md).

## Concept: 2D raycast

### C++

```cpp
PhysicsQueries2D& q = physics.GetQueries2D();
PhysicsRaycastHit2D hit{};
if (q.Raycast(world, origin, direction, maxDist, hit)) {
    GameObject* o = hit.object;
    (void)o;
}
```

Call after `Simulate2D` when dynamic positions must be current.

### C\#

```csharp
if (_physics.GetQueries2D().Raycast(World, origin, dir, maxDist, out var hit)) { }
```

### Python

```python
# physics queries via context.physics when interop exposes Raycast2D.
```

## Concept: overlap circle

### C++

```cpp
Array<PhysicsOverlapHit2D> hits;
q.OverlapCircle(world, center, radius, hits);
```

### C\#

```csharp
_physics.GetQueries2D().OverlapCircle(World, center, radius, hits);
```

### Python

```python
pass
```

## Concept: 3D raycast (FPS)

Used in [Part 9 shooting](../9-3d-game/04-shooting.md) and `PhysicsShowcase3DDemo`.

### C++

```cpp
PhysicsQueries3D& q3 = physics.GetQueries3D();
PhysicsRaycastHit3D hit3{};
if (q3.Raycast(world, ro, rd, maxDist, hit3)) {
    ApplyDamage(hit3);
}
```

Rebuild statics after level load: `q3.RebuildStatics(world)`.

### C\#

```csharp
_physics.GetQueries3D().Raycast(World, ro, rd, maxDist, out var hit);
```

### Python

```python
pass
```

## Picking vs physics

Screen → world for **tilemaps** uses orthographic unproject, not always physics raycast — [Tilemaps](../2-2d-graphics/03-tilemaps.md).

Next: [Physics 3D](05-physics-3d.md).

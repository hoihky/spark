# Physics 3D Contract

This chapter summarizes **what Spark 3D physics guarantees** for gameplay programmers — aligned with `PhysicsShowcase3DDemo` and `PhysicsSubsystem` sources.

## Simulation entry points

| Call | Includes |
|------|----------|
| `Simulate3D` | Rigidbodies, joints, some triggers |
| `SimulateAll3D` | Above + character controllers + trigger volume sync + query cell sync |

### C++

```cpp
physics.SimulateAll3D(world, timing);
```

### C\#

```csharp
_physics.SimulateAll3D(World, timing);
```

### Python

```python
context.physics.simulate_all_3d(world, timing)
```

## Query contract

- Queries share **broad-phase cell size** with `PhysicsWorld3D` after each simulate.
- Call `GetQueries3D().RebuildStatics(world)` after bulk static spawn.
- Dynamic rebuild: `RebuildDynamics` when adding many moving props mid-level.

### C++

```cpp
physics.GetQueries3D().RebuildStatics(world);
```

### C\#

```csharp
_physics.GetQueries3D().RebuildStatics(World);
```

### Python

```python
pass
```

## Trigger contract

- `TriggerVolume3DComponent` dispatches `SignalId::Physics3DTriggerEnter` / `Exit` to **siblings** on the same object.
- Collider must overlap; rigidbody optional depending on setup.

### C++

```cpp
void OnSignal(GameObject& owner, SignalId id, const SignalPayload& payload) override {
    if (id == SignalId::Physics3DTriggerEnter) {
        GameObject* other = static_cast<GameObject*>(payload.ptr);
    }
}
```

### C\#

```csharp
// GameComponent subclass OnSignal override in managed component when used.
```

### Python

```python
def on_signal(self, owner, signal_id, payload):
    pass
```

## Character controller contract

- Motor resolves against static + dynamic colliders baked that frame.
- Not a full rigidbody — use `Rigidbody3D` for throwing props.

## CCD / fast movers

Increase `sweptStaticCcdBinaryIterations` and cap velocity or use substep count > 1 for thrown balls — showcase **carry/throw** scene.

## Layering / filtering

Use gameplay tags + ignore lists in query filters where exposed in query API; default queries hit all registered colliders unless demo adds filters.

Further depth: [Collision layers](08-collision-layers-and-filters.md) · [Triggers & showcase](09-triggers-contacts-and-showcase.md) (Part 5 closing chapter).

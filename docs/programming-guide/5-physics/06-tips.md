# Tips and Patterns

## Fixed Timestep

```cpp
PhysicsSubsystem physics;
float accum = 0.0F;
constexpr float kFixedDt = 1.0F / 60.0F;
accum += timing.deltaTimeSeconds;
while (accum >= kFixedDt) {
    physics.Simulate2D(world, FrameTiming{kFixedDt, timing.totalTimeSeconds, timing.frameIndex});
    accum -= kFixedDt;
}
```

## Layer Design Example

| Layer bit | Category | Collides with |
|-----------|----------|---------------|
| 0 | Player | 1, 2 |
| 1 | Environment | all |
| 2 | Enemy | 0, 1 |
| 3 | Pickup (trigger) | 0 |

## Dynamic vs Dynamic

Default `resolveDynamicVsDynamic = false` in 2D — enable only if you need pile-ups.

## Query Performance

Rebuild static queries once per frame (or when level geometry changes):

```cpp
physics.GetQueries2D().RebuildStatics(world);
// then many OverlapCircleStatics / RaycastStatics calls
```

`PhysicsSubsystem::Simulate2D` syncs query cell size from `PhysicsWorld2D` each step. Use `SetBroadPhaseCellSize2D` when tuning the spatial hash for both simulation and queries.

## Debug Visualization

Use `TextOverlayComponent` or GUI labels to show velocity:

```cpp
std::format("v=({:.1f},{:.1f})", v.x, v.y);
```

## When to Skip Physics

Grid-based tactics, visual novels, and menu scenes need no solver — omit `PhysicsSubsystem::Simulate*`.

Part 5 complete → **Part 6**: [Sound Engine](../6-sound/01-sound-engine.md).

---
title: Steering Behaviors
order: 2
---

# Steering Behaviors

## Class Design: `ISteeringBehavior` (2D)

```cpp
class ISteeringBehavior {
public:
    virtual Vector2 ComputeAcceleration(
        const Vector2& positionXZ,
        const Vector2& velocityXZ,
        AiBlackboard& board) const = 0;
};
```

Implementations in `spark/ai/steering/SteeringBehaviors.hpp`:

- `SteeringSeek`, `SteeringFlee`, `SteeringArrive`, `SteeringWander`
- `SteeringPursuit`, `SteeringEvade`, `SteeringSeparation`

## `SteeringComposer`

Weighted sum of behaviors:

```cpp
SteeringComposer composer;
composer.Add(&seek, 1.0F);
composer.Add(&separation, 0.8F);
Vector2 accel = composer.Compose(posXZ, velXZ, blackboard);
```

## 3D Steering

`spark/ai/steering/SteeringBehaviors3D.hpp` provides `ISteeringBehavior3D` and `SteeringComposer3D`:

- `SteeringSeek3D`, `SteeringFlee3D`, `SteeringObstacleAvoidance3D`
- `SteeringFlocking3D` (separation + alignment + cohesion)

## Blackboard Targets

Store goal position in blackboard slots (indices are game-defined):

```cpp
AiBlackboard& bb = agent->GetBlackboard();
bb.SetFloat(0, targetPos.x);  // slot 0 = target X
bb.SetFloat(1, targetPos.z);  // slot 1 = target Z
```

Behaviors read slots in `ComputeAcceleration`.

## Apply to Rigidbody

```cpp
Vector2 vel = rb->GetVelocity();
vel += accel * timing.deltaTimeSeconds;
vel = vel.ClampLength(agent->GetMaxSpeed());
rb->SetVelocity(vel);
```

Next: [Finite State Machines](4-ai/03-fsm.html).

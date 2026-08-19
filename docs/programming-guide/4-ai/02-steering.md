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

## ECS Patrol Agent (from `SteeringShowcase3DDemo`)

For nav-mesh patrol without manual steering math, combine `PatrolPathComponent` + `NavMeshAgentComponent` + `AiAgentComponent`:

```cpp
GameObject* pathGo = world.CreateGameObject();
pathGo->AddComponent<TransformComponent>()->SetTranslation({-14.0F, 0.0F, -12.0F});
PatrolPathComponent* patrol = pathGo->AddComponent<PatrolPathComponent>();
patrol->SetLooping(true);
const float leg = 5.0F;
patrol->GetWaypoints().PushBack(Vector3::Zero);
patrol->GetWaypoints().PushBack({leg, 0.0F, 0.0F});
patrol->GetWaypoints().PushBack({leg, 0.0F, leg});
patrol->GetWaypoints().PushBack({0.0F, 0.0F, leg});

GameObject* agentGo = world.CreateGameObject();
agentGo->GetComponent<TransformComponent>()->SetTranslation({-14.0F, 0.55F, -12.0F});
NavMeshAgentComponent* nav = agentGo->AddComponent<NavMeshAgentComponent>();
nav->SetPatrolPathObject(pathGo);
AiAgentComponent* agent = agentGo->AddComponent<AiAgentComponent>();
agent->SetMaxSpeed(2.8F);
agent->SetSteeringPlane(AiSteeringPlane::XzWorld);

// Each frame:
SimulateGameAi(world, timing, context);
```

## Apply to Rigidbody

```cpp
Vector2 vel = rb->GetVelocity();
vel += accel * timing.deltaTimeSeconds;
vel = vel.ClampLength(agent->GetMaxSpeed());
rb->SetVelocity(vel);
```

Next: [Finite State Machines](03-fsm.md).

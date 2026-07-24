# Player Controller

## Spawn Player

```cpp
playerObject = world.CreateGameObject();
playerTr = playerObject->AddComponent<TransformComponent>();
playerTr->SetScale({0.88F, 1.05F, 1.0F});
playerTr->SetTranslation({kPlayerSpawnX, spawnY, 0.04F});

playerObject->AddComponent<SpriteComponent>(
    tileTex, Vector4{0.35F, 0.55F, 0.95F, 1.0F},
    Vector4{0,0,1,1}, 500);

playerObject->AddComponent<BoxCollider2DComponent>();
playerRb = playerObject->AddComponent<Rigidbody2DComponent>(
    RigidbodyBodyType2D::Dynamic, 1.0F);
```

## Movement + Jump

```cpp
float run = 0.0F;
if (in.IsKeyDown(GLFW_KEY_A) || in.IsKeyDown(GLFW_KEY_LEFT)) run -= 1.0F;
if (in.IsKeyDown(GLFW_KEY_D) || in.IsKeyDown(GLFW_KEY_RIGHT)) run += 1.0F;

if (std::fabs(run) > 0.5F) facingLeft = (run < 0.0F);
playerTr->SetScale({facingLeft ? -baseScaleX : baseScaleX, baseScaleY, 1.0F});

Vector2 v = playerRb->GetVelocity();
v.x = run * kRunSpeed;
if (playerRb->IsGrounded() && in.IsKeyPressedThisFrame(GLFW_KEY_SPACE))
    v.y = kJumpSpeed;
playerRb->SetVelocity(v);

PhysicsWorld2DSettings phys{};
phys.gravityY = -30.0F;
SimulatePhysics2D(GetWorld(), timing, phys);
```

## Respawn + Goal

```cpp
if (p.y < kFallRespawnY) {
    playerTr->SetTranslation({kPlayerSpawnX, spawnY, p.z});
    playerRb->SetVelocity(Vector2::Zero);
}
if (!goalReached && p.x >= kGoalMinX)
    goalReached = true;
```

Next: [Camera and HUD](05-camera-hud.md).

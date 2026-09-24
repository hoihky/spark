# Player Controller

The platformer demo (`Platformer2DDemo`) uses a **component stack** instead of raw GLFW polling in `Simulate`. This chapter mirrors that pattern: semantic input → character motor → animation hitbox.

## Component Stack

| Component | Role |
|-----------|------|
| `InputActionMapComponent` | Flyweight key bindings (`MoveX`, `Jump`, `Attack`, …) |
| `PlayerInputComponent` | Polls `IInput` each frame; exposes `WasActionPressedThisFrame` |
| `CharacterController2DComponent` | Coyote time, jump buffer, one-way drop-through |
| `Rigidbody2DComponent` + `BoxCollider2DComponent` | Physics body the motor drives |
| `Sprite2DCharacterAnimFsmComponent` + `SpriteAnimatorComponent` | Locomotion + attack clips |
| `AnimationHitbox2DComponent` | Frame-synced melee arc during attack clip |

## Spawn Player

```cpp
playerObject = world.CreateGameObject();
playerObject->GetName() = Utf8String("Player");
playerTr = playerObject->AddComponent<TransformComponent>();
playerTr->SetScale({0.88F, 1.05F, 1.0F});
playerTr->SetTranslation({kPlayerSpawnX, spawnY, 0.04F});

playerObject->AddComponent<SpriteComponent>(
    heroTex, Vector4{1,1,1,1}, Vector4{0,0,1,1}, 500);

auto* fsm = playerObject->AddComponent<Sprite2DCharacterAnimFsmComponent>();
auto* anim = playerObject->AddComponent<SpriteAnimatorComponent>();
anim->SetUniformGrid(4, 4);
anim->AddClip(SpriteAnimationClip{0, 1, 1.0F, true});   // idle
anim->AddClip(SpriteAnimationClip{1, 2, 10.0F, true});  // run
anim->AddClip(SpriteAnimationClip{2, 1, 14.0F, false});  // attack
fsm->SetLocomotionClips(0, 1);
fsm->SetCombatClips(2, 3);
fsm->SetLocomotionSource(Sprite2DAnimLocomotionSource::HorizontalAbsVelX);

playerObject->AddComponent<BoxCollider2DComponent>();
playerRb = playerObject->AddComponent<Rigidbody2DComponent>(
    RigidbodyBodyType2D::Dynamic, 1.0F);

playerController = playerObject->AddComponent<CharacterController2DComponent>();
playerController->SetMoveSpeed(11.0F);
playerController->SetJumpSpeed(13.2F);
```

## Input Action Map

```cpp
auto* actionMap = playerObject->AddComponent<InputActionMapComponent>();
actionMap->BindAxis1D("MoveX", GLFW_KEY_A, GLFW_KEY_D, GLFW_KEY_LEFT, GLFW_KEY_RIGHT);
actionMap->BindButton("Jump", GLFW_KEY_SPACE);
actionMap->BindButton("Drop", GLFW_KEY_S, GLFW_KEY_DOWN);
actionMap->BindButton("Attack", GLFW_KEY_J);

playerInput = playerObject->AddComponent<PlayerInputComponent>();
playerInput->SetActionMap(actionMap);
```

`PlayerInputComponent` runs at priority **50**, before most gameplay `OnUpdate` hooks. Game code in `Simulate` can read actions immediately after `UpdateGameObjects` or call `Refresh` earlier if needed.

## Movement + Jump (each frame)

```cpp
const float moveX = playerInput->GetActionAxis1D("MoveX");
playerController->SetMoveInputX(moveX);

if (playerInput->WasActionPressedThisFrame("Jump")) {
    playerController->RequestJump();
}
if (playerInput->IsActionPressed("Drop")) {
    playerController->SetDropThroughOneWay(true);
}

// Face flip from input or velocity:
if (std::fabs(moveX) > 0.1F) {
    facingLeft = (moveX < 0.0F);
}
playerTr->SetScale({facingLeft ? -baseScaleX : baseScaleX, baseScaleY, 1.0F});

if (playerInput->WasActionPressedThisFrame("Attack")) {
    fsm->RequestAttack();
}

physics.Simulate2D(GetWorld(), timing);
```

Configure gravity once in `OnAttach`:

```cpp
physics.GetWorld2D().GetSettings().gravityY = -30.0F;
physics.GetWorld2D().GetSettings().maxFallSpeed = 42.0F;
```

## Melee Hitbox

```cpp
auto* melee = playerObject->AddComponent<AnimationHitbox2DComponent>();
melee->SetClipIndex(2U);
melee->SetStartLocalFrame(0U);
melee->SetEndLocalFrame(0U);
melee->SetShape(AnimationHitbox2DShape::Arc);
melee->SetRadius(1.1F);
melee->SetDamagePerHit(1.0F);

PhysicsQueryFilter2D filter{};
filter.queryCategoryBits = 1u << 3;
filter.queryMaskBits = 1u << 4;
filter.hitTriggers = true;
filter.hitSolids = false;
melee->SetQueryFilter(filter);
```

The hitbox runs overlap queries only while the attack clip's local frame is inside the configured window.

## Respawn + Fall Death

```cpp
const Vector3 p = playerTr->GetLocalTransform().translation;
if (p.y < kFallRespawnY) {
    playerTr->SetTranslation({kPlayerSpawnX, spawnY, p.z});
    playerRb->SetVelocity(Vector2::Zero);
}
```

Use `GameStateComponent` + `GameFlowTriggerComponent` for goal and defeat flow instead of ad-hoc booleans — see [Camera and HUD](05-camera-hud.md).

Next: [Camera and HUD](05-camera-hud.md).

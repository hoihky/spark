# Player Controller

The platformer demo (`Platformer2DDemo`) uses a **component stack** instead of raw GLFW polling in `Simulate`. This chapter mirrors that pattern: semantic input → character motor → animation hitbox.

**Bindings:** [Part 7 — Input & gameplay](../7-game-components-usage/09-input-and-gameplay.md) · [Physics 2D](../7-game-components-usage/06-physics-2d.md) · [Animation](../7-game-components-usage/08-animation.md).

## Component Stack

| Component | Role |
|-----------|------|
| `InputActionMapComponent` | Flyweight key bindings (`MoveX`, `Jump`, `Attack`, …) |
| `PlayerInputComponent` | Polls `IInput` each frame; exposes `WasActionPressedThisFrame` |
| `CharacterController2DComponent` | Coyote time, jump buffer, one-way drop-through |
| `Rigidbody2DComponent` + `BoxCollider2DComponent` | Physics body the motor drives |
| `Sprite2DCharacterAnimFsmComponent` + `SpriteAnimatorComponent` | Locomotion + attack clips |
| `AnimationHitbox2DComponent` | Frame-synced melee arc during attack clip |
| `HealthComponent` + `DamageableComponent` | Player vitals (optional) |
| `Hurtbox2DComponent` | On enemies — receives melee / projectiles |
| `Projectile2DComponent` | Ranged attacks (platformer shooting) |
| `GridNavAgent2DComponent` | Top-down click-to-move (P0 path) |

## Spawn Player

### C++

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

### C\#

```csharp
var player = world.CreateGameObject("Player");
player.GetOrAddTransform().SetTranslation(new Vector3(kPlayerSpawnX, spawnY, 0.04f));
player.AddSprite(heroTex, Vector4.One, new Vector4(0, 0, 1, 1), sortOrder: 500);
var fsm = player.AddSprite2DCharacterAnimFsm();
var anim = player.AddSpriteAnimator();
anim.SetUniformGrid(4, 4);
anim.AddClip(new SpriteAnimationClip(0, 1, 1.0f, true));
fsm.SetLocomotionClips(0, 1);
fsm.SetCombatClips(2, 3);
player.AddBoxCollider2D();
player.AddRigidbody2D(RigidbodyBodyType2D.Dynamic, 1.0f);
player.AddCharacterController2D().SetMoveSpeed(11f).SetJumpSpeed(13.2f);
```

### Python

```python
from spark._generated.component_kind import ComponentKind

player = world.create_game_object("Player")
player.get_or_add_transform().translation = Vector3(k_player_spawn_x, spawn_y, 0.04)
player.add_sprite("platformer/hero", Vector4(1, 1, 1, 1), sort_order=500)
player.get_or_add_default_component(ComponentKind.Sprite2DCharacterAnimFsm)
player.get_or_add_default_component(ComponentKind.Rigidbody2D)
player.get_or_add_default_component(ComponentKind.CharacterController2D)
```

## Input Action Map

### C++

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

### C\#

```csharp
var map = player.AddInputActionMap();
map.BindAxis1D("MoveX", Key.A, Key.D, Key.Left, Key.Right);
map.BindButton("Jump", Key.Space);
player.AddPlayerInput().SetActionMap(map);
```

### Python

```python
player.get_or_add_default_component(ComponentKind.InputActionMap)
player.get_or_add_default_component(ComponentKind.PlayerInput)
```

## Movement + Jump (each frame)

### C++

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

### C\#

```csharp
var moveX = playerInput.GetActionAxis1D("MoveX");
playerController.SetMoveInputX(moveX);
if (playerInput.WasActionPressedThisFrame("Jump"))
    playerController.RequestJump();
if (playerInput.WasActionPressedThisFrame("Attack"))
    fsm.RequestAttack();
_physics.Simulate2D(World, timing);
```

### Python

```python
# After world.update_game_objects: read player_input, set character_controller intent,
# then context.physics.simulate_2d(world, timing)
```

Configure gravity once in `OnAttach`:

### C++

```cpp
physics.GetWorld2D().GetSettings().gravityY = -30.0F;
physics.GetWorld2D().GetSettings().maxFallSpeed = 42.0F;
```

### C\#

```csharp
_physics.GetWorld2D().GetSettings().GravityY = -30f;
```

### Python

```python
context.physics.get_world_2d().gravity_y = -30.0
```

## Melee Hitbox

### C++

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

### C\#

```csharp
var melee = player.AddAnimationHitbox2D();
melee.SetClipIndex(2).SetShape(AnimationHitbox2DShape.Arc).SetRadius(1.1f);
```

### Python

```python
player.get_or_add_default_component(ComponentKind.AnimationHitbox2D)
```

## Respawn + Fall Death

### C++

```cpp
const Vector3 p = playerTr->GetLocalTransform().translation;
if (p.y < kFallRespawnY) {
    playerTr->SetTranslation({kPlayerSpawnX, spawnY, p.z});
    playerRb->SetVelocity(Vector2::Zero);
}
```

### C\#

```csharp
if (p.Y < kFallRespawnY)
{
    playerTr.SetTranslation(new Vector3(kPlayerSpawnX, spawnY, p.Z));
    playerRb.SetVelocity(Vector2.Zero);
}
```

Use `GameStateComponent` + `GameFlowTriggerComponent` for goal and defeat flow instead of ad-hoc booleans — see [Camera and HUD](05-camera-hud.md).

## Top-down / grid movement (P0 product path)

The platformer uses `CharacterController2DComponent`. For **tilemap grid** games (shell **F**), use `GridNavAgent2DComponent` on the player:

### C++

```cpp
playerNav = playerObject->AddComponent<GridNavAgent2DComponent>();
playerNav->SetGridSourceObject(levelRoot);
playerNav->SetGoalMode(GridNavGoalMode2D::GridCell);
playerNav->SetSyncToAiAgent(false);

// Left-click: pick walkable cell, then:
playerNav->SetGoalCell(cell);
playerNav->RequestRepath();

// Each frame (after ProcessGridNavAgents2D):
ApplyGridNavAgent2DRigidbodyMotion(*playerNav, *playerTr, *playerRb, speed, arriveRadius, dt);

// WASD: ClearPath(), set velocity directly; skip path follower.
```

Frame order matters: run `ProcessGridNavAgents2D` **after** setting goals / patrol waypoints. See [Pathfinding](../4-ai/05-pathfinding.md) and [Gameplay API](09-2d-gameplay-api-guide.md#7-grid-navigation-player-and-npcs).

### C\#

```csharp
player.AddGridNavAgent2D()
    .SetGridSourceObject(levelRoot)
    .SetGoalMode(GridNavGoalMode2D.GridCell);
```

### Python

```python
nav = player.get_or_add_default_component(ComponentKind.GridNavAgent2D)
```

## Combat components (melee + hazards)

**Melee** — `AnimationHitbox2DComponent` enables a damage window during an attack clip; overlap queries hit `Hurtbox2DComponent` on enemies.

### C++

```cpp
#include "spark/ecs/components/animation/AnimationHitbox2DComponent.hpp"
#include "spark/ecs/components/physics/2d/Hurtbox2DComponent.hpp"
#include "spark/ecs/components/gameplay/HealthComponent.hpp"

player->AddComponent<AnimationHitbox2DComponent>()->SetAttackClipIndex(2);

auto* hurt = enemy->AddComponent<Hurtbox2DComponent>();
hurt->SetShape(Hurtbox2DShape::Circle);
hurt->SetRadius(0.65F);
enemy->AddComponent<HealthComponent>(3.0F);
```

### C\#

```csharp
player.AddAnimationHitbox2D().SetAttackClipIndex(2);
enemy.AddHurtbox2D().SetRadius(0.65f);
enemy.AddHealth(3f);
```

**Hazards** — `DamageZone2DComponent` applies DPS inside a volume:

### C++

```cpp
#include "spark/ecs/components/gameplay/DamageZone2DComponent.hpp"

lava->AddComponent<DamageZone2DComponent>()->SetDamagePerSecond(20.0F);
```

### C\#

```csharp
lava.AddDamageZone2D().SetDamagePerSecond(20f);
```

**Projectiles** — see `Projectile2DComponent` in [Components handbook](10-2d-components-handbook.md#projectile2d).

Next: [Camera and HUD](05-camera-hud.md).

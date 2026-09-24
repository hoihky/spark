# 2D Animation

## Sprite Sheet Playback

Manual frame stepping on `SpriteComponent`:

```cpp
struct AnimFrame { Vector4 uv; float duration; };
Array<AnimFrame> runLoop;

void TickSpriteAnim(const FrameTiming& timing, SpriteComponent* sprite) {
    static float accum = 0.0F;
    static int frame = 0;
    accum += timing.deltaTimeSeconds;
    if (accum >= runLoop[frame].duration) {
        accum = 0.0F;
        frame = (frame + 1) % static_cast<int>(runLoop.GetSize());
        sprite->SetUvRect(runLoop[frame].uv);
    }
}
```

## `SpriteAnimatorComponent`

Grid-atlas clips with crossfade support (`spark/ecs/components/animation/SpriteAnimatorComponent.hpp`). **Update priority 200** — runs after FSM drivers (priority 100).

```cpp
go->AddComponent<SpriteComponent>(texture);
auto* sa = go->AddComponent<SpriteAnimatorComponent>();
sa->SetUniformGrid(4, 4);
SpriteAnimationClip run{};
run.firstFrame = 4;
run.frameCount = 8;
run.framesPerSecond = 10.0F;
run.loop = true;
sa->AddClip(run);
sa->SetClipIndex(0);
```

## `Sprite2DCharacterAnimFsmComponent`

Higher-level locomotion + combat overlay. Add **before** `SpriteAnimatorComponent` on the same object (priority 100).

```cpp
go->AddComponent<Sprite2DCharacterAnimFsmComponent>();
go->AddComponent<SpriteAnimatorComponent>();
auto* fsm = go->GetComponent<Sprite2DCharacterAnimFsmComponent>();
fsm->RequestAttack();
fsm->RequestHurt();
```

Uses `Rigidbody2D` velocity for locomotion when present; can read `AiAgentComponent` blackboard for combat commands.

## Sprite Animation Events

### `SpriteAnimationEventReceiverComponent`

**Priority:** 215 (after `SpriteAnimatorComponent` at 200). Markers use **normalized clip time** (0..1).

```cpp
#include "spark/ecs/components/animation/SpriteAnimationEventReceiverComponent.hpp"

auto* recv = go->AddComponent<SpriteAnimationEventReceiverComponent>();
recv->AddMarker(1, 0.25F, "Footstep");  // run clip, 25%
recv->AddMarker(1, 0.75F, "Footstep");
```

Fires `SignalId::SpriteAnimationEvent` to sibling components (`ptr` = event name, `a` = clip index).

### `SpriteAnimationEventVfxComponent`

Listens for `SpriteAnimationEvent` and queues VFX from `VfxLibrary` by asset key:

```cpp
#include "spark/ecs/components/animation/SpriteAnimationEventVfxComponent.hpp"

auto* vfx = go->AddComponent<SpriteAnimationEventVfxComponent>();
vfx->AddBinding("Footstep", "dust_puff");
vfx->SetWorldOffset({0.0F, -0.35F, 0.0F});
```

Pair receiver + VFX on the same `GameObject` as `SpriteAnimatorComponent`.

## `AnimationHitbox2DComponent`

Frame-synced melee / hurtbox during sprite clips. Active when local frame ∈ `[start, end]`:

```cpp
#include "spark/ecs/components/animation/AnimationHitbox2DComponent.hpp"

auto* hit = go->AddComponent<AnimationHitbox2DComponent>();
hit->SetClipIndex(2U);
hit->SetStartLocalFrame(0U);
hit->SetEndLocalFrame(0U);
hit->SetShape(AnimationHitbox2DShape::Arc);
hit->SetRadius(1.0F);
```

See [Player Controller](../7-2d-game/04-player-controller.md) and [Queries](../5-physics/04-queries.md).

## `AnimationEventReceiverComponent` (3D)

For skeletal clips, use markers on the same object as `AnimatorComponent`:

```cpp
auto* recv = go->AddComponent<AnimationEventReceiverComponent>();
recv->AddMarker(0, 0.5F, "Footstep");
```

Fires `SignalId::AnimationEvent` to sibling components when the animator crosses each marker.

See [Game Component Reference](../1-overview-architecture/07-game-component-reference.md#animation).

## Flip Without Extra Textures

```cpp
bool facingLeft = velocity.x < 0.0F;
tr->SetScale({facingLeft ? -baseScaleX : baseScaleX, baseScaleY, 1.0F});
```

## `TextOverlayComponent` HUD

Screen-space text without full GUI:

```cpp
auto* hud = world.CreateGameObject();
auto* text = hud->AddComponent<TextOverlayComponent>();
text->SetScreenPosition(12.0F, 12.0F);
text->SetFontSizePixels(20.0F);
text->SetColor({0.94F, 0.97F, 1.0F});
text->SetText(Utf8String("Score: 0"));
```

Requires `world.SetUiFont(font)` — see platformer `MountUiFontIfNeeded`.

## Squash and Stretch (Juice)

```cpp
const float vy = playerRb->GetVelocity().y;
const float stretch = 1.0F + std::clamp(vy * 0.02F, -0.15F, 0.15F);
playerTr->SetScale({baseScaleX / stretch, baseScaleY * stretch, 1.0F});
```

Next: [2D Lighting](05-2d-lighting.md).

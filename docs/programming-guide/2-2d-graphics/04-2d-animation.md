---
title: 2D Animation
order: 4
---

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

## `AnimationEventReceiverComponent` (3D)

For skeletal clips, use markers on the same object as `AnimatorComponent`:

```cpp
auto* recv = go->AddComponent<AnimationEventReceiverComponent>();
recv->AddMarker(0, 0.5F, "Footstep");
```

Fires `SignalId::AnimationEvent` to sibling components when the animator crosses each marker.

See [Game Component Reference](../1-overview-architecture/07-game-component-reference.html#animation).

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

Next: [2D Lighting](2d-graphics/05-2d-lighting.html).

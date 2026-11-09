# Component Guide: Animation

Animation components drive **skeletal** (3D) or **sprite sheet** (2D) playback. Respect update priorities: FSM **100** → animator **200** → event receivers **210–215**.

| Component | Space |
|-----------|-------|
| `AnimatorComponent` | 3D skeletal |
| `Character3DAnimFsmComponent` | 3D locomotion FSM |
| `SpriteAnimatorComponent` | 2D clips |
| `Sprite2DCharacterAnimFsmComponent` | 2D locomotion FSM |
| `AnimationEventReceiverComponent` | 3D markers |
| `AnimationEventVfxComponent` | 3D VFX bind |
| `SpriteAnimationEventReceiverComponent` | 2D markers |
| `SpriteAnimationEventVfxComponent` | 2D VFX bind |
| `AnimationHitbox2DComponent` | 2D combat frames |
| `AnimationMeleeHitComponent` | 3D melee sweep |
| `AttachmentSocketComponent` | Weapon attach |
| `RootMotionComponent` | Root motion delta |
| `FootIkComponent` / `AimIkComponent` | IK solvers |

---

## SpriteAnimatorComponent

#### C++

```cpp
go->AddComponent<SpriteComponent>(tex, tint, uv, 100);
SpriteAnimatorComponent* sa = go->AddComponent<SpriteAnimatorComponent>();
sa->SetUniformGrid(4, 4);
SpriteAnimationClip run{};
run.firstFrame = 0; run.frameCount = 8; run.framesPerSecond = 10.0F; run.loop = true;
sa->AddClip(run);
sa->SetClipIndex(0);
```

#### C\#

```csharp
go.AddSpriteAnimator();
var sa = go.GetComponent<SpriteAnimatorComponent>();
sa.SetUniformGrid(4, 4);
```

#### Python

```python
go.add_sprite(tex_key, tint, uv, 100)
anim = go.get_or_add_default_component(ComponentKind.SpriteAnimator)
```

---

## Sprite2DCharacterAnimFsmComponent

Add **before** or after — sort order fixes priority; FSM always runs first.

#### C++

```cpp
go->AddComponent<Sprite2DCharacterAnimFsmComponent>();
go->AddComponent<SpriteAnimatorComponent>();
go->GetComponent<Sprite2DCharacterAnimFsmComponent>()->RequestAttack();
```

#### C\#

```csharp
go.AddSprite2DCharacterAnimFsm();
```

#### Python

```python
go.get_or_add_default_component(ComponentKind.Sprite2DCharacterAnimFsm)
```

---

## SpriteAnimationEventReceiverComponent + Vfx

#### C++

```cpp
recv->AddMarker(1, 0.25F, "Footstep");
vfx->AddBinding("Footstep", "footstep_puff_2d");
```

#### C\#

```csharp
recv.AddMarker(1, 0.25f, "Footstep");
```

#### Python

```python
go.get_or_add_default_component(ComponentKind.SpriteAnimationEventReceiver)
```

---

## AnimationHitbox2DComponent

#### C++

```cpp
hit->SetClipIndex(2);
hit->SetShape(AnimationHitbox2DShape::Arc);
hit->SetRadius(1.0F);
```

#### C\#

```csharp
hit.SetRadius(1f);
```

#### Python

```python
go.get_or_add_default_component(ComponentKind.AnimationHitbox2D)
```

---

## AnimatorComponent (3D)

#### C++

```cpp
AnimatorComponent* anim = go->AddComponent<AnimatorComponent>(skel, walkIndex);
anim->SetLoopMode(AnimLoopMode::Loop);
anim->SetPlaybackSpeed(1.1F);
```

#### C\#

```csharp
go.AddAnimator(skeleton, walkIndex);
```

#### Python

```python
go.get_or_add_default_component(ComponentKind.Animator)
```

---

## Character3DAnimFsmComponent

#### C++

```cpp
fsm->ConfigureLocomotionFromSkeleton(*skel, walkIndex);
```

#### C\#

```csharp
go.AddCharacter3DAnimFsm();
```

#### Python

```python
go.get_or_add_default_component(ComponentKind.Character3DAnimFsm)
```

---

## AnimationEventReceiverComponent (3D)

#### C++

```cpp
recv->AddMarker(0, 0.5F, "Footstep");
```

#### C\#

```csharp
go.AddAnimationEventReceiver();
```

#### Python

```python
go.get_or_add_default_component(ComponentKind.AnimationEventReceiver)
```

---

## AttachmentSocketComponent

**Priority 250** — places child objects on skeleton joints.

#### C++

```cpp
socket->SetJointIndex(skeleton, jointName);
```

#### C\#

```csharp
go.AddAttachmentSocket();
```

#### Python

```python
go.get_or_add_default_component(ComponentKind.AttachmentSocket)
```

---

## AnimationMeleeHitComponent

3D melee trace during clip window.

#### C++

```cpp
melee->SetClipIndex(attackClip);
melee->SetDamage(20.0F);
```

#### C\#

```csharp
go.GetOrAddComponent<AnimationMeleeHitComponent>();
```

#### Python

```python
go.get_or_add_default_component(ComponentKind.AnimationMeleeHit)
```

Next: [Input and gameplay components](09-input-and-gameplay.md).

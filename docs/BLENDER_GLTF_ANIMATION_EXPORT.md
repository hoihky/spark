# Blender → glTF animation export (Spark)

Spark reads **animation gameplay events**, **locomotion clips**, and **root-motion joints** from glTF plus optional sidecar JSON. This page is the artist/engine checklist for 3D character export.

## Quick checklist

| Rule | Why |
|------|-----|
| ≤ **64 joints** per skin (current GPU limit) | Palette SSBO size in `scene.vert` |
| Name clips **`Idle`**, **`Walk`**, **`Run`**, **`Attack`**, **`Hurt`** (substring match) | `Character3DAnimFsmComponent` auto-resolve |
| Name root/hips joint with **`hip`**, **`root`**, or **`pelvis`** | `RootMotionComponent` pattern resolver |
| Use **LINEAR** rotation/translation keys | STEP / CUBICSPLINE not fully supported yet |
| Add melee events **`active_start`** / **`active_end`** on attack clips | `AnimationMeleeHitComponent` hit window |
| Place sidecar next to `.glb` when using events | See below |

## glTF animation extras (reserved)

When the bundled cgltf build exposes animation extras JSON, Spark will read:

```json
{ "sparkEvents": [ { "time": 0.18, "name": "active_start" } ] }
```

Until then, use the **sidecar file** below (works today).

## Sidecar file (recommended for events)

Place next to the model:

`MyCharacter.glb` → `MyCharacter.spark-anim-events.json`

```json
{
  "version": 1,
  "clips": [
    {
      "name": "Run",
      "events": [
        { "time": 0.18, "name": "active_start" },
        { "time": 0.42, "name": "active_end" },
        { "time": 0.30, "name": "footstep" }
      ]
    }
  ]
}
```

- Clip `name` matches the glTF animation name (case-insensitive).
- Event `name` is a UTF-8 string; engine convention for combat windows is `active_start` / `active_end`.
- `time` is seconds from clip start (absolute, not normalized).

## Event naming conventions

| Event name | Typical use |
|------------|-------------|
| `active_start` | Begin hit trace / damage window |
| `active_end` | End hit trace / damage window |
| `footstep` | VFX / SFX (see `AnimationEventVfxComponent`) |
| `spawn_vfx` | Custom gameplay hook from C# callback |

## Runtime wiring (C++)

```cpp
auto* recv = character->AddComponent<AnimationEventReceiverComponent>();
recv->ImportFromSkeleton(*skeleton);

auto* melee = character->AddComponent<AnimationMeleeHitComponent>();
melee->SetFacingObject(characterRoot);
melee->AddTarget(enemy);

auto* socket = character->AddComponent<AttachmentSocketComponent>();
socket->SetSourceObject(characterVisual);
socket->SetAttachedObject(weaponObject);
socket->SetJointByNamePattern("hand");  // or SetJointIndex(...)
socket->SetLocalOffset({0, 0, 0.2f});

auto* rootMotion = characterRoot->AddComponent<RootMotionComponent>();
rootMotion->SetAnimatorObject(characterVisual);
rootMotion->SetApplyTarget(characterRoot);
rootMotion->UsePatternMotionJointResolver();
rootMotion->SetInPlace(true);  // toggle false to drive transform from clip
```

## C# gameplay events

```csharp
receiver.SetCallback((owner, payload) =>
{
    Console.WriteLine($"Event {payload.EventName} clip {payload.ClipIndex} @ {payload.TimeSeconds}");
});
```

See `AnimationEventReceiverComponent` in `Spark.Bindings`.

## Root motion notes

- Export locomotion with motion on the **hips/root** joint (not only on a child mesh).
- Spark extracts per-frame translation from the resolved motion joint and applies it to a target transform.
- Use `SetInPlace(true)` when gameplay code (WASD / character controller) owns world movement.

## Attachment sockets

- Bind props, weapons, or VFX anchors with `AttachmentSocketComponent`.
- Joint index or `SetJointByNamePattern("head")` (case-insensitive substring).
- Socket follows locomotion blend and crossfade poses automatically.

## Load-time validation (M6)

`GameWorld::LoadSkinnedGltf` runs `GltfSkinnedLoadValidator` after a successful parse. Non-fatal issues are logged to **stderr** with the asset path:

| Check | Warning |
|-------|---------|
| Joint count > 64 | Exceeds recommended GPU skinning budget |
| Joint count = `Skeleton::MaxJoints` (128) | At engine hard limit |
| No animation clips | Skinned mesh has no clips |
| Clip duration ≈ 0 | Clip name + duration in message |
| Missing `inverse_bind_matrices` | Skinning may be incorrect |
| Non-invertible inverse bind | One or more joint matrices invalid |
| Vertex joint index ≥ joint count | Bad skin weights |

Fix these in Blender before shipping; the load still succeeds so you can inspect in-engine.

## Demo

**Character camera demo**

- **WASD** — analog walk/run blend (magnitude controls walk↔run)
- **Shift+WASD** — sprint
- **F** — attack (`active_start` / `active_end` on Fox `Run` clip)
- **H** — hurt
- **R** — toggle root motion
- **B** — toggle skeleton debug draw (bone segments + joint markers)
- **K** — toggle foot IK
- **L** — toggle aim IK
- **M** — switch Fox ↔ CesiumMan
- Yellow cube on head/hip socket shows attachment follow

Sample sidecars: `assets/models/Fox.spark-anim-events.json`, `CesiumMan.spark-anim-events.json`.

**Sample asset catalog:** see [`ANIMATION_SAMPLE_ASSETS.md`](ANIMATION_SAMPLE_ASSETS.md) for Fox / CesiumMan clip tables and FSM resolve behavior.

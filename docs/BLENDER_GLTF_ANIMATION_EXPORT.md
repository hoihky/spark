# Blender → glTF animation events (Spark)

Spark reads **animation gameplay events** from glTF or a sidecar JSON file. Events drive `SignalId::AnimationEvent` through `AnimationEventReceiverComponent`.

## glTF animation extras (reserved)

When the bundled cgltf build exposes animation extras JSON, Spark will read:

```json
{ "sparkEvents": [ { "time": 0.18, "name": "active_start" } ] }
```

Until then, use the **sidecar file** below (works today).

Until then, use the **sidecar file** below (works today).

## Sidecar file (recommended)

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
        { "time": 0.42, "name": "active_end" }
      ]
    }
  ]
}
```

Clip `name` matches the glTF animation name (case-insensitive).

## Runtime wiring

```cpp
auto* recv = character->AddComponent<AnimationEventReceiverComponent>();
recv->ImportFromSkeleton(*skeleton);

auto* melee = character->AddComponent<AnimationMeleeHitComponent>();
melee->SetFacingObject(characterRoot);
melee->AddTarget(enemy);
```

`AnimationMeleeHitComponent` enables a forward hit trace between `active_start` and `active_end`.

## Demo

**Character camera demo** — press **F** to attack. Fox uses `Run` clip events from `Fox.spark-anim-events.json`; red training dummies take damage during the active window.

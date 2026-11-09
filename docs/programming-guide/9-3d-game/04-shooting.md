# Shooting and Tracers

Input-driven camera + **hitscan** against analytic spheres around targets. Visual feedback = short-lived emissive cube tracers (not simulated projectiles).

**Picking alternatives:** `Scene::RaycastPick`, `TryRaycastMeshWorld` — see `SceneEditor3DDemo`.

## Concept: camera + fire input

### C++

```cpp
void FpsGame::OnUpdate(const FrameTiming& timing, IEngineContext& context) {
    IInput& in = context.GetInput();
    if (in.IsCursorCaptured()) {
        camera.AddLook(in.GetMouseDeltaX(), in.GetMouseDeltaY());
        camera.ProcessMovement(in, timing.deltaTimeSeconds);
    }
    if (in.IsMouseButtonPressedThisFrame(0))
        TryShootTarget(context);
    UpdateTracerBullets(timing.deltaTimeSeconds);
    Game::OnUpdate(timing, context);
}
```

### C\#

```csharp
public override void OnUpdate(FrameTiming timing, IEngineContext context)
{
    var input = context.GetInput();
    if (input.IsCursorCaptured())
    {
        _camera.AddLook(input.MouseDeltaX, input.MouseDeltaY);
        _camera.ProcessMovement(input, timing.DeltaTimeSeconds);
    }
    if (input.IsMouseButtonPressedThisFrame(MouseButton.Left))
        TryShootTarget(context);
    Game.OnUpdate(timing, context);
}
```

### Python

```python
def on_update(self, timing, context):
    inp = context.get_input()
    if inp.is_cursor_captured() and inp.is_mouse_button_pressed_this_frame(0):
        self._try_shoot(context)
```

**F1** or right-click toggles cursor capture (`CharacterCameraDemo` pattern).

## Concept: hitscan ray vs sphere

World center from transform matrix; radius ~ scaled cube half-extent.

### C++

```cpp
#include "spark/scene/mesh/MeshRaycast.hpp"

void TryShootTarget(IEngineContext& context) {
    const Vector3 origin = camera.position;
    const Vector3 dir = camera.Forward().Normalized();
    ++shotsFired;

    float bestT = 1e9F;
    GameObject* best = nullptr;
    for (GameObject* t : targets) {
        TransformComponent* tr = t->GetComponent<TransformComponent>();
        if (!tr) continue;
        const Vector3 center = tr->GetWorldMatrix().GetTranslation();
        float hitT = 0.0F;
        if (TryRaycastSphereWorld(origin, dir, center, 0.6F, 1.0e-4F, bestT, hitT)) {
            if (hitT > 0.0F && hitT < bestT) {
                bestT = hitT;
                best = t;
            }
        }
    }
  // Optional: flash best target material / sound cue
    if (best) ++hits;
    SpawnTracerBullet(origin, dir);
}
```

### C\#

```csharp
var origin = _camera.Position;
var dir = _camera.Forward.Normalized();
foreach (var t in _targets)
{
    var tr = t.GetComponent<TransformComponent>();
    if (tr is null) continue;
    var center = tr.WorldMatrix.Translation;
    if (MeshRaycast.TryRaycastSphereWorld(origin, dir, center, 0.6f, 1e-4f, ref bestT, out var hitT))
    { /* track closest */ }
}
```

### Python

```python
# Raycast helpers live in native module today — call from C++ game or thin pybind when added
pass
```

## Concept: tracer entity

### C++

```cpp
void SpawnTracerBullet(const Vector3& origin, const Vector3& dirUnit) {
    GameObject* go = GetWorld().CreateGameObject();
    go->AddComponent<TransformComponent>()->SetTranslation(origin);
    go->AddComponent<MeshComponent>(unitCube, SceneMeshSlot::Custom, Vector3::One);
    if (MaterialComponent* mat = go->AddComponent<MaterialComponent>()) {
        mat->SetEmissive({1.0F, 0.9F, 0.4F}, 4.0F);
    }

    TracerBullet tb{};
    tb.go = go;
    tb.velocity = dirUnit * 48.0F;
    tb.timeLeft = 0.35F;
    tracers.PushBack(tb);
}
```

### C\#

```csharp
var go = World.CreateGameObject();
go.GetOrAddTransform().SetTranslation(origin);
go.AddMesh(SceneMeshSlot.Custom, Vector3.One, "fps/unit_cube");
go.AddMaterialLit(null).SetEmissive(new Vector3(1f, 0.9f, 0.4f), 4f);
_tracers.Add(new TracerBullet { Go = go, Velocity = dirUnit * 48f, TimeLeft = 0.35f });
```

## Concept: update tracers

### C++

```cpp
void UpdateTracerBullets(float dt) {
    for (std::size_t i = 0; i < tracers.GetSize();) {
        TracerBullet& tb = tracers[i];
        tb.timeLeft -= dt;
        if (tb.timeLeft <= 0.0F || tb.go == nullptr) {
            if (tb.go) GetWorld().DestroyGameObject(tb.go);
            tracers.EraseAt(i);
            continue;
        }
        TransformComponent* tr = tb.go->GetComponent<TransformComponent>();
        tr->SetTranslation(tr->GetLocalTransform().translation + tb.velocity * dt);
        ++i;
    }
}
```

### C\#

```csharp
tb.TimeLeft -= dt;
if (tb.TimeLeft <= 0f) { World.DestroyGameObject(tb.Go); _tracers.RemoveAt(i); }
```

Pair hits with `SoundCueComponent` on the camera for feedback ([Part 6](../6-sound/03-cues.md)).

Next: [Rendering the 3D Scene](05-rendering.md).

# Shooting and Tracers

## Camera Update

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

Press **F1** or right-click to toggle cursor capture (pattern from `CharacterCameraDemo`).

## Hitscan Ray vs Sphere

Use `TryRaycastSphereWorld` from `spark/scene/MeshRaycast.hpp`. World position comes from the transform's world matrix:

```cpp
#include "spark/scene/MeshRaycast.hpp"

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
    if (best) ++hits;
    SpawnTracerBullet(origin, dir);
}
```

For mesh-accurate picking, use `Scene::RaycastPick` or `TryRaycastMeshWorld` — see `SceneEditor3DDemo`.

## Tracer Entity

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

## Update Tracers

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

Next: [Rendering the 3D Scene](05-rendering.md).

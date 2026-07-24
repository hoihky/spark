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

## Hitscan Ray vs Sphere

```cpp
void TryShootTarget(IEngineContext& context) {
    const Vector3 origin = camera.position;
    const Vector3 dir = camera.Forward();
    ++shotsFired;

    float bestT = 1e9F;
    GameObject* best = nullptr;
    for (GameObject* t : targets) {
        Vector3 center = t->GetComponent<TransformComponent>()->GetWorldPosition();
        float hitT = RaySphereNearestT(origin, dir, center, 0.6F);
        if (hitT > 0.0F && hitT < bestT) { bestT = hitT; best = t; }
    }
    if (best) ++hits;
    SpawnTracerBullet(origin, dir);
}
```

## Tracer Entity

```cpp
void SpawnTracerBullet(const Vector3& origin, const Vector3& dirUnit) {
    GameObject* go = GetWorld().CreateGameObject();
    go->AddComponent<TransformComponent>()->SetTranslation(origin);
    go->AddComponent<MeshComponent>(unitCube, SceneMeshSlot::Custom, Vector3::One);
    auto* mat = go->AddComponent<MaterialComponent>(nullptr);
    mat->SetEmissive({1.0F, 0.9F, 0.4F});
    mat->SetEmissiveStrength(4.0F);

    TracerBullet tb{};
    tb.go = go;
    tb.velocity = dirUnit * 48.0F;
    tb.timeLeft = 0.35F;
    tracers.PushBack(tb);
}
```

Next: [Rendering the 3D Scene](05-rendering.md).

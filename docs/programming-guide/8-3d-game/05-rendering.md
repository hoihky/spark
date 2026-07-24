# Rendering the 3D Scene

## Full OnRender

```cpp
void FpsGame::OnRender(IRenderFrame&, IEngineContext& context) {
    int fbW = 0, fbH = 0;
    context.GetFramebufferSize(fbW, fbH);
    if (fbW <= 0) fbW = 1;
    if (fbH <= 0) fbH = 1;

    const float aspect = static_cast<float>(fbW) / static_cast<float>(fbH);
    const Matrix4 proj = Matrix4::PerspectiveVulkan(DegreesToRadians(72.0F), aspect, 0.1F, 200.0F);
    const Matrix4 view = camera.ViewMatrix();
    const Matrix4 viewProj = proj * view;

    Vector3 pr{}, pu{};
    camera.BillboardBasis(pr, pu);

    SubmitStandardLitSceneFromWorld(
        GetWorld(), context, viewProj, camera.position,
        Vector3{0.28F, -1.0F, 0.18F}.Normalized(),
        Vector3{1.0F, 0.97F, 0.92F}, 3.2F,
        Vector3{0.10F, 0.12F, 0.18F},
        true, pr, pu, sceneTimeSeconds,
        SceneSpriteSortMode::SortOrderOnly);
}
```

## HUD Overlay

```cpp
hudText->SetText(Utf8String(std::format(
    "FPS template | shots {} hits {} | LMB fire | WASD move | ESC quit",
    shotsFired, hits).c_str()));
```

`TextOverlayComponent` is collected by `FillStandardLitSceneFromWorld`.

Next: [Extending the FPS](06-extending.md).

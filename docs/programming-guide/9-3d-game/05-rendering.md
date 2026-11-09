# Rendering the 3D Scene

The FPS template uses the same **standard lit** path as other 3D demos: build Vulkan-style perspective + `FlyCamera` view, then fill `SceneRenderParams` from the world.

Deep dive: [3D lighting & submit](../3-3d-graphics/03-lighting.md) · [Cameras](../3-3d-graphics/02-cameras-3d.md).

## Concept: full `OnRender`

### C++

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

### C\#

```csharp
public override void OnRender(IRenderFrame frame, IEngineContext context)
{
    var (fbW, fbH) = context.GetFramebufferSize();
    var aspect = (float)fbW / fbH;
    var proj = Matrix4.PerspectiveVulkan(MathF.DegreesToRadians(72f), aspect, 0.1f, 200f);
    var view = _camera.ViewMatrix;
    SceneSubmit.SubmitStandardLitSceneFromWorld(
        World, context, proj * view, _camera.Position,
        sunDir, sunColor, 3.2f, ambient, enableSprites: true, sceneTime);
}
```

### Python

```python
# Default script host calls native OnRender when not overridden
def on_render(self, frame, context):
    pass
```

For **ECS camera** workflows, attach `CameraComponent` + `AudioListenerComponent` and use `SubmitStandardLitSceneFromWorldWithCamera` ([Part 7 — Camera](../7-game-components-usage/05-camera.md)).

## Concept: HUD overlay

### C++

```cpp
hudText->SetText(Utf8String(std::format(
    "FPS template | shots {} hits {} | LMB fire | WASD move | ESC quit",
    shotsFired, hits).c_str()));
```

### C\#

```csharp
hud.GetComponent<TextOverlayComponent>()?.SetText(
    $"shots {_shotsFired} hits {_hits}");
```

### Python

```python
hud.get_component_text_overlay().set_text(f"shots {self._shots}")
```

`TextOverlayComponent` is collected during `FillStandardLitSceneFromWorld`.

## Shadows and particles (next steps)

| Upgrade | Where |
|---------|--------|
| Shadowed sun | `DirectionalLightComponent` + cascade settings — [Shadows](../3-3d-graphics/04-shadows.md) |
| Muzzle flash | `ParticleEmitterComponent` on weapon socket |
| Decal impacts | `DecalProjectorComponent` at hit point |

Next: [Extending the FPS](06-extending.md).

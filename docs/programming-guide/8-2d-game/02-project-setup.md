# Project Setup

Fork the **platformer pattern** from SparkDemo into your own `SparkEngine` target, or run from **spark-sharp** / **spark-python** hosts with the same frame contract.

## Concept: CMake game target

### C++

```cmake
add_executable(MyPlatformer src/main.cpp src/Platformer2DGame.cpp)
target_link_libraries(MyPlatformer PRIVATE SparkEngine SparkGameplay)
target_compile_features(MyPlatformer PRIVATE cxx_std_23)
```

Link `SparkGameplay` when using `GameplayKit`, combat helpers, or inventory hooks.

### Shell (spark-sharp)

```bash
cd ../spark-sharp
cmake --preset debug -DSPARK_ENGINE_ROOT=/path/to/spark
cmake --build build --target SparkRunHelloCsGameDebug
```

### Python (install)

```bash
# spark-python README: install -e . with SPARK_ENGINE_ROOT
python -m spark samples.hello_game
```

## Concept: bootstrap in `OnAttach`

### C++

```cpp
void OnAttach(IEngineContext& context) override {
    MountUiFont(GetWorld());  // TextOverlayComponent
    physics.GetWorld2D().GetSettings().gravityY = -32.0F;

    auto* inputMap = player->AddComponent<InputActionMapComponent>();
    inputMap->AddAction("Jump", {GLFW_KEY_SPACE});
    inputMap->AddAction("Move", InputActionType::Axis2D);
    player->AddComponent<PlayerInputComponent>()->SetActionMap(inputMap);

    if (SoundEngine* se = context.TryGetSoundEngine())
        se->SetBackgroundMusic(LoadBgm(), 0.3F, true);
}
```

### C\#

```csharp
public override void OnAttach(IEngineContext context)
{
    World.MountUiFont();
    _physics.GetWorld2D().GetSettings().GravityY = -32f;
}
```

### Python

```python
def on_attach(self, context):
    world = context.try_get_scene().get_world()
    camera = world.create_game_object("Camera")
    camera.add_camera_2d(6.5, priority=10)
```

## Concept: render submit

### C++

```cpp
void OnRender(IRenderFrame&, IEngineContext& ctx) override {
    SubmitStandardLitSceneFromWorldWithCamera(
        GetWorld(), ctx,
        sunDir, sunColor, 1.0F, ambient,
        enableParticles: true, sceneTime);
}
```

Requires `Camera2DComponent` on scene. See [2D render pipeline](../2-2d-graphics/06-2d-render-pipeline.md).

### C\#

```csharp
SceneSubmit.SubmitStandardLitSceneFromWorldWithCamera(World, ctx, ...);
```

### Python

Native host submits after `on_update` when using default script template.

## Asset roots

Runtime content resolves via `SPARK_BUILD_ASSETS_DIR` — copy Kenney packs or use procedural textures like demo fallback.

### C++

```cpp
#include "spark/config.hpp"
Utf8String(SPARK_BUILD_ASSETS_DIR).AppendUtf8("/sprites/...");
```

### C\#

```csharp
// Relative keys under assets/ in interop loaders
```

### Python

```python
world.register_checkerboard_texture("game/tiles", c0, c1)
```

Next: [Level Design](03-level-design.md).

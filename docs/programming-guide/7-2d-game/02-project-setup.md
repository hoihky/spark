# Project Setup

## Start from SparkDemo Patterns

Create a new executable in the Spark repo (or link against a built `SparkEngine`):

```cmake
add_executable(Platformer2D src/main.cpp src/Platformer2DGame.cpp)
target_link_libraries(Platformer2D PRIVATE SparkEngine)
target_compile_features(Platformer2D PRIVATE cxx_std_23)
```

Copy structure from `Platformer2DDemo` — split logic into `Load`, `Simulate`, and `Render` methods, or inline them in `OnAttach` / `OnUpdate` / `OnRender`.

## `OnAttach` Checklist

```cpp
void Platformer2DGame::OnAttach(IEngineContext& context) override {
    MountUiFont(GetWorld());  // or MountUiFontIfNeeded helper below

    physics.GetWorld2D().GetSettings().gravityY = -32.0F;
    physics.GetWorld2D().GetSettings().maxFallSpeed = 46.0F;

    LoadLevel(GetWorld());
    SpawnPlayer(GetWorld());
}
```

## Font Mounting

UI text requires `GameWorld::SetUiFont`:

```cpp
void MountUiFontIfNeeded(GameWorld& world) {
    if (world.GetUiFont()) return;
    auto uiFont = MakeShared<Font>();
    if (uiFont->TryLoadTrueTypeFromFile(SPARK_UI_FONT_PATH, 42.0F))
        world.SetUiFont(uiFont);
}
```

Called from `OnAttach` before creating `TextOverlayComponent`. `Platformer2DDemo` calls `MountUiFont` from `spark/demo/DemoFoundation.hpp`.

## Texture Registration

```cpp
auto tex = MakeShared<Texture2D>(Utf8String("Tiles"));
*tex = Texture2D::CreateCheckerboard(256, 32, colorA, colorB);
world.RegisterTexture(tex, "my_game/tiles");

go->AddComponent<SpriteComponent>(tex, Vector4::One, uvRect, sortOrder);
```

## Track Roots for Cleanup

```cpp
void OnDetach() override {
    for (GameObject* go : roots)
        if (go) GetWorld().DestroyGameObject(go);
    roots.Clear();
}
```

Or use `DemoRootCollection` from `spark/demo/DemoFoundation.hpp`.

## Frame Order

```cpp
void OnUpdate(const FrameTiming& t, IEngineContext& ctx) override {
    HandlePlayerInput(t, ctx);
    Game::OnUpdate(t, ctx);       // component ticks + sound cues
    physics.Simulate2D(GetWorld(), t);
    UpdateEnemies(t);
}

void OnRender(IRenderFrame&, IEngineContext& ctx) override {
    SubmitStandardLitSceneFromWorldWithCamera(
        GetWorld(), ctx, sunDir, sunColor, sunIntensity, ambient,
        false, sceneTime);
}
```

Next: [Building the Level](03-level-design.md).

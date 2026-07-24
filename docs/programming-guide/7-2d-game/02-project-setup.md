# Project Setup

## Copy Template

```bash
cp -r samples/platformer2d_game_template my_platformer
```

## CMakeLists.txt Pattern

```cmake
add_executable(Platformer2D src/main.cpp src/Platformer2DGame.cpp)
target_link_libraries(Platformer2D PRIVATE SparkEngine)
target_compile_features(Platformer2D PRIVATE cxx_std_23)
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

Called from `OnAttach` before creating `TextOverlayComponent`.

## Track Roots for Cleanup

```cpp
void OnDetach() override {
    for (GameObject* go : roots)
        if (go) GetWorld().DestroyGameObject(go);
    roots.Clear();
}
```

Next: [Building the Level](03-level-design.md).

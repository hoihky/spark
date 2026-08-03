# Spark FPS game template

Separate CMake project (under `samples/fps_game_template`) that links **Spark** as a **shared** `SparkEngine` library. It is a small **first-person** starting point: fly camera, ground plane, red target cubes, **LMB** fires a **visible emissive tracer** plus instant **hitscan** on targets, HUD text, **F1** mouse lock, **ESC** quit.

## Layout

```
samples/fps_game_template/
  CMakeLists.txt
  README.md
  src/
    main.cpp
    FpsGame.hpp
    FpsGame.cpp
```

When this folder sits in the Spark tree at `samples/fps_game_template`, the default `SPARK_ROOT` is `../..` (the engine repository root). If you copy the folder elsewhere, pass:

```bash
cmake -S . -B build -DSPARK_ROOT=/absolute/path/to/spark
```

## Configure and build

```bash
cd samples/fps_game_template
cmake -S . -B build
cmake --build build
```

Same engine cache toggles as `game_template`: shared engine, no `SparkDemo` by default.

## Controls

| Input | Action |
|--------|--------|
| **WASD** | Move (fly) |
| **Space / Shift** | Up / down |
| **Mouse** | Look (when cursor captured) |
| **LMB** | Spawn tracer bullet + hitscan nearest target along view ray |
| **F1** | Toggle cursor capture |
| **ESC** | Close window |

## Next steps for a real FPS

Add **physics** (`PhysicsSubsystem::Simulate3D`, colliders), **weapon models**, **projectiles** that damage via overlap instead of the template’s cosmetic tracers + separate hitscan, **AI**, **audio**, and asset loading under your own content paths. Keep `SubmitStandardLitSceneFromWorld` or switch to a custom `SceneRenderParams` build if you need more control.

## Vulkan SDK

The engine’s CMake step requires **glslangValidator** from the [Vulkan SDK](https://vulkan.lunarg.com/).

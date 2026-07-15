# Spark 2D platformer game template

Separate CMake project under `samples/platformer2d_game_template` that links **Spark** as a **shared** `SparkEngine` library. It is a minimal **side-scrolling platformer**: procedural checker **tile** texture, a few **static** platforms + **dynamic** player (`Rigidbody2D` + `BoxCollider2D`), **`SimulatePhysics2D`**, **`Camera2D`**, and **`SubmitStandardLitSceneFromWorld`** with **`SortOrderThenWorldY`** for sprite depth.

## Layout

```
samples/platformer2d_game_template/
  CMakeLists.txt
  README.md
  src/
    main.cpp
    Platformer2DGame.hpp
    Platformer2DGame.cpp
```

Default **`SPARK_ROOT`** is **`../..`** when this folder lives at `spark/samples/platformer2d_game_template`. If you move the template, configure with:

```bash
cmake -S . -B build -DSPARK_ROOT=/absolute/path/to/spark
```

## Build

```bash
cd samples/platformer2d_game_template
cmake -S . -B build
cmake --build build
```

Same engine options as the other templates: shared `SparkEngine`, no `SparkDemo` by default.

## Controls

| Input | Action |
|--------|--------|
| **A / Left** | Move left |
| **D / Right** | Move right |
| **Space** | Jump (when grounded) |
| **ESC** | Quit |

Walk to **x ≥ 15.5** to set the simple **goal** flag (shown in the HUD). Falling below **y = -6.5** respawns at the start.

## Next steps

Swap the checker texture for **Kenney** or your own atlases, add **tilemaps**, **collectibles**, **enemies**, and **audio**—see the in-tree `Platformer2DDemo` for a richer reference.

## Vulkan SDK

Spark’s CMake step expects **glslangValidator** from the [Vulkan SDK](https://vulkan.lunarg.com/).

# Building and Running

## Requirements

| Tool | Version |
|------|---------|
| CMake | ≥ 3.28 |
| C++ | C++23 |
| Vulkan SDK | For shader compilation at build time |
| Network | First configure downloads GLFW, fonts, glTF samples |

## Configure & Build

```bash
cd /path/to/spark
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/SparkDemo
```

## CMake Options (selected)

| Option | Default | Effect |
|--------|---------|--------|
| `SPARK_BUILD_DEMO` | ON | Builds `SparkDemo` launcher |
| `SPARK_ENABLE_IMGUI` | ON | Dear ImGui (docking) + Vulkan/GLFW backends for tool UI |
| `SPARK_BUILD_GAMEPLAY` | ON | Optional `SparkGameplay` kit (combat, pickups, game flow); demos link it |
| C# / interop | — | **[spark-sharp](../../SPARK_SHARP.md)** sibling repo (`SparkInterop`, bindings, `SparkScriptHost`) |
| `SPARK_BUILD_TESTS` | OFF | Unit tests |

## Generated Config

`build/include/spark/config.hpp` defines:

- `SPARK_BUILD_ASSETS_DIR` — runtime asset root
- `SPARK_UI_FONT_PATH` — bundled Roboto paths

```cpp
#include "spark/config.hpp"
Utf8String path(SPARK_BUILD_ASSETS_DIR);
path.AppendUtf8("/fonts/Roboto-Regular.ttf");
```

## Your Own Target

```cmake
add_executable(MyGame src/main.cpp src/MyGame.cpp)
target_link_libraries(MyGame PRIVATE SparkEngine)
target_compile_features(MyGame PRIVATE cxx_std_23)
```

Copy the patterns in `src/spark/demo/` as a starting point — especially `Platformer2DDemo` for 2D and `CharacterCameraDemo` / `PhysicsBallThrow3DDemo` for 3D. Link your own executable against `SparkEngine` as shown above.

## SparkDemo built-in scenes (selected)

| # | Scene | Highlights |
|---|--------|------------|
| 6 | 2D platformer | Kenney tiles, character FSM |
| 12 | Match-3 | `Camera2D` + grid pick |
| **19** | **Tilemap showcase** | Layers, autotile, animation, gameplay grid, pathfinding; **L** loads Kenney `sampleMap.tmx`, **R** resets procedural map |
| 20 | Dear ImGui | Docking tool UI (`SPARK_ENABLE_IMGUI`) |
| **29** | **Physics 3D showcase** | `PhysicsShowcase3DDemo` — CC, carry/throw, CCD shots, unified raycast, triggers, fixed step, replay (**P**) |

Full list: `DemoCatalog` in `include/spark/demo/DemoCatalog.hpp` and the `SparkShellDemo` menu.

Next: [The Engine Loop](04-engine-loop.md).

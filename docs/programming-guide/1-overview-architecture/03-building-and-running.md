---
title: Building and Running
order: 3
---

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
| `SPARK_BUILD_SCRIPT_HOST` | OFF | CoreCLR C# scripting host |
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

Copy `game_template/` or `samples/platformer2d_game_template/` as a starting point.

Next: [The Engine Loop](overview-architecture/04-engine-loop.html).

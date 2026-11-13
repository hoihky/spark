# Building and Running

Spark builds as a **CMake superproject**: the engine library (`SparkEngine`), optional tests, and **SparkDemo** launcher. C\# and Python hosts live in sibling repositories but consume the same installed headers and `SPARK_BUILD_ASSETS_DIR`.

## Requirements

| Tool | Version |
|------|---------|
| CMake | ≥ 3.28 |
| C++ | C++23 |
| Vulkan SDK | Shader compilation at configure/build |
| Network | First configure may fetch GLFW, fonts, glTF samples |

macOS: MoltenVK via the Vulkan SDK. Linux/Windows: install the LunarG SDK and ensure `glslc` is on `PATH`.

## Concept: build the engine + SparkDemo

### Shell (all platforms)

```bash
cd /path/to/spark
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/SparkDemo
```

Debug iteration:

```bash
cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug -j
./build-debug/SparkDemo
```

### C++

Your game target links `SparkEngine` (see below). No separate step beyond CMake.

### C\#

From **spark-sharp** (engine path required):

```bash
cd ../spark-sharp
cmake --preset debug -DSPARK_ENGINE_ROOT="$(cd ../spark && pwd)"
cmake --build build --target SparkInterop SparkScriptHost SparkRunHelloCsGameDebug
```

Details: [SPARK_SHARP.md](../../../SPARK_SHARP.md), spark-sharp `README.md`.

### Python

Build **spark** first, then **spark-python** against the same `SPARK_ENGINE_ROOT`. Run samples:

```bash
cd ../spark-python
# follow spark-python README for cmake preset + pip install -e .
python -m spark samples.hello_game
```

## CMake options (selected)

| Option | Default | Effect |
|--------|---------|--------|
| `SPARK_BUILD_DEMO` | ON | Builds `SparkDemo` launcher |
| `SPARK_ENABLE_IMGUI` | ON | Dear ImGui + Vulkan/GLFW backends |
| `SPARK_BUILD_GAMEPLAY` | ON | `SparkGameplay` kit linked by demos |
| `SPARK_BUILD_TESTS` | OFF | Unit tests (`ctest`) |

Disable ImGui for shipping builds without tools:

```bash
cmake -S . -B build -DSPARK_ENABLE_IMGUI=OFF
```

## Generated config

`build/include/spark/config.hpp` defines runtime paths:

- `SPARK_BUILD_ASSETS_DIR` — content root at run time
- `SPARK_UI_FONT_PATH` — bundled Roboto paths
- `SPARK_ENABLE_IMGUI` — `0` or `1`

### C++

```cpp
#include "spark/config.hpp"
Utf8String path(SPARK_BUILD_ASSETS_DIR);
path.AppendUtf8("/fonts/Roboto-Regular.ttf");
```

### C\#

```csharp
// SparkScriptHost sets asset roots from native bootstrap; use relative keys under assets/.
world.Assets.RegisterTextureFromFile("ui/icon", "assets/sprites/icon.png");
```

### Python

```python
# Paths resolve relative to engine asset dir when using keys like "sprites/foo.png"
world.load_texture("assets/sprites/player.png", key="game/player")
```

## Concept: your own C++ executable

```cmake
add_executable(MyGame src/main.cpp src/MyGame.cpp)
target_link_libraries(MyGame PRIVATE SparkEngine)
target_compile_features(MyGame PRIVATE cxx_std_23)
```

Mirror `src/spark/demo/Platformer2DDemo` for 2D or `CharacterCameraDemo` for 3D — both show correct physics order and render submit.

### C++

```cpp
#include "spark/engine/Engine.hpp"

int main() {
    Spark::Engine engine(Spark::Engine::NewGame<MyGame>());
    engine.Run();
    return 0;
}
```

### C\#

Managed entry is `SparkScriptHost` loading your `Game` subclass — see spark-sharp template project.

### Python

```python
from spark.host import run_game
from my_game import MyGame

run_game(MyGame())
```

## Regenerate script bindings

After adding or changing components in the engine, refresh the catalog in **spark-sharp**:

```bash
export SPARK_ENGINE_ROOT=/path/to/spark
cd /path/to/spark-sharp
./tools/generate-csharp-bindings.sh
```

Python interop is regenerated from the same catalog in **spark-python** (see its README).

## SparkDemo built-in scenes (selected)

| # | Scene | Highlights |
|---|--------|------------|
| 6 | 2D platformer | Kenney tiles, character FSM, semantic input |
| 12 | Match-3 | `Camera2D` + grid pick |
| 19 | Tilemap showcase | Layers, autotile, pathfinding, TMX **L** |
| 20 | Dear ImGui | Docking tool UI |
| 21 | glTF PBR | `DamagedHelmet.glb`, HDR IBL |
| 29 | Physics 3D | CC, CCD, triggers, replay **P** |

Full list: `DemoCatalog` in `include/spark/demo/DemoCatalog.hpp`.

## Shell shortcuts (global in SparkDemo)

| Key | Action |
|-----|--------|
| **TAB** | Return to launcher |
| **H** | Toggle help overlay |
| **F3** | Toggle FPS overlay |
| **F9** | Video recording |
| **F12** | PNG screenshot to `assets/screenshots/` |

Details: [Debugging and diagnostics](14-debugging-and-diagnostics.md).

## Troubleshooting

| Issue | Fix |
|-------|-----|
| Missing assets at runtime | Run from build dir or set cwd so `SPARK_BUILD_ASSETS_DIR` resolves |
| Vulkan instance fail | Install SDK; update GPU drivers |
| C\# host can't find engine | Pass `-DSPARK_ENGINE_ROOT` at spark-sharp configure |
| Script method missing | Regenerate bindings; confirm component in catalog |

Next: [The Engine Loop](04-engine-loop.md).

# Spark empty game (template)

Minimal C++ executable that links the **Spark** engine as a **shared library** (`SparkEngine`). Use it as a starting point for a game that lives in its own CMake project.

## Layout

- Copy **`game_template/`** next to your Spark clone (sibling folders), **or** keep it at `<spark>/game_template` and point `SPARK_ROOT` at the engine root.
- Your sources stay under `src/`; you do not modify the engine tree for normal gameplay code.

## Configure and build

From this directory:

```bash
cmake -S . -B build
cmake --build build
```

If the template is **not** inside `<spark>/game_template`, pass the engine path:

```bash
cmake -S . -B build -DSPARK_ROOT=/absolute/path/to/spark
```

The first configure sets engine cache options for this tree:

- `SPARK_BUILD_ENGINE_SHARED=ON` — builds `SparkEngine` as `.so` / `.dylib` / `.dll`
- `SPARK_BUILD_DEMO=OFF` — skips `SparkDemo`

## Run

- **macOS / Linux:** run the executable from the build tree; `BUILD_RPATH` points at the nested `spark_engine` output where `libSparkEngine` is produced.
- **Windows:** the post-build step copies `SparkEngine.dll` next to your `.exe`. You still need a Vulkan-capable GPU and the usual Vulkan loader/runtime on the PATH.

## Vulkan SDK

Spark’s CMake expects **glslangValidator** (Vulkan SDK) when configuring the engine. Install the [Vulkan SDK](https://vulkan.lunarg.com/) and ensure `VULKAN_SDK` / `PATH` are set before running CMake.

## Renaming the project

Change the `project(...)` line in `CMakeLists.txt` and keep `src/main.cpp` as your entry point (or add more sources to the `add_executable` call).

## Engine documentation

See the Spark repo [`README.md`](../README.md) and [`docs/`](../docs/README.md) for architecture, rendering, and editor roadmaps.

# CLion run configurations

## CMake profiles

| Profile | Build dir | Targets |
|---------|-----------|---------|
| **debug** | `cmake-build-debug/` | SparkDemo, SparkEditor, SparkScriptHost (with `SPARK_BUILD_SCRIPT_HOST`) |
| **editor-debug** | `cmake-build-editor/` | SparkEditor + shared SparkEngine (slim, no demos) |

Enable presets: **Settings → CMake → Load CMake presets**. Repo also ships `.idea/cmake.xml` with both profiles.

If **editor-debug** is missing: **Tools → CMake → Reload CMake Project**, or re-open the project.

## SparkEditor

Runs the Spark game editor (`spark_editor/`).

| Run configuration | CMake profile | Build directory |
|-------------------|---------------|-----------------|
| **SparkEditor** | `Debug` (default) | `cmake-build-debug/` |
| **SparkEditor (editor-debug)** | `editor-debug` preset | `cmake-build-editor/` |

**Red cross on the run config?** CLion cannot resolve the executable for the active CMake profile:

1. **Tools → CMake → Reload CMake Project**
2. Build **SparkEditor** once (CMake tool window)
3. **Run → Edit Configurations → SparkEditor** — **Target** = `SparkEditor`, **CMake profile** must match (`Debug` vs `editor-debug`)
4. Or delete the broken config and **+ → CMake Application**, pick **SparkEditor** from the target dropdown

For **editor-debug** (shared `libSparkEngine.dylib`), use **SparkEditor (editor-debug)**.

Executable: `cmake-build-debug/spark_editor/SparkEditor` or `cmake-build-editor/spark_editor/SparkEditor`.

## SparkScriptHost (HelloCsGame)

Requires **`-DSPARK_BUILD_INTEROP=ON`** and **`-DSPARK_BUILD_SCRIPT_HOST=ON`** (both ON in the `debug` preset).

1. Reload CMake, build target **SparkScriptHost** (pulls **SparkScriptingBuild** → HelloCsGame Release).
2. Run **SparkScriptHost (HelloCsGame)** from the run configuration dropdown.
3. Working directory: repo root (for `assets/`). `DYLD_LIBRARY_PATH` must include `cmake-build-debug/scripting` (set in `.run/SparkScriptHost.run.xml`).

Manual:

```bash
cmake -B cmake-build-debug -DSPARK_BUILD_INTEROP=ON -DSPARK_BUILD_SCRIPT_HOST=ON
cmake --build cmake-build-debug --target SparkScriptHost
DYLD_LIBRARY_PATH=cmake-build-debug/scripting ./cmake-build-debug/scripting/SparkScriptHost
```

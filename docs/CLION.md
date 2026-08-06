# CLion — CMake profiles

## Profiles (from `CMakePresets.json`)

| Profile | Build directory | Use for |
|---------|-----------------|---------|
| **debug** | `cmake-build-debug/` | SparkDemo + SparkEditor (default dev) |
| **editor-debug** | `cmake-build-editor/` | SparkEditor only — shared `libSparkEngine.dylib`, no demos |

## Enable `editor-debug` in CLion

1. **Settings → Build, Execution, Deployment → CMake**
2. Enable **Load CMake presets from CMakePresets.json** (if not already on)
3. You should see **editor-debug** in the profile list (also seeded via `.idea/cmake.xml`)
4. **Tools → CMake → Reload CMake Project**
5. Select profile **editor-debug** in the CMake tool window, build **SparkEditor**
6. Run **SparkEditor (editor-debug)** from `.run/SparkEditor.editor-debug.run.xml`

Executable: `cmake-build-editor/spark_editor/SparkEditor`

`DYLD_LIBRARY_PATH` must include `cmake-build-editor/` (set in the run config).

## Manual CMake options (editor-debug)

If not using presets, create a profile with build directory `cmake-build-editor` and options:

```
-DSPARK_BUILD_ENGINE_SHARED=ON
-DSPARK_BUILD_ENGINE_DEMOS=OFF
-DSPARK_BUILD_EDITOR=ON
-DSPARK_BUILD_SPARK_EDITOR=ON
-DSPARK_BUILD_DEMO=OFF
```

See also [`.run/README.md`](../.run/README.md).

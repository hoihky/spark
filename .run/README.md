# CLion run configurations

## SparkScriptHost (HelloCsGame)

Runs the native CoreCLR host with the **HelloCsGame** sample.

### CMake error: `Microsoft.NETCore.App.Host pack not found`

Older `DotNetHost.cmake` used the SDK path from `dotnet --info` instead of `<dotnet-root>/packs`. That is fixed — **Reload CMake** after pulling.

You still need the **.NET SDK** installed (`dotnet` on PATH). On macOS ARM, the pack is:

`$DOTNET_ROOT/packs/Microsoft.NETCore.App.Host.osx-arm64/<version>/runtimes/osx-arm64/native/`

### If Target / Executable show “Not found”

CMake has not created the `SparkScriptHost` target yet. This usually means **`SPARK_BUILD_SCRIPT_HOST` is OFF** in your CMake cache (common after upgrading from an older default).

**Fix (pick one):**

1. **CMake cache (CLion)**  
   - **Settings → CMake → your profile → CMake options**  
   - Add: `-DSPARK_BUILD_SCRIPT_HOST=ON`  
   - **Reload CMake Project**

2. **CMake cache editor**  
   - Open `cmake-build-debug/CMakeCache.txt`  
   - Set `SPARK_BUILD_SCRIPT_HOST:BOOL=ON`  
   - **Reload CMake Project**

3. **CMake Presets** (repo includes `CMakePresets.json`)  
   - Use preset **debug** or enable **CMake Presets** in CLion and reconfigure.

After reload, **SparkScriptHost** and **SparkInterop** should appear under CMake targets. The run configuration’s **Target** and **Executable** fields will then resolve.

### First run (SparkScriptHost)

1. Build target **SparkScriptHost** (depends on **SparkScriptingBuild**, which runs `dotnet build` for HelloCsGame).
2. Confirm this file exists:  
   `scripting/samples/HelloCsGame/bin/Release/net8.0/HelloCsGame.runtimeconfig.json`  
   If only `HelloCsGame.dll` is present, rebuild HelloCsGame — the csproj sets `EnableDynamicLoading` so the native host gets a `.runtimeconfig.json`.
3. Run **SparkScriptHost (HelloCsGame)**. Program args use `$PROJECT_DIR$/...` so paths work even when the executable lives under `cmake-build-debug/`. **Working directory** must stay **repo root** (for `assets/`).

## CMake profiles

| Profile | Build dir | Targets |
|---------|-----------|---------|
| **debug** | `cmake-build-debug/` | SparkDemo, SparkEditor, SparkScriptHost |
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


### `runtimeconfig.json does not exist` / `hostfxr_initialize_for_runtime_config failed`

| Cause | Fix |
|--------|-----|
| HelloCsGame not built, or built before `EnableDynamicLoading` | `dotnet build scripting/samples/HelloCsGame/HelloCsGame.csproj -c Release` or build **SparkScriptingBuild** |
| Wrong working directory (relative paths miss the repo) | Set **Working directory** to `$PROJECT_DIR$` in the run config (already set in `.run/SparkScriptHost.run.xml`) |
| Stale CLion args without `$PROJECT_DIR$` | Re-import or use absolute paths; **SparkScriptHost** also walks up from cwd to find the repo root |

You can run with no arguments from the repo root; defaults point at HelloCsGame under `scripting/samples/...`.

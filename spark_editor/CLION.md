# Building Spark Editor in CLion

Two ways to get the **SparkEditor** target in CLion.

---

## Option A — Open `spark_editor/` (editor-only project)

1. **File → Open** → select the **`spark_editor`** folder (not the parent `spark/` repo).
2. **Tools → CMake → Reload CMake Project**.
3. In the **CMake** tool window you should see **`SparkEngine`** and **`SparkEditor`**.
4. Build / run **SparkEditor** (or use `.run/SparkEditor.run.xml`).

`SPARK_ROOT` defaults to `../` (engine repo).

### Generator mismatch

If CMake says the build folder used **Unix Makefiles** but CLion uses **Ninja**:

```bash
rm -rf cmake-build-debug
```

Then **Reload CMake Project**.

---

## Option B — Open main `spark/` repo (recommended if you work on engine + editor)

The default profile only builds **SparkDemo**. Enable the editor preset or CMake flags:

### Using CMake Presets (easiest)

1. Open the **`spark`** repo root in CLion.
2. **Settings → CMake** → enable **CMake Presets**.
3. Select preset **`editor-debug`** (build dir: `cmake-build-editor`).
4. **Reload CMake Project** → **`SparkEditor`** appears under targets.
5. Run **SparkEditor** from `.run/SparkEditor.run.xml`.

### Manual CMake options

**Settings → CMake → your profile → CMake options:**

```
-DSPARK_BUILD_ENGINE_SHARED=ON
-DSPARK_BUILD_ENGINE_DEMOS=OFF
-DSPARK_BUILD_EDITOR=ON
-DSPARK_BUILD_SPARK_EDITOR=ON
-DSPARK_BUILD_DEMO=OFF
-DSPARK_BUILD_SCRIPT_HOST=OFF
```

Reload CMake, then build target **SparkEditor**.

Executable: `cmake-build-*/spark_editor/SparkEditor` (embedded) or `cmake-build-debug/SparkEditor` (standalone).

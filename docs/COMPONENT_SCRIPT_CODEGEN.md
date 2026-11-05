# Component script bindings — architecture

> **Repository split:** Interop implementation, generated bindings, and all tools live in **[spark-sharp](SPARK_SHARP.md)**. Scriptable component APIs are declared in **spark-sharp** `scripting/bindings/component-script-catalog.json`. Engine `*Component.hpp` files hold the C++ implementations.

C# cannot call C++ `GameComponent` methods directly. The pipeline treats **`native/include/spark/scripting/SparkInterop.h`** (in **spark-sharp**) as the scriptable contract (stable C ABI). The **component script catalog** is the source of truth for which methods get C thunks; C++ headers must implement those members.

```mermaid
flowchart TB
  subgraph cpp [C++ engine + spark-sharp native]
    HPP["spark/include *Component.hpp"]
    Catalog["component-script-catalog.json"]
    InteropH["SparkInterop.h spark_* exports"]
    InteropCpp["SparkInterop*.cpp thunks"]
    Catalog -->|"spark_script_bindgen.py"| BindGen["SparkInteropComponentBindings.generated.*"]
    HPP -->|"validate + member lookup"| Catalog
    BindGen --> InteropCpp
    HPP -->|"manual thunks (asset keys, Add* factories)"| InteropCpp
    InteropCpp --> InteropH
  end
  subgraph gen [.NET in spark-sharp]
    Clang["ClangSharpPInvokeGenerator"]
    Mirror["ComponentMirrorCodegen"]
    Reg["generate-csharp-component-registry.py"]
    PInvoke["generate_native_pinvoke_companion.py"]
    Clang --> Native["Native.g.cs"]
    PInvoke --> Companion["Native.Interop.g.cs"]
    InteropH --> Mirror
    Mirror --> CS["Components/FromInterop/*.g.cs"]
    Reg --> Access["GameObject.ComponentAccess.g.cs"]
    Reg --> Factory["SparkInteropComponentFactory.generated.cpp"]
  end
  subgraph scan [Coverage]
    Scan["scan-cpp-component-api.py"]
    HPP --> Scan
    Scan --> Cov["binding-coverage.json"]
    InteropH --> Cov
  end
  Native --> Bindings["Spark.Bindings"]
  Companion --> Bindings
  CS --> Bindings
  Access --> Bindings
```

## Layers

| Layer | Artifact | Role |
|-------|----------|------|
| 1 | `SparkInterop.h` + `SparkInterop*.cpp` | C ABI mirroring scriptable C++ surface (**spark-sharp** `native/`) |
| 1b | `SparkInteropComponentBindings.generated.h/.cpp` | Auto thunks from **`component-script-catalog.json`** |
| 2 | `Native.g.cs` + `Native.Interop.g.cs` | `DllImport` declarations (ClangSharp + companion) |
| 3 | `Components/FromInterop/*.g.cs` | Usable C# types from `spark_<prefix>_*` grouping |
| 4 | `Components/Generated/*` | Handle-only stubs when **no** `spark_<prefix>_` group exists yet |
| 5 | Generated glue | `GameObject.ComponentAccess.g.cs`, `SparkComponentKind.g.cs`, C++ default-add factory |

**Ergonomics (hand-maintained):** `CppMirrors.g.cs` (`Game`, `GameWorld`, `GameObject` `Add*` helpers), `InteropPtr.cs`, `InteropUtf8.cs`, small `*.Extensions.g.cs` files for enums/overloads.

## Naming convention

Interop functions use **`spark_<componentPrefix>_<verb>`**:

- `spark_transform_get_translation` / `spark_transform_set_translation` → `TransformComponent.Translation`
- `spark_sound_cue_queue_bundled_clip` → `SoundCueComponent.QueueBundledClip` (UTF-8 path)
- Irregular prefixes live in `scripting/bindings/spark-interop-bindings.json` (**spark-sharp**, from `generate_interop_manifest.py`).

**Creation vs get-or-add:** Some kinds are denied in `generate-csharp-component-registry.py` (`FACTORY_DENY`) because C++ needs extra constructor args. Expose **`spark_object_add_*`** in `SparkInterop.h` and mirror as `GameObject.AddTerrain()`, `AddSoundCue()`, etc. in `CppMirrors.g.cs`.

## Regenerate (full stack)

From the **spark-sharp** repository root:

```bash
export SPARK_ENGINE_ROOT=/path/to/spark
./tools/generate-csharp-bindings.sh
```

This runs, in order:

1. `validate_component_script_catalog.py` → `spark_script_bindgen.py`
2. `generate_interop_manifest.py` → `spark-interop-bindings.json`
3. `dotnet run` → **Spark.Bindings.Generator** (ClangSharp + mirrors)
4. `sync-component-kind-bindings.sh`
5. `scan-cpp-component-api.py` → `binding-coverage.json`
6. `generate_native_pinvoke_companion.py`
7. `generate-csharp-component-registry.py`

See **spark-sharp** `scripting/bindings/COMPONENT_SCRIPT_CATALOG.md` for catalog editing.

CMake also runs `spark_script_bindgen.py` when building **`SparkInterop`** in spark-sharp (`SparkInteropBindingsCodegen` target).

## Closing gaps (C++ → interop)

1. Run `python3 tools/scan-cpp-component-api.py` (spark-sharp) and open `scripting/bindings/binding-coverage.json`.
2. Add the method to **`component-script-catalog.json`** (suffix + C++ `member` name), implement it on the component in **this** repo, then validate and bindgen (or `./tools/generate-csharp-bindings.sh` in spark-sharp).
3. Ensure new symbols are not duplicated in manual `SparkInterop.h` (bindgen skips existing names).
4. Re-run `./tools/generate-csharp-bindings.sh` in **spark-sharp**.

`skipMirrorCodegenClasses` in `generate_interop_manifest.py` is for APIs that need custom C# (e.g. `AnimationEventReceiverComponent` callbacks).

## Managed host (optional)

Build **`SparkScriptHost`** from **spark-sharp** (`SPARK_SHARP_BUILD_SCRIPT_HOST=ON`). It loads a game `.dll` and registers managed `Game` callbacks via `ScriptHostEntry` (`Spark.Scripting`). See **[CSHARP_SCRIPTING.md](CSHARP_SCRIPTING.md)** and **[SPARK_SHARP.md](SPARK_SHARP.md)**.

## Related docs

| Doc | Topic |
|-----|--------|
| [SPARK_SHARP.md](SPARK_SHARP.md) | Sibling repo, CMake presets, run HelloCsGame |
| [CSHARP_SCRIPTING.md](CSHARP_SCRIPTING.md) | Architecture, debugging, game authoring |

# C# bindings coverage (vs C++ components)

Interop and generated **`Spark.Bindings`** live in the sibling **[spark-sharp](SPARK_SHARP.md)** repo. The engine exposes scriptable surface via **`SPARK_SCRIPT_BIND`** on component headers and a stable C ABI (`SparkInterop`).

## Regenerate and audit

Use a placeholder engine path (see [PATHS_AND_PRIVACY.md](PATHS_AND_PRIVACY.md)); do not commit machine-specific paths.

```bash
export SPARK_ENGINE_ROOT=/path/to/spark
cd /path/to/spark-sharp
./tools/generate-csharp-bindings.sh
python3 tools/scan-cpp-component-api.py
# Report: scripting/bindings/binding-coverage.json
```

After a full regen (Oct 2025 split work), expect **~1,000+** `spark_*` exports and **~115** `Components/FromInterop/*.g.cs` mirrors (plus handle-only types under `Components/Generated/`).

## What is exposed

| Layer | C# access |
|-------|-----------|
| All `ComponentKind` values in `Ecs.hpp` | `GameObject.GetOrAdd*` / `TryGet*` (see `GameObject.ComponentAccess.g.cs`) |
| Methods marked `SPARK_SCRIPT_BIND` | `Components/FromInterop/*Component.g.cs` properties and methods |
| Manual `SparkInterop.h` | `Native.g.cs` + `CppMirrors.g.cs` (`Game`, `GameWorld`, `AddTerrain`, animator, mesh-by-key, …) |

## Intentionally not mirrored (C++-only today)

These patterns are skipped by bindgen (`tools/inject_spark_script_bind.py` / `spark_script_bindgen.py`):

| Pattern | Examples | Workaround in C# |
|---------|----------|------------------|
| `Function<>` / `std::function` callbacks | `PhysicsContactListener3DComponent::SetContactHandler`, `GameplayInventoryCollectorComponent::SetOnItemReceived` | Use **`SignalId`** (`Physics3DContact`, `ItemCollected`) on managed `Game` |
| Methods taking `GameObject& owner` | `Hurtbox3DComponent::GetWorldCenter(owner)` | Pass owner from your `GameObject` handle; use world-space helpers on related components |
| `SharedPtr<>` / asset constructors | `Mesh`, `Animator`, `Terrain`, … | `spark_object_add_*` / `GameObject.Add*` in **`CppMirrors.g.cs`** |
| Internal engine hooks | `FoliageInstancedMeshComponent::RebuildInstanceCache` | C++ editor / systems only |
| Full `Transform` struct | `GetLocalTransform()` | Use existing `spark_transform_set_translation` / rotation / scale in **`SparkInterop.h`** |

Non-component engine APIs (Vulkan, `IEngineContext`, editor, raw physics world) remain C++-only unless added manually to **`SparkInterop.h`**.

## Closing a gap

1. Add `SPARK_SCRIPT_BIND(suffix)` above the C++ member in **`include/spark/ecs/components/...`** (this repo).
2. Run **`./tools/generate-csharp-bindings.sh`** in **spark-sharp**.
3. If the component needs constructor args, add `spark_object_add_*` in **`SparkInterop.h`** and mirror in **`CppMirrors.g.cs`**; add kind to `FACTORY_DENY` if needed.

See **[COMPONENT_SCRIPT_CODEGEN.md](COMPONENT_SCRIPT_CODEGEN.md)** for the full pipeline.

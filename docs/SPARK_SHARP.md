# SparkSharp (C# bindings)

C# scripting, **SparkInterop**, and generated **Spark.Bindings** live in a separate repository:

**[spark-sharp](https://github.com/hoihky/spark-sharp)** — sibling repository next to this engine tree (convention: `../spark-sharp`). Intro: [spark-sharp `docs/index.html`](https://github.com/hoihky/spark-sharp/blob/main/docs/index.html).

The engine keeps only `include/spark/scripting/SparkScriptBind.hpp` — annotation macros scanned by bindgen in **spark-sharp**.

## Build interop

```bash
cd ../spark-sharp
cmake --preset debug -DSPARK_ENGINE_ROOT="$(pwd)/../spark"   # from engine dir: -DSPARK_ENGINE_ROOT="$(pwd)"
cmake --build build --target SparkInterop SparkScriptHost SparkRunHelloCsGameDebug
```

See **spark-sharp/README.md** (debugging C# with VS Code / Rider / CLion).

## Regenerate C# API

From **spark-sharp**:

```bash
export SPARK_ENGINE_ROOT=/path/to/spark
./tools/generate-csharp-bindings.sh
```

See **spark-sharp/README.md** and the former **`docs/CSHARP_SCRIPTING.md`** (content migrating to spark-sharp).

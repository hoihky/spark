# Paths and privacy (contributors)

## Do not commit

| Artifact | Why |
|----------|-----|
| `build/`, `cmake-build-*`, `_deps/` | CMake outputs embed **absolute paths** to this machine |
| `**/bin/`, `**/obj/` | MSBuild / NuGet files often contain **home directory** paths |
| `.env`, keys, `credentials.json` | Secrets |
| `compile_commands.json`, `*FileListAbsolute.txt` | Tooling dumps with full paths |

These are listed in **`.gitignore`**.

## Source and scripts (conventions)

- **CMake / presets** use `${sourceDir}`, `CMAKE_BINARY_DIR`, and options — no hardcoded user paths.
- **Python** resolves repo roots via `Path(__file__).resolve()` (e.g. `docs/site/build-docs.py`, spark-sharp `tools/spark_sharp_paths.py`).
- **Shell** uses `$(dirname "$0")` and **`SPARK_ENGINE_ROOT`** / **`SPARK_SHARP_ROOT`** for sibling checkouts.
- **Docs** use placeholders like `/path/to/spark`, not real home directories.
- **C# interop** lives in **[spark-sharp](../docs/SPARK_SHARP.md)**; see also [spark-sharp `docs/PATHS_AND_PRIVACY.md`](https://github.com/hoihky/spark-sharp/blob/main/docs/PATHS_AND_PRIVACY.md) when working in that repo.

## Before pushing

```bash
./tools/check-paths-privacy.sh
git status   # no build trees, bin/, obj/, or IDE workspace.xml
```

Optional manual scan:

```bash
git grep -E '/Users/|/home/|\\\\Users\\\\' -- ':!build' ':!cmake-build-*' ':!**/obj/**' ':!**/bin/**'
```

If build artifacts were committed, remove them from the index (`git rm -r --cached …`) and rotate any exposed secrets.

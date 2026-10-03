#!/usr/bin/env bash
# Regenerates C# bindings from SparkInterop headers via ClangSharp.
# Run after C++ API changes (or in CI). Requires: dotnet 8+, macOS SDK or Linux headers.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
CONFIG="${ROOT}/scripting/bindings/bindings.config.json"
GENERATOR="${ROOT}/scripting/bindings/generator/Spark.Bindings.Generator.csproj"
OUT="${ROOT}/scripting/bindings/generated/Spark.Bindings"

cd "${ROOT}"
python3 "${ROOT}/tools/inject_spark_script_bind.py"
python3 "${ROOT}/tools/spark_script_bindgen.py"
python3 "${ROOT}/tools/generate_interop_manifest.py"
dotnet tool restore >/dev/null
export PATH="${PATH}:${HOME}/.dotnet/tools"

dotnet run --project "${GENERATOR}" -- "${CONFIG}"

"${ROOT}/tools/sync-component-kind-bindings.sh"
python3 "${ROOT}/tools/scan-cpp-component-api.py" || true
python3 "${ROOT}/tools/generate_native_pinvoke_companion.py"
python3 "${ROOT}/tools/generate-csharp-component-registry.py"

if [[ ! -f "${OUT}/Native.g.cs" ]]; then
  echo "error: Native.g.cs was not generated" >&2
  exit 1
fi

echo "Bindings written to ${OUT}"

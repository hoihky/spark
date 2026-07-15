#!/usr/bin/env bash
# Regenerates C# bindings from SparkInterop headers via ClangSharp.
# Run after C++ API changes (or in CI). Requires: dotnet 8+, macOS SDK or Linux headers.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
CONFIG="${ROOT}/scripting/bindings/bindings.config.json"
GENERATOR="${ROOT}/scripting/bindings/generator/Spark.Bindings.Generator.csproj"
OUT="${ROOT}/scripting/bindings/generated/Spark.Bindings"

cd "${ROOT}"
dotnet tool restore >/dev/null
export PATH="${PATH}:${HOME}/.dotnet/tools"

dotnet run --project "${GENERATOR}" -- "${CONFIG}"

if [[ ! -f "${OUT}/Native.g.cs" ]]; then
  echo "error: Native.g.cs was not generated" >&2
  exit 1
fi

echo "Bindings written to ${OUT}"

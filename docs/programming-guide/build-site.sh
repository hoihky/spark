#!/usr/bin/env bash
# Regenerate the Spark programming guide website with MDWeb.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MDWEB_DIR="${MDWEB_DIR:-$(cd "$SCRIPT_DIR/../../.." && pwd)/MDWeb}"
OUT_DIR="${OUT_DIR:-$SCRIPT_DIR/site}"

cd "$MDWEB_DIR"
dotnet build -v q
dotnet run --project src/MDWeb.Cli -- \
  -s "$SCRIPT_DIR" \
  -o "$OUT_DIR" \
  -t ./themes/default \
  --title "Spark Game Engine Programming Guide" \
  --description "Learn to build 2D and 3D games with the Spark Game Engine in C++23"

echo "Site: $OUT_DIR"
echo "Preview: npx serve \"$OUT_DIR\""

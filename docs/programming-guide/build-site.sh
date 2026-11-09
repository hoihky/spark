#!/usr/bin/env bash
# Regenerate programming guide HTML under site/ from .md sources.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Primary pipeline (Python markdown → themed HTML + nav)
python3 "$SCRIPT_DIR/rebuild-site.py"

# Optional: MDWeb static site (requires sibling MDWeb repo + dotnet)
if [[ "${USE_MDWEB:-0}" == "1" ]]; then
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
  echo "MDWeb site: $OUT_DIR"
fi

echo "Programming guide site: $SCRIPT_DIR/site"
echo "Preview: npx serve \"$SCRIPT_DIR/site\""

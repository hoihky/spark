#!/usr/bin/env bash
# Regenerate all Spark documentation HTML and embed theme CSS.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

python3 site/build-docs.py
python3 programming-guide/patch-site-from-md.py
python3 ensure-html-theme.py

echo ""
echo "Docs built. Preview locally:"
echo "  cd \"$SCRIPT_DIR\" && python3 -m http.server 8080"
echo "  open http://127.0.0.1:8080/index.html"

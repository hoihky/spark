#!/usr/bin/env bash
# Fail if tracked source/config files contain machine-specific absolute paths or common secret patterns.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

if ! git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
  echo "check-paths-privacy: not a git repository" >&2
  exit 1
fi

GLOB=(
  '*.py' '*.sh' '*.json' '*.yml' '*.yaml' '*.cmake' '*.cmake.in'
  'CMakeLists.txt' 'CMakePresets.json'
  '*.md' '*.html' '*.xml' '*.properties'
  '.vscode/*' '.run/*' '.idea/runConfigurations/*'
)

PATH_PATTERN='(/Users/|/home/[^/]+/|\\\\Users\\\\|C:\\\\Users\\\\)'
ALLOW_PATH_DOC='docs/PATHS_AND_PRIVACY.md'

SECRET_PATTERN='(api[_-]?key\s*[:=]|password\s*[:=]|secret\s*[:=]|Bearer\s+[A-Za-z0-9._-]{20,})'

PATH_HITS=0
SECRET_HITS=0
COUNT=0

while IFS= read -r f; do
  [[ -n "$f" ]] || continue
  COUNT=$((COUNT + 1))
  [[ -f "$f" ]] || continue
  if [[ "$f" == "$ALLOW_PATH_DOC" ]]; then
    continue
  fi
  if grep -qE "$PATH_PATTERN" "$f" 2>/dev/null; then
    echo "absolute path pattern in: $f"
    grep -nE "$PATH_PATTERN" "$f" | head -5
    PATH_HITS=1
  fi
  if grep -qiE "$SECRET_PATTERN" "$f" 2>/dev/null; then
    echo "possible secret pattern in: $f"
    grep -niE "$SECRET_PATTERN" "$f" | head -3
    SECRET_HITS=1
  fi
done < <(
  git ls-files -- "${GLOB[@]}" 2>/dev/null | grep -vE '^docs/site/|^docs/programming-guide/site/' || true
)

if ((COUNT == 0)); then
  echo "check-paths-privacy: no tracked files matched"
  exit 0
fi

if ((PATH_HITS != 0 || SECRET_HITS != 0)); then
  echo "check-paths-privacy: FAILED (use /path/to/... placeholders and env vars in committed files)" >&2
  exit 1
fi

echo "check-paths-privacy: OK ($COUNT tracked files scanned)"

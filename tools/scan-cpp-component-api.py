#!/usr/bin/env python3
"""Scan *Component.hpp public methods and compare to spark_* interop (coverage report)."""
from __future__ import annotations

import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
COMPONENT_GLOB = ROOT / "include/spark/ecs/components"
INTEROP = ROOT / "include/spark/scripting/SparkInterop.h"
OUT = ROOT / "scripting/bindings/binding-coverage.json"

METHOD_RE = re.compile(
    r"^\s+(?:virtual\s+)?(?:[\w:<>,\s&]+\s+)?(\w+)\s*\([^;]*\)\s*(?:const)?\s*(?:override)?\s*;",
    re.MULTILINE,
)

SPARK_FN_RE = re.compile(r"spark_(\w+?)_(?:get_|set_|is_|[\w_]+)", re.MULTILINE)


def component_kind_from_path(path: Path) -> str:
    name = path.stem  # FooComponent
    if name.endswith("Component"):
        name = name[: -len("Component")]
    return name


def scan_headers() -> dict[str, list[str]]:
    result: dict[str, list[str]] = {}
    for path in COMPONENT_GLOB.rglob("*Component.hpp"):
        text = path.read_text(encoding="utf-8", errors="ignore")
        methods = []
        for m in METHOD_RE.finditer(text):
            name = m.group(1)
            if name in {"Kind", "OnAttach", "OnDetach", "OnUpdate", "OnSignal", "UpdatePriority"}:
                continue
            methods.append(name)
        if methods:
            result[component_kind_from_path(path)] = sorted(set(methods))
    return result


def scan_interop_prefixes() -> set[str]:
    text = INTEROP.read_text(encoding="utf-8")
    prefixes: set[str] = set()
    for m in SPARK_FN_RE.finditer(text):
        prefixes.add(m.group(1))
    return prefixes


def main() -> int:
    headers = scan_headers()
    prefixes = scan_interop_prefixes()
    report = {
        "componentsWithPublicMethods": len(headers),
        "interopPrefixes": sorted(prefixes),
        "gaps": [],
    }
    for kind, methods in sorted(headers.items()):
        snake = re.sub(r"(?<!^)(?=[A-Z])", "_", kind).lower()
        has_prefix = any(
            snake == p or snake.startswith(p + "_") or p.startswith(snake)
            for p in prefixes
        )
        if not has_prefix and methods:
            report["gaps"].append({"kind": kind, "methodCount": len(methods), "sampleMethods": methods[:8]})
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote {OUT} ({len(report['gaps'])} kinds without obvious spark_ prefix)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

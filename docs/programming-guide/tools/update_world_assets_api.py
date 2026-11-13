#!/usr/bin/env python3
"""Rewrite GameWorld flat asset API examples to WorldAssets / world.Assets (C++ / C#)."""
from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DOCS_ROOT = ROOT.parent

MARKDOWN_ROOTS = [
    ROOT,
    DOCS_ROOT,
]

SKIP_UNDER = {"site", "tools", "__pycache__"}

CPP_RULES: list[tuple[re.Pattern[str], str]] = [
    (re.compile(r"\bGameWorld::LoadSkinnedGltf\b"), "WorldAssets::LoadSkinnedGltf"),
    (re.compile(r"\bGameWorld::LoadGltf\b"), "WorldAssets::LoadGltf"),
    (re.compile(r"\bGameWorld::LoadTexture\b"), "WorldAssets::LoadTexture"),
    (re.compile(r"\bGameWorld::LoadMesh\b"), "WorldAssets::LoadMesh"),
    (re.compile(r"\bGameWorld::RegisterTexture\b"), "WorldAssets::RegisterTexture"),
    (re.compile(r"\bGameWorld::RegisterMesh\b"), "WorldAssets::RegisterMesh"),
    (re.compile(r"\bGameWorld::PumpAssets\b"), "WorldAssets::Pump"),
    (re.compile(r"\bw\.RegisterTexture\("), "w.Assets().RegisterTexture("),
    (re.compile(r"\bw\.RegisterMesh\("), "w.Assets().RegisterMesh("),
    (re.compile(r"\bworld\.PumpAssets\(\)"), "world.Assets().Pump()"),
    (re.compile(r"\bworld\.GetPendingAssetLoadCount\(\)"), "world.Assets().PendingLoadCount()"),
    (re.compile(r"\bworld\.(RequestGltf|IsGltfReady|IsSkinnedGltfReady|AwaitGltf|AwaitSkinnedGltf)\("),
     r"world.Assets().\1("),
    (re.compile(r"\bworld\.(LoadMesh|LoadGltf|LoadSkinnedGltf|LoadTexture)\("), r"world.Assets().\1("),
    (re.compile(r"\bworld\.(RegisterMesh|RegisterTexture|RegisterGltf|RegisterSkinnedGltf)\("),
     r"world.Assets().\1("),
]

CSHARP_RULES: list[tuple[re.Pattern[str], str]] = [
    (re.compile(r"\bWorld\.PumpAssets\(\)"), "World.Assets.Pump()"),
    (re.compile(r"\bworld\.PumpAssets\(\)"), "world.Assets.Pump()"),
    (re.compile(r"\bWorld\.(LoadGltf|LoadSkinnedGltf|LoadTexture)\("), r"World.Assets.\1("),
    (re.compile(r"\bworld\.(LoadGltf|LoadSkinnedGltf|LoadTexture|LoadMesh)\("), r"world.Assets.\1("),
    (re.compile(r"\bworld\.(RegisterMesh|RegisterTexture)\("), r"world.Assets.\1("),
    (re.compile(r"\bworld\.RegisterTextureFromFile\("), "world.Assets.RegisterTextureFromFile("),
    (re.compile(r"\bWorld\.RequestGltf\("), "World.Assets.LoadGltf("),  # RequestGltf not bound; doc sync load
    (re.compile(r"\bworld\.(IsGltfReady|IsSkinnedGltfReady)\("), r"world.Assets.\1("),
]

PROSE_RULES: list[tuple[re.Pattern[str], str]] = [
    (re.compile(r"\bworld\.PumpAssets\(\)"), "world.Assets().Pump()"),
    (re.compile(r"`world\.RegisterMesh\("), "`world.Assets().RegisterMesh(`"),
    (re.compile(r"`world\.RegisterTexture`"), "`world.Assets().RegisterTexture`"),
    (re.compile(r"\bGameWorld::LoadTexture\b"), "WorldAssets::LoadTexture"),
    (re.compile(r"\bGameWorld::LoadSkinnedGltf\b"), "WorldAssets::LoadSkinnedGltf"),
    (re.compile(r"\bGameWorld::LoadSkinnedGltf\b"), "WorldAssets::LoadSkinnedGltf"),
    (re.compile(r"`PumpAssets`"), "`Assets().Pump()` (C++) / `Assets.Pump` (C#)"),
    (re.compile(r"async `RequestGltf` \+ `PumpAssets`"), "`Assets().RequestGltf` + `Assets().Pump()`"),
    (re.compile(r"`GameWorldAssetCache` \(via `GameWorld`\)"), "`WorldAssets` (`world.Assets()`) backed by `GameWorldAssetCache`"),
    (re.compile(r"\*\*`GameWorldAssetCache`\*\* \(`assetCache`\)"), "**`WorldAssets`** (`world.Assets()`): cache + loader surface"),
    (re.compile(r"asset I/O via \*\*`GameWorldAssetCache`\*\*"), "asset I/O via **`WorldAssets`** (`world.Assets()`)"),
    (re.compile(r"`GameWorld::LoadMesh`, `LoadGltf`, `LoadSkinnedGltf`, `LoadTexture`"),
     "`WorldAssets::LoadMesh`, `LoadGltf`, `LoadSkinnedGltf`, `LoadTexture`"),
    (re.compile(r"`WorldAssetCache`"), "`WorldAssets`"),
    (re.compile(r"`world\.AssetCache`"), "`world.Assets`"),
]


def apply_rules(text: str, rules: list[tuple[re.Pattern[str], str]]) -> str:
    for pattern, repl in rules:
        text = pattern.sub(repl, text)
    return text


def transform_markdown(text: str) -> str:
    out: list[str] = []
    pos = 0
    for match in re.finditer(r"```([^\n]*)\n(.*?)```", text, re.DOTALL):
        out.append(apply_rules(text[pos : match.start()], PROSE_RULES))
        lang = (match.group(1) or "").strip().lower()
        body = match.group(2)
        if lang in ("cpp", "c++", "cxx"):
            body = apply_rules(body, CPP_RULES)
        elif lang in ("csharp", "cs", "c#"):
            body = apply_rules(body, CSHARP_RULES)
        fence_lang = match.group(1) or ""
        out.append(f"```{fence_lang}\n{body}```")
        pos = match.end()
    out.append(apply_rules(text[pos:], PROSE_RULES))
    return "".join(out)


def iter_md_files() -> list[Path]:
    files: list[Path] = []
    for base in MARKDOWN_ROOTS:
        for path in base.rglob("*.md"):
            if any(part in SKIP_UNDER for part in path.parts):
                continue
            if "/programming-guide/" in str(path) and "programming-guide/site" in str(path):
                continue
            files.append(path)
    return sorted(set(files))


def main() -> int:
    changed = 0
    for path in iter_md_files():
        raw = path.read_text(encoding="utf-8")
        if "PumpAssets" not in raw and "LoadGltf" not in raw and "GameWorld::" not in raw and "RegisterTexture" not in raw:
            if "GameWorldAssetCache" not in raw and "AssetCache" not in raw:
                continue
        updated = transform_markdown(raw)
        if updated != raw:
            path.write_text(updated, encoding="utf-8")
            print(f"updated: {path.relative_to(DOCS_ROOT.parent)}")
            changed += 1
    # generate_ebook.py has embedded samples
    ebook = ROOT / "generate_ebook.py"
    if ebook.is_file():
        raw = ebook.read_text(encoding="utf-8")
        updated = apply_rules(apply_rules(raw, CPP_RULES), PROSE_RULES)
        if updated != raw:
            ebook.write_text(updated, encoding="utf-8")
            print("updated: docs/programming-guide/generate_ebook.py")
            changed += 1
    print(f"done ({changed} files)")
    return 0


if __name__ == "__main__":
    sys.exit(main())

#!/usr/bin/env python3
"""
Split Spark demo headers: class definitions stay in .hpp (declarations only),
implementations go to src/spark/demo/<Base>.cpp with Spark::<Class>::method.

Handles: public/private sections, nested braces, [[nodiscard]] static, multi-line params.
Skips lines that are only member variable declarations (contain ';' before any '{' from '=').
"""
from __future__ import annotations

import re
import sys
from pathlib import Path
from dataclasses import dataclass
from typing import List, Optional, Tuple

ROOT = Path(__file__).resolve().parents[1]
INC = ROOT / "include" / "spark" / "demo"
SRC_DEMO = ROOT / "src" / "spark" / "demo"


def find_matching_brace(s: str, open_idx: int) -> int:
    """s[open_idx] must be '{'. Returns index of matching '}'."""
    depth = 0
    i = open_idx
    n = len(s)
    while i < n:
        c = s[i]
        if c == "/" and i + 1 < n:
            nxt = s[i + 1]
            if nxt == "/":
                i = s.find("\n", i + 2)
                if i < 0:
                    return -1
                continue
            if nxt == "*":
                end = s.find("*/", i + 2)
                if end < 0:
                    return -1
                i = end + 2
                continue
        if c == '"':
            i += 1
            while i < n:
                if s[i] == "\\":
                    i += 2
                    continue
                if s[i] == '"':
                    break
                i += 1
            i += 1
            continue
        if c == "'":
            i += 1
            while i < n:
                if s[i] == "\\":
                    i += 2
                    continue
                if s[i] == "'":
                    break
                i += 1
            i += 1
            continue
        if c == "{":
            depth += 1
        elif c == "}":
            depth -= 1
            if depth == 0:
                return i
        i += 1
    return -1


@dataclass
class MethodChunk:
    name: str
    full_sig_before_brace: str  # includes leading whitespace/newlines before 'void'
    body_with_braces: str  # from '{' through matching '}'


def strip_default_from_sig(sig: str) -> str:
    """Remove '= default' / '= delete' from defaulted special members (keep rare)."""
    return sig


def extract_class(text: str, class_name: str) -> Tuple[str, str, str, str]:
    """
    Returns: (pre_namespace, namespace_open, class_decl_skeleton, rest_after_class)
    class_decl_skeleton is the class without method bodies (replaced by ';')
    """
    m = re.search(
        rf"\bclass\s+{re.escape(class_name)}\b(?:\s+final)?\s*\{{",
        text,
    )
    if not m:
        raise ValueError(f"class {class_name} not found")
    class_open_brace = m.end() - 1
    class_close = find_matching_brace(text, class_open_brace)
    if class_close < 0:
        raise ValueError(f"unclosed class {class_name}")

    before = text[: m.start()]
    after = text[class_close + 1 :]

    class_body = text[class_open_brace + 1 : class_close]
    methods: List[MethodChunk] = []

    i = 0
    lb = len(class_body)
    while i < lb:
        # skip access specifiers
        if class_body.startswith("public:", i) or class_body.startswith("private:", i) or class_body.startswith(
            "protected:", i
        ):
            nl = class_body.find("\n", i)
            if nl < 0:
                break
            i = nl + 1
            continue
        # skip whitespace
        if class_body[i] in " \t\n\r":
            i += 1
            continue
        # skip using / static_assert / friend
        if class_body.startswith("using ", i) or class_body.startswith("static_assert", i):
            nl = class_body.find("\n", i)
            i = nl + 1 if nl >= 0 else lb
            continue
        if class_body.startswith("friend ", i):
            depth = 0
            j = i
            while j < lb:
                if class_body[j] == "{":
                    depth += 1
                elif class_body[j] == "}":
                    depth -= 1
                    if depth == 0:
                        j += 1
                        break
                elif class_body[j] == ";" and depth == 0:
                    j += 1
                    break
                j += 1
            i = j
            continue

        # Heuristic: member data line (has ';' before '{') — consume one statement
        stmt_end = class_body.find(";", i)
        brace_before_semi = class_body.find("{", i, stmt_end) if stmt_end >= 0 else -1
        if stmt_end >= 0 and (brace_before_semi < 0 or brace_before_semi > stmt_end):
            i = stmt_end + 1
            continue

        # Possible function: find '(' before '{'
        br = class_body.find("{", i)
        if br < 0:
            break
        par = class_body.rfind("(", i, br)
        if par < 0:
            i = br + 1
            continue
        # name before '('
        j = par - 1
        while j >= i and class_body[j] in " \t\n\r":
            j -= 1
        if j < i:
            i = br + 1
            continue
        name_end = j + 1
        k = j
        while k >= i and (class_body[k].isalnum() or class_body[k] == "_"):
            k -= 1
        name_start = k + 1
        name = class_body[name_start:name_end]
        if not name or not name[0].isalpha():
            i = br + 1
            continue
        # operator() etc.
        if name in ("if", "for", "while", "switch"):
            i = br + 1
            continue

        sig_start = i
        full_through_brace = class_body[sig_start : br + 1]
        body_close = find_matching_brace(class_body, br)
        if body_close < 0:
            raise ValueError(f"bad brace for method {name}")
        body = class_body[br : body_close + 1]
        methods.append(MethodChunk(name=name, full_sig_before_brace=class_body[sig_start:br].rstrip(), body_with_braces=body))
        i = body_close + 1
        while i < lb and class_body[i] in " \t\n\r":
            i += 1

    # Build new class text: replace each method region with declaration
    pieces: List[str] = []
    last = 0
    for mc in methods:
        sig_start = class_body.find(mc.full_sig_before_brace, last)
        if sig_start < 0:
            sig_start = class_body.find(mc.name + "(", last)
        if sig_start < 0:
            raise ValueError(f"rescan failed for {mc.name}")
        br = class_body.find("{", sig_start)
        body_close = find_matching_brace(class_body, br)
        pieces.append(class_body[last:sig_start])
        decl = mc.full_sig_before_brace.strip()
        if not decl.endswith(")"):
            decl = decl.rstrip()
        pieces.append(decl + ";\n")
        last = body_close + 1
    pieces.append(class_body[last:])

    new_class_inner = "".join(pieces)
    new_hpp_class = text[m.start() : class_open_brace + 1] + new_class_inner + "\n};\n"

    cpp_methods = []
    for mc in methods:
        sig = mc.full_sig_before_brace.strip()
        sig = re.sub(r"^inline\s+", "", sig)
        pat = re.compile(rf"\b{re.escape(mc.name)}\b\s*\(")
        sig_qual, n = pat.subn(f"{class_name}::{mc.name}(", sig, count=1)
        if n != 1:
            raise ValueError(f"could not qualify {class_name}::{mc.name} in signature: {sig!r}")
        cpp_methods.append(f"\n{sig_qual}\n{mc.body_with_braces}\n")

    cpp_body = "".join(cpp_methods)
    return before, after, new_hpp_class, cpp_body


def process_file(rel_hpp: str, class_name: str) -> None:
    path = INC / rel_hpp
    text = path.read_text()
    before, after, new_class, cpp_methods_src = extract_class(text, class_name)

    # Re-assemble hpp: before + new_class + after (trim duplicate closing namespace if needed)
    m_ns = re.search(r"namespace\s+Spark\s*\{", before)
    if not m_ns:
        raise ValueError("namespace Spark not found")
    # 'before' should include everything up to class; 'after' should be }; namespace close
    new_text = before + new_class + after
    path.write_text(new_text)

    base = Path(rel_hpp).stem
    cpp_path = SRC_DEMO / f"{base}.cpp"
    cpp = (
        f'#include "spark/demo/{rel_hpp}"\n\n'
        f"namespace Spark {{\n"
        f"{cpp_methods_src}"
        f"}}  // namespace Spark\n"
    )
    cpp_path.write_text(cpp)
    print(f"Wrote {cpp_path}")


def main() -> None:
    jobs = [
        ("TwoDDemo.hpp", "TwoDDemo"),
        ("ThreeDDemo.hpp", "ThreeDDemo"),
        ("SkyDemo.hpp", "SkyDemo"),
        ("TerrainDemo.hpp", "TerrainDemo"),
        ("ParticleDemo.hpp", "ParticleDemo"),
        ("CharacterCameraDemo.hpp", "CharacterCameraDemo"),
        ("BroadPhase2DDemo.hpp", "BroadPhase2DDemo"),
        ("Platformer2DDemo.hpp", "Platformer2DDemo"),
        ("Connect3Demo.hpp", "Connect3Demo"),
        ("Tetris2DDemo.hpp", "Tetris2DDemo"),
        ("SpaceInvaders2DDemo.hpp", "SpaceInvaders2DDemo"),
        ("PhysicsBallThrow3DDemo.hpp", "PhysicsBallThrow3DDemo"),
        ("Maze3DDemo.hpp", "Maze3DDemo"),
        ("SteeringShowcase3DDemo.hpp", "SteeringShowcase3DDemo"),
        ("SceneEditor3DDemo.hpp", "SceneEditor3DDemo"),
    ]
    for rel, cls in jobs:
        print(f"=== {rel} :: {cls} ===")
        process_file(rel, cls)


if __name__ == "__main__":
    main()

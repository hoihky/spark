#!/usr/bin/env python3
"""Embed theme CSS and fix asset URLs for all docs HTML pages."""

from __future__ import annotations

import os
import re
import sys
from pathlib import Path

DOCS_ROOT = Path(__file__).resolve().parent
MAIN_CSS = DOCS_ROOT / "site/css/site.css"
GUIDE_CSS = DOCS_ROOT / "programming-guide/site/assets/css/site.css"
GUIDE_HIGHLIGHT_CSS = DOCS_ROOT / "programming-guide/site/assets/vendor/atom-one-dark.min.css"
GUIDE_JS = DOCS_ROOT / "programming-guide/site/assets/js/site.js"
GUIDE_HLJS = DOCS_ROOT / "programming-guide/site/assets/vendor/highlight.min.js"
GUIDE_MERMAID = DOCS_ROOT / "programming-guide/site/assets/vendor/mermaid.min.js"

INLINE_STYLE_RE = re.compile(r"\s*<style id=\"spark-inline-theme\">.*?</style>", re.DOTALL)
STYLESHEET_LINK_RE = re.compile(
    r'<link rel="stylesheet" href="(?P<href>[^"]+)"(\s*/)?>',
    re.IGNORECASE,
)
SCRIPT_SRC_RE = re.compile(r'<script src="(?P<src>[^"]+)"')


def rel_href(from_file: Path, to_file: Path) -> str:
    return os.path.relpath(to_file, from_file.parent).replace("\\", "/")


def css_bundle(paths: list[Path]) -> str:
    parts: list[str] = []
    for path in paths:
        if path.is_file():
            parts.append(path.read_text(encoding="utf-8"))
    return "\n".join(parts)


def inject_inline_style(html: str, css_paths: list[Path]) -> str:
    block = f'<style id="spark-inline-theme">\n{css_bundle(css_paths)}\n</style>'
    if INLINE_STYLE_RE.search(html):
        return INLINE_STYLE_RE.sub(f"\n  {block}", html, count=1)
    return html.replace("</head>", f"  {block}\n</head>", 1)


def set_stylesheet(html: str, old_href: str, new_href: str) -> str:
    return html.replace(f'href="{old_href}"', f'href="{new_href}"', 1)


def ensure_script(html: str, marker: str, script_tag: str) -> str:
    if marker in html:
        return html
    return html.replace("</body>", f"  {script_tag}\n</body>", 1)


def patch_main_doc(html_path: Path) -> None:
    html = html_path.read_text(encoding="utf-8")
    css_href = rel_href(html_path, MAIN_CSS)
    highlight_href = rel_href(html_path, GUIDE_HIGHLIGHT_CSS)
    html = inject_inline_style(html, [MAIN_CSS, GUIDE_HIGHLIGHT_CSS])
    if 'href="../css/site.css"' in html:
        html = set_stylesheet(html, "../css/site.css", css_href)
    elif 'href="site/css/site.css"' in html:
        html = set_stylesheet(html, "site/css/site.css", css_href)
    for old in (
        "../../programming-guide/site/assets/vendor/atom-one-dark.min.css",
        "../programming-guide/site/assets/vendor/atom-one-dark.min.css",
    ):
        if f'href="{old}"' in html:
            html = set_stylesheet(html, old, highlight_href)
            break
    html_path.write_text(html, encoding="utf-8")
    print(f"Themed {html_path.relative_to(DOCS_ROOT)}")


def patch_guide_doc(html_path: Path) -> None:
    html = html_path.read_text(encoding="utf-8")
    css_href = rel_href(html_path, GUIDE_CSS)
    highlight_href = rel_href(html_path, GUIDE_HIGHLIGHT_CSS)
    js_href = rel_href(html_path, GUIDE_JS)
    hljs_href = rel_href(html_path, GUIDE_HLJS)
    mermaid_href = rel_href(html_path, GUIDE_MERMAID)

    html = inject_inline_style(html, [GUIDE_CSS, GUIDE_HIGHLIGHT_CSS])

    for old in ("../assets/css/site.css", "assets/css/site.css"):
        if f'href="{old}"' in html:
            html = set_stylesheet(html, old, css_href)
            break
    for old in ("../assets/vendor/atom-one-dark.min.css", "assets/vendor/atom-one-dark.min.css"):
        if f'href="{old}"' in html:
            html = set_stylesheet(html, old, highlight_href)
            break

    html = re.sub(
        r'<script src="(?:\.\./|)assets/vendor/mermaid\.min\.js"></script>',
        f'<script src="{mermaid_href}"></script>',
        html,
        count=1,
    )
    html = re.sub(
        r'<script src="(?:\.\./|)assets/vendor/highlight\.min\.js"></script>',
        f'<script src="{hljs_href}"></script>',
        html,
        count=1,
    )
    html = re.sub(
        r'<script src="(?:\.\./|)assets/js/site\.js"></script>',
        f'<script src="{js_href}"></script>',
        html,
        count=1,
    )

    html_path.write_text(html, encoding="utf-8")
    print(f"Themed {html_path.relative_to(DOCS_ROOT)}")


def main() -> None:
    if not MAIN_CSS.is_file():
        print(f"Missing theme CSS: {MAIN_CSS}", file=sys.stderr)
        sys.exit(1)

    for html_path in sorted(DOCS_ROOT.rglob("*.html")):
        rel = html_path.relative_to(DOCS_ROOT).as_posix()
        if rel.startswith("programming-guide/site/"):
            patch_guide_doc(html_path)
        elif rel == "index.html" or rel.startswith("site/docs/"):
            patch_main_doc(html_path)

    print("Done — inline theme CSS embedded in docs HTML pages.")


if __name__ == "__main__":
    main()

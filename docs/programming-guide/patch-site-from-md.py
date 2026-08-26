#!/usr/bin/env python3
"""Patch programming-guide/site HTML bodies from sibling .md sources."""

from __future__ import annotations

import re
import sys
from pathlib import Path

try:
    import markdown
    from markdown.extensions.fenced_code import FencedCodeExtension
    from markdown.extensions.tables import TableExtension
    from markdown.extensions.toc import TocExtension
except ImportError:
    print("Missing dependency: pip install markdown", file=sys.stderr)
    sys.exit(1)

ROOT = Path(__file__).resolve().parent
SITE = ROOT / "site"

BODY_START = '<div class="doc-body">'
BODY_END = '</div>\n          <footer class="doc-footer">'


def md_to_html(text: str) -> str:
    md = markdown.Markdown(
        extensions=[
            TableExtension(),
            FencedCodeExtension(),
            TocExtension(permalink=False, toc_depth=3),
        ]
    )
    html = md.convert(text)
    # MDWeb uses .html links for internal refs.
    html = re.sub(r'href="([^"]+?)\.md(#[^"]*)?"', r'href="\1.html\2"', html)
    html = re.sub(
        r'<pre><code class="language-mermaid">(.*?)</code></pre>',
        lambda m: f'<pre class="mermaid">{m.group(1)}</pre>',
        html,
        flags=re.DOTALL,
    )
    return html


def patch_page(md_path: Path) -> bool:
    rel = md_path.relative_to(ROOT)
    if rel.name == "index.md":
        html_path = SITE / "index.html"
    else:
        html_path = SITE / rel.with_suffix(".html")

    if not html_path.is_file():
        print(f"Skip (no html): {html_path.relative_to(ROOT)}", file=sys.stderr)
        return False

    page = html_path.read_text(encoding="utf-8")
    start = page.find(BODY_START)
    end = page.find(BODY_END)
    if start < 0 or end < 0 or end <= start:
        print(f"Skip (markers missing): {html_path.relative_to(ROOT)}", file=sys.stderr)
        return False

    body = md_to_html(md_path.read_text(encoding="utf-8"))
    updated = page[: start + len(BODY_START)] + body + page[end:]
    html_path.write_text(updated, encoding="utf-8")
    print(f"Patched {html_path.relative_to(ROOT)}")
    return True


def main() -> None:
    count = 0
    for md_path in sorted(ROOT.rglob("*.md")):
        if "site/" in md_path.as_posix():
            continue
        if patch_page(md_path):
            count += 1
    print(f"Done — patched {count} pages under {SITE.relative_to(ROOT)}/")

    ensure = ROOT / "ensure-html-theme.py"
    if ensure.is_file():
        import subprocess
        import sys

        subprocess.run([sys.executable, str(ensure)], check=True)


if __name__ == "__main__":
    main()

"""Shared Markdown → HTML helpers for programming-guide site builds."""

from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path

try:
    import markdown
    from markdown.extensions.fenced_code import FencedCodeExtension
    from markdown.extensions.tables import TableExtension
    from markdown.extensions.toc import TocExtension
except ImportError:
    markdown = None  # type: ignore[assignment]

ROOT = Path(__file__).resolve().parent
_HLJS_SCRIPT = ROOT / "highlight-html-blocks.cjs"

_CSHARP_HEADING = re.compile(r"^(#{1,6}\s+.+?)C#(\s*)$")


def prepare_markdown_for_html(text: str) -> str:
    """ATX headings treat a trailing # as a closer, so '### C#' becomes 'C' — escape it."""
    out: list[str] = []
    for line in text.splitlines():
        m = _CSHARP_HEADING.match(line)
        out.append(f"{m.group(1)}C\\#{m.group(2)}" if m else line)
    return "\n".join(out)


def highlight_codeblock_html(html: str) -> str:
    """Apply highlight.js to fenced code blocks at build time (works without client JS)."""
    if not _HLJS_SCRIPT.is_file():
        return html
    try:
        proc = subprocess.run(
            ["node", str(_HLJS_SCRIPT)],
            input=html.encode("utf-8"),
            capture_output=True,
            check=True,
        )
        return proc.stdout.decode("utf-8")
    except FileNotFoundError:
        print("Warning: node not found — skipping syntax highlight", file=sys.stderr)
    except subprocess.CalledProcessError as exc:
        err = (exc.stderr or b"").decode("utf-8", errors="replace")
        print(f"Warning: syntax highlight failed: {err}", file=sys.stderr)
    return html


def md_to_html(text: str, *, highlight: bool = True) -> str:
    if markdown is None:
        raise RuntimeError("Missing dependency: pip install markdown")
    text = prepare_markdown_for_html(text)
    md = markdown.Markdown(
        extensions=[
            TableExtension(),
            FencedCodeExtension(),
            TocExtension(permalink=False, toc_depth=3),
        ]
    )
    html = md.convert(text)
    html = re.sub(r'href="([^"]+?)\.md(#[^"]*)?"', r'href="\1.html\2"', html)
    html = re.sub(
        r'<pre><code class="language-mermaid">(.*?)</code></pre>',
        lambda m: f'<pre class="mermaid">{m.group(1)}</pre>',
        html,
        flags=re.DOTALL,
    )
    if highlight:
        html = highlight_codeblock_html(html)
    return html

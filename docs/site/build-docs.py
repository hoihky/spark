#!/usr/bin/env python3
"""Convert Spark markdown docs to static HTML pages under site/docs/."""

from __future__ import annotations

import re
import sys
from pathlib import Path

try:
    import markdown
    from markdown.extensions.tables import TableExtension
    from markdown.extensions.fenced_code import FencedCodeExtension
    from markdown.extensions.toc import TocExtension
except ImportError:
    print("Missing dependency: pip install markdown", file=sys.stderr)
    sys.exit(1)

REPO_ROOT = Path(__file__).resolve().parent.parent.parent
DOCS_ROOT = Path(__file__).resolve().parent.parent
OUT_DIR = DOCS_ROOT / "site" / "docs"

# Source markdown -> output HTML filename (under docs/site/docs/)
PAGES: list[tuple[Path, str, str]] = [
    (REPO_ROOT / "README.md", "readme.html", "Spark — README"),
    (DOCS_ROOT / "ARCHITECTURE_AND_DEVELOPER_GUIDE.md", "architecture-and-developer-guide.html", "Architecture & Developer Guide"),
    (DOCS_ROOT / "LIGHTING_AND_SHADOWS.md", "lighting-and-shadows.html", "Lighting & Shadows"),
    (DOCS_ROOT / "MATERIALS_AND_LIGHTING.md", "materials-and-lighting.html", "Materials & Lighting"),
    (DOCS_ROOT / "SPARK_EDITOR_PLAN.md", "spark-editor-plan.html", "Spark Editor Plan"),
    (DOCS_ROOT / "GUI_EDITOR_ROADMAP.md", "gui-editor-roadmap.html", "GUI & Editor Roadmap"),
    (DOCS_ROOT / "ANIMATION_3D_ROADMAP.md", "animation-3d-roadmap.html", "3D Animation Roadmap"),
]

# Maps any .md basename (or docs/foo.md path) to site/docs HTML output.
MD_LINK_MAP: dict[str, str] = {src.name: dst for src, dst, _ in PAGES}
MD_LINK_MAP.update(
    {
        "ARCHITECTURE_AND_DEVELOPER_GUIDE.md": "architecture-and-developer-guide.html",
        "LIGHTING_AND_SHADOWS.md": "lighting-and-shadows.html",
        "MATERIALS_AND_LIGHTING.md": "materials-and-lighting.html",
        "SPARK_EDITOR_PLAN.md": "spark-editor-plan.html",
        "GUI_EDITOR_ROADMAP.md": "gui-editor-roadmap.html",
        "ANIMATION_3D_ROADMAP.md": "animation-3d-roadmap.html",
        "README.md": "readme.html",
    }
)

PROGRAMMING_GUIDE_SITE = "../../programming-guide/site"

TEMPLATE = """<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>{title} · Spark Game Engine</title>
  <meta name="description" content="{description}">
  <link rel="preconnect" href="https://fonts.googleapis.com">
  <link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
  <link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700&family=JetBrains+Mono:wght@400;500&display=swap" rel="stylesheet">
  <link rel="stylesheet" href="../css/site.css">
  <script src="https://cdn.jsdelivr.net/npm/mermaid@10/dist/mermaid.min.js"></script>
  <script>mermaid.initialize({{ startOnLoad: true, theme: 'dark', securityLevel: 'loose' }});</script>
</head>
<body>
  <div class="site">
    <header class="topbar">
      <a href="../../index.html" class="brand">Spark Game Engine</a>
      <nav class="topbar-nav" aria-label="Primary">
        <a href="../../index.html#documentation">Documentation</a>
        <a href="{programming_guide}">Programming guide</a>
        <a href="https://github.com/hoihky/spark" target="_blank" rel="noopener">GitHub</a>
      </nav>
    </header>

    <main class="doc-page">
      <article class="doc-body">
        {body}
      </article>
    </main>

    <footer class="site-footer doc-page-footer">
      <p>
        <a href="../../index.html">← Back to Spark home</a>
        · <a href="https://github.com/hoihky/spark" target="_blank" rel="noopener">GitHub</a>
      </p>
    </footer>
  </div>
</body>
</html>
"""


def rewrite_links(html: str) -> str:
    def replace_href(match: re.Match[str]) -> str:
        href = match.group(1)
        if href.startswith(("http://", "https://", "mailto:", "#")):
            return match.group(0)
        if "programming-guide/" in href and href.endswith(".md"):
            rel = href.removeprefix("docs/").removeprefix("programming-guide/")
            rel = rel.removesuffix(".md") + ".html"
            return f'href="{PROGRAMMING_GUIDE_SITE}/{rel}"'
        if href.endswith(".md"):
            base = Path(href).name
            if base in MD_LINK_MAP:
                return f'href="{MD_LINK_MAP[base]}"'
        return match.group(0)

    html = re.sub(r'href="([^"]+)"', replace_href, html)
    return html


def fix_mermaid(html: str) -> str:
    pattern = re.compile(
        r'<pre><code class="language-mermaid">(.*?)</code></pre>',
        re.DOTALL,
    )

    def to_mermaid_div(match: re.Match[str]) -> str:
        body = match.group(1)
        body = body.replace("&lt;", "<").replace("&gt;", ">").replace("&amp;", "&")
        return f'<div class="mermaid-wrap"><div class="mermaid">{body}</div></div>'

    return pattern.sub(to_mermaid_div, html)


def convert_md_to_html(text: str) -> str:
    md = markdown.Markdown(
        extensions=[
            TableExtension(),
            FencedCodeExtension(),
            TocExtension(permalink=False, toc_depth=3),
        ]
    )
    html = md.convert(text)
    html = fix_mermaid(html)
    html = rewrite_links(html)
    return html


def build_page(src: Path, out_name: str, title: str) -> None:
    text = src.read_text(encoding="utf-8")
    body = convert_md_to_html(text)
    description = re.sub(r"\s+", " ", text.split("\n", 1)[0].lstrip("# ").strip())[:200]
    page = TEMPLATE.format(
        title=title,
        description=description,
        body=body,
        programming_guide=f"{PROGRAMMING_GUIDE_SITE}/index.html",
    )
    out_path = OUT_DIR / out_name
    out_path.write_text(page, encoding="utf-8")
    print(f"Wrote {out_path.relative_to(DOCS_ROOT)}")


def main() -> None:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    for src, out_name, title in PAGES:
        if not src.is_file():
            print(f"Skip missing: {src}", file=sys.stderr)
            continue
        build_page(src, out_name, title)
    print(f"Done — {len(PAGES)} pages in {OUT_DIR.relative_to(DOCS_ROOT)}/")


if __name__ == "__main__":
    main()

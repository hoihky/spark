#!/usr/bin/env python3
"""Rebuild programming-guide/site HTML from all sibling .md sources."""

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
    print("Missing dependency: pip install markdown", file=sys.stderr)
    sys.exit(1)

ROOT = Path(__file__).resolve().parent
SITE = ROOT / "site"
DOCS_ROOT = ROOT.parent
BODY_START = '<div class="doc-body">'
BODY_END = '</div>\n          <footer class="doc-footer">'
NAV_START = '<ul class="nav-list">'
NAV_END = "</ul>\n        </nav>"

PART_ORDER = [
    "1-overview-architecture",
    "2-2d-graphics",
    "3-3d-graphics",
    "4-ai",
    "5-physics",
    "6-sound",
    "7-2d-game",
    "8-3d-game",
]


def md_to_html(text: str) -> str:
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
    return html


def nav_label(stem: str) -> str:
    return stem.replace("-", " ")


def group_label(part: str) -> str:
    return part.replace("-", " ")


def collect_chapters() -> dict[str, list[str]]:
    by_part: dict[str, list[str]] = {}
    for md_path in sorted(ROOT.rglob("*.md")):
        if "/site/" in md_path.as_posix():
            continue
        rel = md_path.relative_to(ROOT)
        if len(rel.parts) < 2:
            continue
        part = rel.parts[0]
        by_part.setdefault(part, []).append(rel.stem)
    for part in by_part:
        by_part[part].sort()
    return by_part


def href_for_page(page_rel: Path, target_part: str, target_stem: str) -> str:
    if page_rel.name == "index.html":
        prefix = ""
    else:
        prefix = "../"
    return f'{prefix}{target_part}/{target_stem}.html'


def build_nav_list(page_rel: Path, by_part: dict[str, list[str]], active: str | None) -> str:
    lines = ["            "]
    index_active = " is-active" if page_rel.name == "index.html" else ""
    lines.append(
        f'                        <li class="nav-item depth-0{index_active}">'
    )
    lines.append('                          ')
    if page_rel.name == "index.html":
        lines.append('                            <a href="index.html" class="nav-link">index</a>')
    else:
        lines.append('                            <a href="../index.html" class="nav-link">index</a>')
    lines.append("                            ")
    lines.append("                        </li>")
    lines.append("                        ")

    for part in PART_ORDER:
        stems = by_part.get(part)
        if not stems:
            continue
        lines.append('                        <li class="nav-item depth-0 is-group">')
        lines.append("                          ")
        lines.append(f'                            <span class="nav-group">{group_label(part)}</span>')
        lines.append("                            ")
        lines.append("                        </li>")
        lines.append("                        ")
        for stem in stems:
            link = href_for_page(page_rel, part, stem)
            is_active = active == f"{part}/{stem}"
            active_cls = " is-active" if is_active else ""
            lines.append(f'                        <li class="nav-item depth-1{active_cls}">')
            lines.append("                          ")
            lines.append(f'                            <a href="{link}" class="nav-link">{nav_label(stem)}</a>')
            lines.append("                            ")
            lines.append("                        </li>")
            lines.append("                        ")
    return "\n".join(lines)


def replace_nav(html: str, new_nav_inner: str) -> str:
    start = html.find(NAV_START)
    end = html.find(NAV_END)
    if start < 0 or end < 0:
        return html
    return html[: start + len(NAV_START)] + "\n" + new_nav_inner + "\n          " + html[end:]


def title_from_md(md_path: Path) -> str:
    for line in md_path.read_text(encoding="utf-8").splitlines():
        if line.startswith("# "):
            return line[2:].strip()
    return nav_label(md_path.stem)


def scaffold_html(md_path: Path, template_path: Path) -> str:
    rel = md_path.relative_to(ROOT)
    part, stem = rel.parts[0], rel.stem
    html_rel = Path(part) / f"{stem}.html"
    title = title_from_md(md_path)
    page = template_path.read_text(encoding="utf-8")
    page = re.sub(r"<title>[^<]*</title>", f"<title>{nav_label(stem)} · Spark Game Engine Programming Guide</title>", page)
    page = re.sub(r"<h1>[^<]*</h1>", f"<h1>{title}</h1>", page, count=1)
    if BODY_START in page and BODY_END in page:
        start = page.find(BODY_START)
        end = page.find(BODY_END)
        page = page[: start + len(BODY_START)] + "<p>Loading…</p>" + page[end:]
    return page


def patch_body(html: str, md_path: Path) -> str:
    start = html.find(BODY_START)
    end = html.find(BODY_END)
    if start < 0 or end < 0:
        return html
    body = md_to_html(md_path.read_text(encoding="utf-8"))
    return html[: start + len(BODY_START)] + body + html[end:]


def main() -> None:
    by_part = collect_chapters()
    default_template = SITE / "2-2d-graphics" / "06-2d-render-pipeline.html"
    if not default_template.is_file():
        print(f"Missing template: {default_template}", file=sys.stderr)
        sys.exit(1)

    # index.md
    index_md = ROOT / "index.md"
    index_html = SITE / "index.html"
    if index_md.is_file() and index_html.is_file():
        nav = build_nav_list(Path("index.html"), by_part, None)
        page = replace_nav(index_html.read_text(encoding="utf-8"), nav)
        page = patch_body(page, index_md)
        index_html.write_text(page, encoding="utf-8")
        print(f"Patched {index_html.relative_to(ROOT)}")

    for part, stems in by_part.items():
        part_dir = SITE / part
        part_dir.mkdir(parents=True, exist_ok=True)
        sibling_template = None
        for existing in sorted(part_dir.glob("*.html")):
            sibling_template = existing
            break
        template = sibling_template or default_template

        for stem in stems:
            md_path = ROOT / part / f"{stem}.md"
            html_path = part_dir / f"{stem}.html"
            active = f"{part}/{stem}"

            if not html_path.is_file():
                page = scaffold_html(md_path, template)
                print(f"Created {html_path.relative_to(ROOT)}")
            else:
                page = html_path.read_text(encoding="utf-8")

            nav = build_nav_list(Path(part) / f"{stem}.html", by_part, active)
            page = replace_nav(page, nav)
            page = patch_body(page, md_path)
            html_path.write_text(page, encoding="utf-8")
            print(f"Patched {html_path.relative_to(ROOT)}")

    ensure = DOCS_ROOT / "ensure-html-theme.py"
    if ensure.is_file():
        subprocess.run([sys.executable, str(ensure)], check=True)

    build_docs = DOCS_ROOT / "site" / "build-docs.py"
    if build_docs.is_file():
        subprocess.run([sys.executable, str(build_docs)], check=True)

    print("Done — programming guide site + docs/site/docs rebuilt.")


if __name__ == "__main__":
    main()

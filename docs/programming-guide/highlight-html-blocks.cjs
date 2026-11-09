#!/usr/bin/env node
/**
 * Post-process HTML: run highlight.js on <pre><code class="language-*"> blocks.
 * Reads HTML from stdin, writes highlighted HTML to stdout.
 */
const fs = require("fs");
const path = require("path");

const hljsPath = path.join(__dirname, "site/assets/vendor/highlight.min.js");
// eslint-disable-next-line no-eval
eval(
  fs.readFileSync(hljsPath, "utf8").replace("module.exports=hljs", "globalThis.hljs=hljs")
);

if (typeof hljs !== "undefined") {
  hljs.registerAliases(["cmake"], { languageName: "makefile" });
  hljs.registerAliases(["sh", "shell"], { languageName: "bash" });
  hljs.registerAliases(["c++", "hpp", "h"], { languageName: "cpp" });
  hljs.registerAliases(["cs", "c-sharp"], { languageName: "csharp" });
  hljs.registerAliases(["py", "python3"], { languageName: "python" });
}

const LANG_FALLBACK = {
  cmake: "makefile",
  text: "plaintext",
  txt: "plaintext",
};

function decodeHtml(s) {
  return s
    .replace(/&lt;/g, "<")
    .replace(/&gt;/g, ">")
    .replace(/&amp;/g, "&")
    .replace(/&quot;/g, '"');
}

const html = fs.readFileSync(0, "utf8");
const BLOCK_RE = /<pre><code class="([^"]+)">([\s\S]*?)<\/code><\/pre>/g;

function languageFromClass(cls) {
  const m = cls.match(/\blanguage-([\w#+.-]+)\b/);
  return m ? m[1] : null;
}

const out = html.replace(BLOCK_RE, (full, classAttr, inner) => {
  const lang = languageFromClass(classAttr);
  if (!lang) return full;
  if (lang === "mermaid") return full;
  if (inner.includes("hljs-") || inner.includes('class="hljs')) return full;

  const mapped = LANG_FALLBACK[lang] || lang;
  const code = decodeHtml(inner);
  let result;
  try {
    if (hljs.getLanguage(mapped)) {
      result = hljs.highlight(code, { language: mapped, ignoreIllegals: true });
    } else {
      result = hljs.highlightAuto(code, { language: mapped });
    }
  } catch {
    return full;
  }

  const useLang = result.language || mapped;
  const cls = `hljs language-${lang} language-${useLang}`;
  return `<pre><code class="${cls}" data-highlighted="yes">${result.value}</code></pre>`;
});

process.stdout.write(out);

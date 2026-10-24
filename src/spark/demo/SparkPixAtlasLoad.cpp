#include "spark/demo/SparkPixAtlasLoad.hpp"

#include "spark/config.hpp"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace Spark {

namespace {

const char* SkipWs(const char* p) noexcept {
    while (p != nullptr && *p != '\0' && std::isspace(static_cast<unsigned char>(*p)) != 0) {
        ++p;
    }
    return p;
}

bool MatchLiteral(const char*& p, const char* literal) noexcept {
    p = SkipWs(p);
    if (p == nullptr || literal == nullptr) {
        return false;
    }
    const std::size_t n = std::strlen(literal);
    if (std::strncmp(p, literal, n) != 0) {
        return false;
    }
    p += n;
    return true;
}

bool ParseJsonString(const char*& p, Utf8String& out) noexcept {
    p = SkipWs(p);
    if (p == nullptr || *p != '"') {
        return false;
    }
    ++p;
    out.Clear();
    while (*p != '\0' && *p != '"') {
        if (*p == '\\' && p[1] != '\0') {
            ++p;
        }
        const char ch[2] = {*p, '\0'};
        out.AppendUtf8(ch);
        ++p;
    }
    if (*p != '"') {
        return false;
    }
    ++p;
    return true;
}

bool ParseJsonNumber(const char*& p, float& out) noexcept {
    p = SkipWs(p);
    if (p == nullptr || *p == '\0') {
        return false;
    }
    char* end = nullptr;
    const float v = strtof(p, &end);
    if (end == p) {
        return false;
    }
    out = v;
    p = end;
    return true;
}

bool ParseJsonUInt(const char*& p, std::uint32_t& out) noexcept {
    float f = 0.0F;
    if (!ParseJsonNumber(p, f)) {
        return false;
    }
    if (f < 0.0F) {
        return false;
    }
    out = static_cast<std::uint32_t>(f);
    return true;
}

bool ParseJsonBool(const char*& p, bool& out) noexcept {
    p = SkipWs(p);
    if (MatchLiteral(p, "true")) {
        out = true;
        return true;
    }
    if (MatchLiteral(p, "false")) {
        out = false;
        return true;
    }
    return false;
}

void SkipJsonValue(const char*& p) noexcept {
    p = SkipWs(p);
    if (p == nullptr || *p == '\0') {
        return;
    }
    if (*p == '"') {
        Utf8String ignored{};
        (void)ParseJsonString(p, ignored);
        return;
    }
    if (*p == '{' || *p == '[') {
        const char open = *p;
        const char close = (open == '{') ? '}' : ']';
        int depth = 1;
        ++p;
        while (*p != '\0' && depth > 0) {
            if (*p == '"') {
                Utf8String ignored{};
                (void)ParseJsonString(p, ignored);
                continue;
            }
            if (*p == open) {
                ++depth;
            } else if (*p == close) {
                --depth;
            }
            ++p;
        }
        return;
    }
    while (*p != '\0' && *p != ',' && *p != '}' && *p != ']' && !std::isspace(static_cast<unsigned char>(*p))) {
        ++p;
    }
}

bool ReadFileText(const char* path, Array<char>& out) noexcept {
    if (path == nullptr || path[0] == '\0') {
        return false;
    }
    FILE* f = std::fopen(path, "rb");
    if (f == nullptr) {
        return false;
    }
    if (std::fseek(f, 0, SEEK_END) != 0) {
        std::fclose(f);
        return false;
    }
    const long sz = std::ftell(f);
    if (sz < 0) {
        std::fclose(f);
        return false;
    }
    if (std::fseek(f, 0, SEEK_SET) != 0) {
        std::fclose(f);
        return false;
    }
    out.Clear();
    out.Reserve(static_cast<std::size_t>(sz) + 1U);
    for (long i = 0; i < sz; ++i) {
        const int ch = std::fgetc(f);
        if (ch == EOF) {
            break;
        }
        out.PushBack(static_cast<char>(ch));
    }
    out.PushBack('\0');
    std::fclose(f);
    return out.GetSize() > 1U;
}

bool TryResolveAssetPath(const char* relativePath, char* outPath, std::size_t outPathSize) noexcept {
    if (relativePath == nullptr || relativePath[0] == '\0' || outPath == nullptr || outPathSize == 0U) {
        return false;
    }
    const char* roots[] = {SPARK_ASSETS_DIR, SPARK_BUILD_ASSETS_DIR, "assets", nullptr};
    for (std::size_t ri = 0; roots[ri] != nullptr; ++ri) {
        std::snprintf(outPath, outPathSize, "%s%s", roots[ri], relativePath);
        FILE* probe = std::fopen(outPath, "rb");
        if (probe != nullptr) {
            std::fclose(probe);
            return true;
        }
    }
    return false;
}

bool ParseClipObject(const char*& p, SparkPixAtlasClip& out) noexcept {
    p = SkipWs(p);
    if (!MatchLiteral(p, "{")) {
        return false;
    }
    while (true) {
        p = SkipWs(p);
        if (*p == '}') {
            ++p;
            break;
        }
        Utf8String key{};
        if (!ParseJsonString(p, key)) {
            return false;
        }
        if (!MatchLiteral(p, ":")) {
            return false;
        }
        if (key == Utf8String("name")) {
            if (!ParseJsonString(p, out.name)) {
                return false;
            }
        } else if (key == Utf8String("firstFrame")) {
            if (!ParseJsonUInt(p, out.firstFrame)) {
                return false;
            }
        } else if (key == Utf8String("frameCount")) {
            if (!ParseJsonUInt(p, out.frameCount)) {
                return false;
            }
        } else if (key == Utf8String("framesPerSecond")) {
            float fps = 8.0F;
            if (!ParseJsonNumber(p, fps)) {
                return false;
            }
            out.framesPerSecond = fps;
        } else if (key == Utf8String("loop")) {
            if (!ParseJsonBool(p, out.loop)) {
                return false;
            }
        } else {
            SkipJsonValue(p);
        }
        p = SkipWs(p);
        if (*p == ',') {
            ++p;
        }
    }
    return true;
}

bool ParseManifestJson(const char* json, SparkPixAtlasManifest& out) noexcept {
    if (json == nullptr) {
        return false;
    }
    const char* p = json;
    if (!MatchLiteral(p, "{")) {
        return false;
    }
    while (true) {
        p = SkipWs(p);
        if (*p == '}') {
            ++p;
            break;
        }
        Utf8String key{};
        if (!ParseJsonString(p, key)) {
            return false;
        }
        if (!MatchLiteral(p, ":")) {
            return false;
        }
        if (key == Utf8String("texture")) {
            if (!ParseJsonString(p, out.textureFileName)) {
                return false;
            }
        } else if (key == Utf8String("columns")) {
            if (!ParseJsonUInt(p, out.columns)) {
                return false;
            }
        } else if (key == Utf8String("rows")) {
            if (!ParseJsonUInt(p, out.rows)) {
                return false;
            }
        } else if (key == Utf8String("frameWidth")) {
            if (!ParseJsonUInt(p, out.frameWidth)) {
                return false;
            }
        } else if (key == Utf8String("frameHeight")) {
            if (!ParseJsonUInt(p, out.frameHeight)) {
                return false;
            }
        } else if (key == Utf8String("padding")) {
            if (!ParseJsonUInt(p, out.padding)) {
                return false;
            }
        } else if (key == Utf8String("nearestFilter")) {
            if (!ParseJsonBool(p, out.nearestFilter)) {
                return false;
            }
        } else if (key == Utf8String("clips")) {
            if (!MatchLiteral(p, "[")) {
                return false;
            }
            out.clips.Clear();
            while (true) {
                p = SkipWs(p);
                if (*p == ']') {
                    ++p;
                    break;
                }
                SparkPixAtlasClip clip{};
                if (!ParseClipObject(p, clip)) {
                    return false;
                }
                out.clips.PushBack(clip);
                p = SkipWs(p);
                if (*p == ',') {
                    ++p;
                }
            }
        } else {
            SkipJsonValue(p);
        }
        p = SkipWs(p);
        if (*p == ',') {
            ++p;
        }
    }
    return out.columns > 0U && out.rows > 0U && !out.textureFileName.IsEmpty();
}

}  // namespace

bool TryLoadSparkPixAtlasManifest(
        const char* manifestRelativePath,
        SparkPixAtlasManifest& out,
        Utf8String& outResolvedManifestPath) noexcept {
    out = SparkPixAtlasManifest{};
    outResolvedManifestPath.Clear();
    char pathBuf[768]{};
    if (!TryResolveAssetPath(manifestRelativePath, pathBuf, sizeof(pathBuf))) {
        return false;
    }
    Array<char> text{};
    if (!ReadFileText(pathBuf, text)) {
        return false;
    }
    if (!ParseManifestJson(text.GetData(), out)) {
        return false;
    }
    outResolvedManifestPath = Utf8String(pathBuf);
    return true;
}

bool TryLoadSparkPixAtlasTexture(
        const SparkPixAtlasManifest& manifest,
        const Utf8String& resolvedManifestPath,
        Texture2D& out) noexcept {
    if (manifest.textureFileName.IsEmpty() || resolvedManifestPath.IsEmpty()) {
        return false;
    }
    const char* manifestPath = resolvedManifestPath.CStr();
    const char* lastSlash = nullptr;
    for (const char* p = manifestPath; p != nullptr && *p != '\0'; ++p) {
        if (*p == '/' || *p == '\\') {
            lastSlash = p;
        }
    }
    char texPath[768]{};
    if (lastSlash != nullptr) {
        const int dirLen = static_cast<int>(lastSlash - manifestPath + 1);
        std::snprintf(
                texPath,
                sizeof(texPath),
                "%.*s%s",
                dirLen,
                manifestPath,
                manifest.textureFileName.CStr());
    } else {
        std::snprintf(texPath, sizeof(texPath), "%s", manifest.textureFileName.CStr());
    }
    if (!Texture2D::TryLoadFromFile(texPath, out, false)) {
        return false;
    }
    if (manifest.nearestFilter) {
        out.SetSceneUploadNearest(true);
    }
    return true;
}

void ApplySparkPixManifestToAnimator(
        const SparkPixAtlasManifest& manifest,
        SpriteAnimatorComponent& animator) noexcept {
    animator.SetUniformGrid(manifest.columns, manifest.rows);
    animator.ClearClips();
    for (std::size_t i = 0; i < manifest.clips.GetSize(); ++i) {
        const SparkPixAtlasClip& c = manifest.clips[i];
        animator.AddClip(SpriteAnimationClip{
                c.firstFrame,
                c.frameCount,
                c.framesPerSecond,
                c.loop});
    }
}

}  // namespace Spark

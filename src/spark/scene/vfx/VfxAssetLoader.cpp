#include "spark/scene/vfx/VfxAssetLoader.hpp"

#include "spark/config.hpp"
#include "spark/scene/assets/GameWorldAssetCache.hpp"
#include "spark/scene/vfx/VfxEffectDefinition.hpp"

#include <cstdio>
#include <cstring>
#include <sys/stat.h>

namespace Spark {

namespace {

Utf8String WithSuffix(const char* base, const char* suffix) {
    Utf8String out(base != nullptr ? base : "");
    out.AppendUtf8(suffix);
    return out;
}

Utf8String VfxRelativePath(const char* keyOrPath) {
    Utf8String out("vfx/");
    out.AppendUtf8(keyOrPath != nullptr ? keyOrPath : "");
    return out;
}

bool IsRegularFile(const char* path) noexcept {
    if (path == nullptr || path[0] == '\0') {
        return false;
    }
    struct stat st {};
    if (stat(path, &st) != 0) {
        return false;
    }
    return S_ISREG(st.st_mode);
}

bool FileExists(const char* path) noexcept {
    return IsRegularFile(path);
}

Utf8String TryExistingPath(const char* path) {
    if (FileExists(path)) {
        return Utf8String(path);
    }
    return {};
}

Utf8String JoinRootRelative(const char* root, const char* relative) {
    if (root == nullptr || relative == nullptr || relative[0] == '\0') {
        return {};
    }
    const char* rel = relative;
    while (rel[0] == '/') {
        ++rel;
    }
    char buf[1024]{};
    const int written = std::snprintf(buf, sizeof(buf), "%s/%s", root, rel);
    if (written <= 0 || static_cast<std::size_t>(written) >= sizeof(buf)) {
        return {};
    }
    return TryExistingPath(buf);
}

bool ParseBurstLine(const char* line, std::uint32_t& outBurst) {
    if (line == nullptr) {
        return false;
    }
    char tag[16]{};
    unsigned burst = 0;
    if (std::sscanf(line, "%15s %u", tag, &burst) != 2) {
        return false;
    }
    if (std::strcmp(tag, "burst") != 0) {
        return false;
    }
    outBurst = burst;
    return true;
}

bool ParseBuiltinLine(const char* line, VfxAsset& out) {
    if (line == nullptr) {
        return false;
    }
    char tag[16]{};
    char builtin[64]{};
    if (std::sscanf(line, "%15s %63s", tag, builtin) != 2) {
        return false;
    }
    if (std::strcmp(tag, "builtin") != 0) {
        return false;
    }
    out.builtinName = Utf8String(builtin);
    return true;
}

bool ParseCustomLine(const char* line, VfxAsset& out) {
    if (line == nullptr) {
        return false;
    }
    const char* cursor = line;
    char tag[16]{};
    if (std::sscanf(cursor, "%15s", tag) != 1 || std::strcmp(tag, "custom") != 0) {
        return false;
    }
    cursor += std::strlen("custom");
    while (*cursor == ' ') {
        ++cursor;
    }

    int enabled = 1;
    unsigned maxParticles = 512U;
    float emissionRate = 48.0F;
    float lifeMin = 0.8F;
    float lifeMax = 1.6F;
    float sizeStart = 0.14F;
    float sizeEnd = 0.02F;
    Vector4 colorStart{0.95F, 0.85F, 0.35F, 1.0F};
    Vector4 colorEnd{0.9F, 0.2F, 0.05F, 0.0F};
    Vector3 gravity{0.0F, -1.8F, 0.0F};
    Vector3 emissionDir{0.0F, 1.0F, 0.0F};
    int useLocal = 0;
    float spread = 0.55F;
    float speedMin = 1.2F;
    float speedMax = 2.8F;
    if (std::sscanf(
                cursor,
                "%d %u "
                "%f %f %f %f %f "
                "%f %f %f %f "
                "%f %f %f %f "
                "%f %f %f "
                "%f %f %f "
                "%d %f %f %f",
                &enabled,
                &maxParticles,
                &emissionRate,
                &lifeMin,
                &lifeMax,
                &sizeStart,
                &sizeEnd,
                &colorStart.x,
                &colorStart.y,
                &colorStart.z,
                &colorStart.w,
                &colorEnd.x,
                &colorEnd.y,
                &colorEnd.z,
                &colorEnd.w,
                &gravity.x,
                &gravity.y,
                &gravity.z,
                &emissionDir.x,
                &emissionDir.y,
                &emissionDir.z,
                &useLocal,
                &spread,
                &speedMin,
                &speedMax)
        < 25) {
        return false;
    }
    out.builtinName = {};
    out.emitter.enabled = enabled != 0;
    out.emitter.maxParticles = maxParticles;
    out.emitter.emissionRate = emissionRate;
    out.emitter.lifeMin = lifeMin;
    out.emitter.lifeMax = lifeMax;
    out.emitter.sizeStart = sizeStart;
    out.emitter.sizeEnd = sizeEnd;
    out.emitter.colorStart = colorStart;
    out.emitter.colorEnd = colorEnd;
    out.emitter.gravity = gravity;
    out.emitter.emissionDir = emissionDir;
    out.emitter.useLocalEmission = useLocal != 0;
    out.emitter.spreadRadians = spread;
    out.emitter.speedMin = speedMin;
    out.emitter.speedMax = speedMax;
    return true;
}

bool ParsePrefabLine(const char* line, Utf8String& outPath) {
    if (line == nullptr) {
        return false;
    }
    char tag[16]{};
    char path[384]{};
    if (std::sscanf(line, "%15s %383s", tag, path) != 2) {
        return false;
    }
    if (std::strcmp(tag, "prefab") != 0) {
        return false;
    }
    outPath = Utf8String(path);
    return true;
}

bool ParsePhaseLine(const char* line, VfxEmitterSpec& out) {
    if (line == nullptr) {
        return false;
    }
    char tag[16]{};
    float start = 0.0F;
    char kind[16]{};
    char builtin[64]{};
    if (std::sscanf(line, "%15s %f %15s %63s", tag, &start, kind, builtin) != 4) {
        return false;
    }
    if (std::strcmp(tag, "phase") != 0 || std::strcmp(kind, "builtin") != 0) {
        return false;
    }
    out = VfxEmitterSpec{};
    out.startTimeSeconds = start;
    out.builtinName = Utf8String(builtin);
    unsigned burst = 0;
    float duration = 0.0F;
    const char* burstPos = std::strstr(line, "burst");
    if (burstPos != nullptr) {
        std::sscanf(burstPos, "burst %u", &burst);
        out.burstCount = burst;
    }
    const char* durationPos = std::strstr(line, "duration");
    if (durationPos != nullptr) {
        std::sscanf(durationPos, "duration %f", &duration);
        out.durationSeconds = duration;
    }
    return true;
}

bool ParseCompositeMarker(const char* line) {
    if (line == nullptr) {
        return false;
    }
    char tag[16]{};
    if (std::sscanf(line, "%15s", tag) != 1) {
        return false;
    }
    return std::strcmp(tag, "composite") == 0;
}

bool TryDecodeBody(const char* diskPath, VfxAsset& out) {
    std::FILE* file = std::fopen(diskPath, "rb");
    if (file == nullptr) {
        return false;
    }
    char header[32]{};
    if (std::fscanf(file, "%31s", header) != 1 || std::strcmp(header, "sparkvfx_v1") != 0) {
        std::fclose(file);
        return false;
    }

    char line[1024]{};
    bool parsedBody = false;
    bool compositeMode = false;
    while (std::fgets(line, static_cast<int>(sizeof(line)), file) != nullptr) {
        if (line[0] == '\n' || line[0] == '\r') {
            continue;
        }
        if (ParseCompositeMarker(line)) {
            compositeMode = true;
            parsedBody = true;
            continue;
        }
        if (ParsePrefabLine(line, out.definition.prefabScenePath)) {
            parsedBody = true;
            continue;
        }
        if (compositeMode) {
            VfxEmitterSpec spec{};
            if (ParsePhaseLine(line, spec)) {
                out.definition.emitters.PushBack(spec);
                parsedBody = true;
            }
            continue;
        }
        if (ParseBurstLine(line, out.burstCount)) {
            continue;
        }
        if (ParseBuiltinLine(line, out) || ParseCustomLine(line, out)) {
            parsedBody = true;
            continue;
        }
    }
    std::fclose(file);
    if (!parsedBody) {
        return false;
    }
    if (diskPath != nullptr) {
        out.name = Utf8String(diskPath);
    }
    return true;
}

}  // namespace

Utf8String VfxAssetLoader::ResolveReadablePath(const char* keyOrPath) {
    if (keyOrPath == nullptr || keyOrPath[0] == '\0') {
        return {};
    }
    if (Utf8String direct = TryExistingPath(keyOrPath); !direct.IsEmpty()) {
        return direct;
    }
    if (Utf8String withExt = TryExistingPath(WithSuffix(keyOrPath, ".sparkvfx").CStr()); !withExt.IsEmpty()) {
        return withExt;
    }
    const Utf8String vfxPath = VfxRelativePath(keyOrPath);
    if (Utf8String nested = TryExistingPath(WithSuffix(vfxPath.CStr(), ".sparkvfx").CStr()); !nested.IsEmpty()) {
        return nested;
    }
    if (Utf8String build = JoinRootRelative(SPARK_BUILD_ASSETS_DIR, keyOrPath); !build.IsEmpty()) {
        return build;
    }
    if (Utf8String buildExt = JoinRootRelative(
                SPARK_BUILD_ASSETS_DIR,
                WithSuffix(keyOrPath, ".sparkvfx").CStr());
        !buildExt.IsEmpty()) {
        return buildExt;
    }
    if (Utf8String buildVfx = JoinRootRelative(
                SPARK_BUILD_ASSETS_DIR,
                WithSuffix(vfxPath.CStr(), ".sparkvfx").CStr());
        !buildVfx.IsEmpty()) {
        return buildVfx;
    }
    if (Utf8String source = JoinRootRelative(SPARK_ASSETS_DIR, keyOrPath); !source.IsEmpty()) {
        return source;
    }
    if (Utf8String sourceExt = JoinRootRelative(
                SPARK_ASSETS_DIR,
                WithSuffix(keyOrPath, ".sparkvfx").CStr());
        !sourceExt.IsEmpty()) {
        return sourceExt;
    }
    if (Utf8String sourceVfx = JoinRootRelative(
                SPARK_ASSETS_DIR,
                WithSuffix(vfxPath.CStr(), ".sparkvfx").CStr());
        !sourceVfx.IsEmpty()) {
        return sourceVfx;
    }
    return {};
}

AssetLoadOutcome<VfxAsset> VfxAssetLoader::TryLoadFromFile(const char* path, GameWorldAssetCache& cache) {
    (void)cache;
    AssetLoadOutcome<VfxAsset> outcome{};
    if (path != nullptr) {
        outcome.path = Utf8String(path);
    }
    const Utf8String resolved = ResolveReadablePath(path);
    if (resolved.IsEmpty()) {
        outcome.errorMessage = Utf8String("VFX asset not found");
        return outcome;
    }
    VfxAsset asset{};
    if (!TryDecodeBody(resolved.CStr(), asset)) {
        outcome.errorMessage = Utf8String("Failed to parse .sparkvfx");
        return outcome;
    }
    outcome.ok = true;
    outcome.value = asset;
    return outcome;
}

bool VfxAssetLoader::TrySaveToFile(const char* path, const VfxAsset& asset, const char* /*relativeToDir*/) {
    if (path == nullptr || path[0] == '\0') {
        return false;
    }
    std::FILE* file = std::fopen(path, "wb");
    if (file == nullptr) {
        return false;
    }
    std::fprintf(file, "sparkvfx_v1\n");
    if (!asset.definition.prefabScenePath.IsEmpty()) {
        std::fprintf(file, "prefab %s\n", asset.definition.prefabScenePath.CStr());
    }
    if (asset.IsComposite()) {
        std::fprintf(file, "composite\n");
        for (std::size_t i = 0; i < asset.definition.emitters.GetSize(); ++i) {
            const VfxEmitterSpec& spec = asset.definition.emitters[i];
            if (spec.durationSeconds > 0.0F && spec.burstCount == 0) {
                std::fprintf(
                        file,
                        "phase %.3f builtin %s duration %.3f\n",
                        spec.startTimeSeconds,
                        spec.builtinName.CStr(),
                        spec.durationSeconds);
            } else if (spec.burstCount > 0) {
                std::fprintf(
                        file,
                        "phase %.3f builtin %s burst %u\n",
                        spec.startTimeSeconds,
                        spec.builtinName.CStr(),
                        spec.burstCount);
            } else {
                std::fprintf(
                        file,
                        "phase %.3f builtin %s\n",
                        spec.startTimeSeconds,
                        spec.builtinName.CStr());
            }
        }
    } else if (!asset.builtinName.IsEmpty()) {
        std::fprintf(file, "builtin %s\n", asset.builtinName.CStr());
    } else {
        const VfxEmitterParams& e = asset.emitter;
        char body[512]{};
        const int written = std::snprintf(
                body,
                sizeof(body),
                "custom %d %u "
                "%.6f %.6f %.6f %.6f %.6f "
                "%.6f %.6f %.6f %.6f "
                "%.6f %.6f %.6f %.6f "
                "%.6f %.6f %.6f "
                "%.6f %.6f %.6f "
                "%d %.6f %.6f %.6f",
                e.enabled ? 1 : 0,
                e.maxParticles,
                e.emissionRate,
                e.lifeMin,
                e.lifeMax,
                e.sizeStart,
                e.sizeEnd,
                e.colorStart.x,
                e.colorStart.y,
                e.colorStart.z,
                e.colorStart.w,
                e.colorEnd.x,
                e.colorEnd.y,
                e.colorEnd.z,
                e.colorEnd.w,
                e.gravity.x,
                e.gravity.y,
                e.gravity.z,
                e.emissionDir.x,
                e.emissionDir.y,
                e.emissionDir.z,
                e.useLocalEmission ? 1 : 0,
                e.spreadRadians,
                e.speedMin,
                e.speedMax);
        if (written <= 0 || static_cast<std::size_t>(written) >= sizeof(body)) {
            std::fclose(file);
            return false;
        }
        std::fprintf(file, "%s\n", body);
    }
    if (asset.burstCount > 0) {
        std::fprintf(file, "burst %u\n", asset.burstCount);
    }
    std::fclose(file);
    return true;
}

}  // namespace Spark

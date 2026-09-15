#include "spark/scene/water/WaterPresetAssetLoader.hpp"

#include "spark/config.hpp"
#include "spark/scene/assets/ScenePathResolver.hpp"
#include "spark/scene/water/GerstnerWave.hpp"

#include <cstdio>
#include <cstring>
#include <sys/stat.h>

namespace Spark {

namespace {

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

Utf8String WithSuffix(const char* base, const char* suffix) {
    Utf8String out(base != nullptr ? base : "");
    out.AppendUtf8(suffix);
    return out;
}

Utf8String WaterRelativePath(const char* keyOrPath) {
    Utf8String out("water/");
    out.AppendUtf8(keyOrPath != nullptr ? keyOrPath : "");
    return out;
}

bool TryParsePresetName(const char* name, WaterWavePresetId& outId) noexcept {
    if (name == nullptr) {
        return false;
    }
    if (std::strcmp(name, "CalmLake") == 0) {
        outId = WaterWavePresetId::CalmLake;
        return true;
    }
    if (std::strcmp(name, "OceanModerate") == 0) {
        outId = WaterWavePresetId::OceanModerate;
        return true;
    }
    if (std::strcmp(name, "StormySea") == 0) {
        outId = WaterWavePresetId::StormySea;
        return true;
    }
    return false;
}

bool ParseWaveLine(const char* line, GerstnerWave& outWave) {
    if (line == nullptr) {
        return false;
    }
    char tag[16]{};
    float dirX = 0.0F;
    float dirZ = 0.0F;
    float amplitude = 0.0F;
    float wavelength = 0.0F;
    float speed = 0.0F;
    float steepness = 0.0F;
    if (std::sscanf(line, "%15s %f %f %f %f %f %f", tag, &dirX, &dirZ, &amplitude, &wavelength, &speed, &steepness) != 7) {
        return false;
    }
    if (std::strcmp(tag, "wave") != 0) {
        return false;
    }
    outWave = GerstnerWave::FromDirection(dirX, dirZ, amplitude, wavelength, speed, steepness);
    return true;
}

WaterWaveSettings BuildEmbeddedPreset(const WaterWavePresetId preset, const float globalWindSpeed) noexcept {
    WaterWaveSettings settings{};
    settings.SetGlobalWindSpeed(globalWindSpeed);

    switch (preset) {
    case WaterWavePresetId::CalmLake:
        settings.GetWave(0) = GerstnerWave::FromDirection(1.0F, 0.15F, 0.045F, 9.0F, 0.55F, 0.35F);
        settings.GetWave(1) = GerstnerWave::FromDirection(-0.35F, 0.92F, 0.028F, 5.5F, 0.72F, 0.30F);
        settings.GetWave(2) = GerstnerWave::FromDirection(0.62F, -0.78F, 0.016F, 3.2F, 0.95F, 0.25F);
        settings.SetActiveWaveCount(3);
        break;
    case WaterWavePresetId::OceanModerate:
        settings.GetWave(0) = GerstnerWave::FromDirection(0.92F, 0.38F, 0.12F, 14.0F, 0.85F, 0.45F);
        settings.GetWave(1) = GerstnerWave::FromDirection(-0.48F, 0.88F, 0.075F, 8.0F, 1.05F, 0.40F);
        settings.GetWave(2) = GerstnerWave::FromDirection(0.22F, -0.97F, 0.04F, 4.5F, 1.25F, 0.35F);
        settings.GetWave(3) = GerstnerWave::FromDirection(0.71F, 0.71F, 0.025F, 2.8F, 1.45F, 0.28F);
        settings.SetActiveWaveCount(4);
        break;
    case WaterWavePresetId::StormySea:
        settings.GetWave(0) = GerstnerWave::FromDirection(0.78F, 0.62F, 0.22F, 18.0F, 1.35F, 0.55F);
        settings.GetWave(1) = GerstnerWave::FromDirection(-0.62F, 0.78F, 0.14F, 10.0F, 1.55F, 0.50F);
        settings.GetWave(2) = GerstnerWave::FromDirection(0.35F, -0.94F, 0.09F, 5.5F, 1.85F, 0.45F);
        settings.GetWave(3) = GerstnerWave::FromDirection(-0.88F, -0.47F, 0.06F, 3.5F, 2.05F, 0.40F);
        settings.SetActiveWaveCount(4);
        break;
    }

    if (globalWindSpeed != 1.0F && globalWindSpeed > 0.0F) {
        for (std::size_t i = 0; i < settings.GetActiveWaveCount(); ++i) {
            settings.GetWave(i).speed *= globalWindSpeed;
        }
    }
    return settings;
}

}  // namespace

const char* WaterPresetAssetLoader::PresetFileName(const WaterWavePresetId presetId) noexcept {
    switch (presetId) {
    case WaterWavePresetId::CalmLake:
        return "calm_lake.sparkwater";
    case WaterWavePresetId::OceanModerate:
        return "ocean_moderate.sparkwater";
    case WaterWavePresetId::StormySea:
        return "stormy_sea.sparkwater";
    }
    return "calm_lake.sparkwater";
}

Utf8String WaterPresetAssetLoader::ResolveReadablePath(const char* keyOrPath) {
    if (keyOrPath == nullptr || keyOrPath[0] == '\0') {
        return {};
    }
    if (IsRegularFile(keyOrPath)) {
        return Utf8String(keyOrPath);
    }

    const Utf8String buildPath = ScenePathResolver::BuildRuntimePath("water", keyOrPath);
    if (IsRegularFile(buildPath.CStr())) {
        return buildPath;
    }

    const Utf8String withExt = ScenePathResolver::BuildRuntimePath("water", WithSuffix(keyOrPath, ".sparkwater").CStr());
    if (IsRegularFile(withExt.CStr())) {
        return withExt;
    }

    const Utf8String nested = ScenePathResolver::ResolveReadablePath(WaterRelativePath(keyOrPath).CStr());
    if (!nested.IsEmpty()) {
        return nested;
    }

    return ScenePathResolver::ResolveReadablePath(WithSuffix(WaterRelativePath(keyOrPath).CStr(), ".sparkwater").CStr());
}

AssetLoadOutcome<WaterPresetAsset> WaterPresetAssetLoader::TryLoadFromFile(const char* path) {
    AssetLoadOutcome<WaterPresetAsset> outcome{};
    const Utf8String resolved = ResolveReadablePath(path);
    if (resolved.IsEmpty()) {
        outcome.errorMessage = Utf8String("Water preset file not found");
        return outcome;
    }

    std::FILE* file = std::fopen(resolved.CStr(), "r");
    if (file == nullptr) {
        outcome.errorMessage = Utf8String("Failed to open water preset file");
        return outcome;
    }

    char header[32]{};
    if (std::fscanf(file, "%31s", header) != 1 || std::strcmp(header, "sparkwater_v1") != 0) {
        std::fclose(file);
        outcome.errorMessage = Utf8String("Invalid sparkwater header");
        return outcome;
    }

    WaterPresetAsset asset{};
    char line[256]{};
    while (std::fgets(line, sizeof(line), file) != nullptr) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') {
            continue;
        }

        char tag[32]{};
        if (std::sscanf(line, "%31s", tag) != 1) {
            continue;
        }

        if (std::strcmp(tag, "preset") == 0) {
            char presetName[64]{};
            if (std::sscanf(line, "%*s %63s", presetName) == 1) {
                TryParsePresetName(presetName, asset.presetId);
            }
            continue;
        }

        if (std::strcmp(tag, "globalWindSpeed") == 0) {
            float wind = 1.0F;
            if (std::sscanf(line, "%*s %f", &wind) == 1) {
                asset.settings.SetGlobalWindSpeed(wind);
            }
            continue;
        }

        if (std::strcmp(tag, "wave") == 0) {
            if (asset.settings.GetActiveWaveCount() >= WaterWaveSettings::kMaxWaves) {
                continue;
            }
            GerstnerWave wave{};
            if (ParseWaveLine(line, wave)) {
                asset.settings.GetWave(asset.settings.GetActiveWaveCount()) = wave;
                asset.settings.SetActiveWaveCount(asset.settings.GetActiveWaveCount() + 1U);
            }
        }
    }

    std::fclose(file);
    if (asset.settings.GetActiveWaveCount() == 0U) {
        outcome.errorMessage = Utf8String("Water preset contained no waves");
        return outcome;
    }

    outcome.ok = true;
    outcome.path = resolved;
    outcome.value = asset;
    return outcome;
}

AssetLoadOutcome<WaterPresetAsset> WaterPresetAssetLoader::TryLoadPreset(const WaterWavePresetId presetId) noexcept {
    const Utf8String path = ResolveReadablePath(PresetFileName(presetId));
    if (!path.IsEmpty()) {
        AssetLoadOutcome<WaterPresetAsset> loaded = TryLoadFromFile(path.CStr());
        if (loaded.ok) {
            return loaded;
        }
    }

    AssetLoadOutcome<WaterPresetAsset> fallback{};
    fallback.ok = true;
    fallback.value.presetId = presetId;
    fallback.value.settings = BuildEmbeddedPreset(presetId, 1.0F);
    return fallback;
}

}  // namespace Spark

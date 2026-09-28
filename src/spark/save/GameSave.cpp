#include "spark/save/GameSave.hpp"

#include "spark/config.hpp"

#include <cstdio>
#include <cstring>
#include <filesystem>

namespace Spark {

namespace {

constexpr const char* kMagic = "spark_save_v1";

bool ReadFloat(const char* line, const char* key, float& out) noexcept {
    const std::size_t keyLen = std::strlen(key);
    if (std::strncmp(line, key, keyLen) != 0 || line[keyLen] != ' ') {
        return false;
    }
    out = strtof(line + keyLen + 1, nullptr);
    return true;
}

bool ReadInt(const char* line, const char* key, int& out) noexcept {
    const std::size_t keyLen = std::strlen(key);
    if (std::strncmp(line, key, keyLen) != 0 || line[keyLen] != ' ') {
        return false;
    }
    out = static_cast<int>(strtol(line + keyLen + 1, nullptr, 10));
    return true;
}

bool ReadBool01(const char* line, const char* key, bool& out) noexcept {
    int v = 0;
    if (!ReadInt(line, key, v)) {
        return false;
    }
    out = v != 0;
    return true;
}

}  // namespace

Utf8String GameSave::DefaultSlotPath(const char* slotName) noexcept {
    Utf8String path(SPARK_BUILD_ASSETS_DIR);
    path.AppendUtf8("/save/");
    path.AppendUtf8(slotName != nullptr ? slotName : "slot0");
    path.AppendUtf8(".savespark");
    return path;
}

bool GameSave::TryLoad(const char* path, GameSaveSlot& out) noexcept {
    if (path == nullptr || path[0] == '\0') {
        return false;
    }
    FILE* file = std::fopen(path, "r");
    if (file == nullptr) {
        return false;
    }
    char line[512]{};
    if (std::fgets(line, sizeof(line), file) == nullptr || std::strncmp(line, kMagic, std::strlen(kMagic)) != 0) {
        std::fclose(file);
        return false;
    }
    out = GameSaveSlot{};
    out.formatVersion = GameSaveSlot::kFormatVersion;
    char levelId[256]{};
    bool gotLevel = false;
    while (std::fgets(line, sizeof(line), file) != nullptr) {
        if (line[0] == '\n' || line[0] == '\r') {
            continue;
        }
        int version = 0;
        if (ReadInt(line, "version", version)) {
            out.formatVersion = version;
            continue;
        }
        if (std::strncmp(line, "level_id ", 9) == 0) {
            std::snprintf(levelId, sizeof(levelId), "%s", line + 9);
            for (char* c = levelId; *c != '\0'; ++c) {
                if (*c == '\n' || *c == '\r') {
                    *c = '\0';
                    break;
                }
            }
            gotLevel = true;
            continue;
        }
        (void)ReadInt(line, "gems_collected", out.progress.gemsCollected);
        (void)ReadInt(line, "gems_total", out.progress.gemsTotal);
        (void)ReadInt(line, "enemies_defeated", out.progress.enemiesDefeated);
        (void)ReadBool01(line, "level_complete", out.progress.levelComplete);
        (void)ReadFloat(line, "master_volume", out.settings.masterVolume);
        (void)ReadFloat(line, "music_volume", out.settings.musicVolume);
        (void)ReadFloat(line, "sfx_volume", out.settings.sfxVolume);
    }
    std::fclose(file);
    if (gotLevel) {
        out.progress.activeLevelId = Utf8String(levelId);
    }
    return true;
}

bool GameSave::TrySave(const char* path, const GameSaveSlot& slot) noexcept {
    if (path == nullptr || path[0] == '\0') {
        return false;
    }
    const std::filesystem::path filePath(path);
    if (filePath.has_parent_path()) {
        std::error_code ec{};
        std::filesystem::create_directories(filePath.parent_path(), ec);
    }
    FILE* file = std::fopen(path, "w");
    if (file == nullptr) {
        return false;
    }
    std::fprintf(file, "%s\n", kMagic);
    std::fprintf(file, "version %d\n", slot.formatVersion);
    std::fprintf(file, "level_id %s\n", slot.progress.activeLevelId.CStr());
    std::fprintf(file, "gems_collected %d\n", slot.progress.gemsCollected);
    std::fprintf(file, "gems_total %d\n", slot.progress.gemsTotal);
    std::fprintf(file, "enemies_defeated %d\n", slot.progress.enemiesDefeated);
    std::fprintf(file, "level_complete %d\n", slot.progress.levelComplete ? 1 : 0);
    std::fprintf(file, "master_volume %.6f\n", slot.settings.masterVolume);
    std::fprintf(file, "music_volume %.6f\n", slot.settings.musicVolume);
    std::fprintf(file, "sfx_volume %.6f\n", slot.settings.sfxVolume);
    std::fclose(file);
    return true;
}

}  // namespace Spark

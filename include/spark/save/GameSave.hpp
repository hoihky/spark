#pragma once

#include "spark/core/Utf8String.hpp"

namespace Spark {

/** Versioned player progress (inventory flags, level unlocks, run stats). */
struct GameProgressRecord {
    Utf8String activeLevelId{"platformer"};
    int gemsCollected = 0;
    int gemsTotal = 0;
    int enemiesDefeated = 0;
    bool levelComplete = false;
};

/** User settings persisted across sessions. */
struct GameSettingsRecord {
    float masterVolume = 1.0F;
    float musicVolume = 0.28F;
    float sfxVolume = 1.0F;
};

/** Slot payload written by <c>GameSave</c> (Facade over file I/O). */
struct GameSaveSlot {
    static constexpr int kFormatVersion = 1;

    int formatVersion = kFormatVersion;
    GameProgressRecord progress{};
    GameSettingsRecord settings{};
};

/**
 * Minimal persistence API (Strategy: line-oriented text file, no third-party JSON).
 * Path is caller-provided (e.g. under user writable dir or next to executable).
 */
class GameSave final {
public:
    [[nodiscard]] static bool TryLoad(const char* path, GameSaveSlot& out) noexcept;
    [[nodiscard]] static bool TrySave(const char* path, const GameSaveSlot& slot) noexcept;

    [[nodiscard]] static Utf8String DefaultSlotPath(const char* slotName = "slot0") noexcept;
};

}  // namespace Spark

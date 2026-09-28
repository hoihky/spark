#pragma once

#include "spark/save/GameSave.hpp"

namespace Spark {

/**
 * Data that survives gameplay scene unload/reload (lives outside ECS).
 * Sync to <c>GameSaveSlot</c> on checkpoints.
 */
struct GameFlowPersistentData {
    GameProgressRecord progress{};
    GameSettingsRecord settings{};

    void ApplyFromSlot(const GameSaveSlot& slot) noexcept;
    void WriteToSlot(GameSaveSlot& slot) const noexcept;
};

}  // namespace Spark

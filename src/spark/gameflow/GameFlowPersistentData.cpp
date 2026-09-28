#include "spark/gameflow/GameFlowPersistentData.hpp"

namespace Spark {

void GameFlowPersistentData::ApplyFromSlot(const GameSaveSlot& slot) noexcept {
    progress = slot.progress;
    settings = slot.settings;
}

void GameFlowPersistentData::WriteToSlot(GameSaveSlot& slot) const noexcept {
    slot.progress = progress;
    slot.settings = settings;
}

}  // namespace Spark

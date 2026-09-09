#include "spark/scene/submit/SkinnedAnimationBudget.hpp"

namespace Spark {

void SkinnedAnimationBudget::BeginFrame() noexcept {
    paletteUpdatesUsed = 0;
    skinnedDrawsSubmitted = 0;
    paletteCacheHits = 0;
    paletteBindPoseFallbacks = 0;
}

bool SkinnedAnimationBudget::CanComputePalette() const noexcept {
    return maxPaletteUpdates == 0 || paletteUpdatesUsed < maxPaletteUpdates;
}

bool SkinnedAnimationBudget::CanSubmitSkinnedDraw() const noexcept {
    return maxSkinnedDraws == 0 || skinnedDrawsSubmitted < maxSkinnedDraws;
}

bool SkinnedAnimationBudget::ConsumePaletteUpdate() noexcept {
    if (!CanComputePalette()) {
        return false;
    }
    ++paletteUpdatesUsed;
    return true;
}

bool SkinnedAnimationBudget::ConsumeSkinnedDraw() noexcept {
    if (!CanSubmitSkinnedDraw()) {
        return false;
    }
    ++skinnedDrawsSubmitted;
    return true;
}

}  // namespace Spark

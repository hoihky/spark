#pragma once

#include "spark/ecs/GameComponent.hpp"

#include <cstdint>

namespace Spark {

/**
 * Optional world policy for skinned animation performance (attach to a scene root object).
 * <c>SceneSubmit</c> reads the first active instance each frame.
 */
class SkinnedAnimationBudgetComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::SkinnedAnimationBudget;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    [[nodiscard]] std::uint32_t GetMaxPaletteUpdatesPerFrame() const noexcept { return maxPaletteUpdatesPerFrame; }
    [[nodiscard]] std::uint32_t GetMaxSkinnedDrawsPerFrame() const noexcept { return maxSkinnedDrawsPerFrame; }
    [[nodiscard]] bool IsPaletteCacheEnabled() const noexcept { return paletteCacheEnabled; }
    [[nodiscard]] float GetPaletteQuantizationHz() const noexcept { return paletteQuantizationHz; }

    void SetMaxPaletteUpdatesPerFrame(std::uint32_t limit) noexcept { maxPaletteUpdatesPerFrame = limit; }
    void SetMaxSkinnedDrawsPerFrame(std::uint32_t limit) noexcept { maxSkinnedDrawsPerFrame = limit; }
    void SetPaletteCacheEnabled(bool enabled) noexcept { paletteCacheEnabled = enabled; }
    void SetPaletteQuantizationHz(float hz) noexcept { paletteQuantizationHz = hz; }

private:
    std::uint32_t maxPaletteUpdatesPerFrame = 0;
    std::uint32_t maxSkinnedDrawsPerFrame = 0;
    bool paletteCacheEnabled = true;
    float paletteQuantizationHz = 30.0F;
};

}  // namespace Spark

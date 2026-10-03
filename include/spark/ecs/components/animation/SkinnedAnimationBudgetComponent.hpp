#pragma once

#include "spark/ecs/GameComponent.hpp"

#include <cstdint>
#include "spark/scripting/SparkScriptBind.hpp"

namespace Spark {

/**
 * Optional world policy for skinned animation performance (attach to a scene root object).
 * <c>SceneSubmit</c> reads the first active instance each frame.
 */
class SkinnedAnimationBudgetComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::SkinnedAnimationBudget;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    SPARK_SCRIPT_BIND(get_max_palette_updates_per_frame)
    [[nodiscard]] std::uint32_t GetMaxPaletteUpdatesPerFrame() const noexcept { return maxPaletteUpdatesPerFrame; }
    SPARK_SCRIPT_BIND(get_max_skinned_draws_per_frame)
    [[nodiscard]] std::uint32_t GetMaxSkinnedDrawsPerFrame() const noexcept { return maxSkinnedDrawsPerFrame; }
    SPARK_SCRIPT_BIND(is_palette_cache_enabled)
    [[nodiscard]] bool IsPaletteCacheEnabled() const noexcept { return paletteCacheEnabled; }
    SPARK_SCRIPT_BIND(get_palette_quantization_hz)
    [[nodiscard]] float GetPaletteQuantizationHz() const noexcept { return paletteQuantizationHz; }

    SPARK_SCRIPT_BIND(set_max_palette_updates_per_frame)
    void SetMaxPaletteUpdatesPerFrame(std::uint32_t limit) noexcept { maxPaletteUpdatesPerFrame = limit; }
    SPARK_SCRIPT_BIND(set_max_skinned_draws_per_frame)
    void SetMaxSkinnedDrawsPerFrame(std::uint32_t limit) noexcept { maxSkinnedDrawsPerFrame = limit; }
    SPARK_SCRIPT_BIND(set_palette_cache_enabled)
    void SetPaletteCacheEnabled(bool enabled) noexcept { paletteCacheEnabled = enabled; }
    SPARK_SCRIPT_BIND(set_palette_quantization_hz)
    void SetPaletteQuantizationHz(float hz) noexcept { paletteQuantizationHz = hz; }

private:
    std::uint32_t maxPaletteUpdatesPerFrame = 0;
    std::uint32_t maxSkinnedDrawsPerFrame = 0;
    bool paletteCacheEnabled = true;
    float paletteQuantizationHz = 30.0F;
};

}  // namespace Spark

#pragma once

#include <cstdint>

namespace Spark {

/**
 * Per-frame counters and limits for skinned palette solves and draw submission.
 * A limit of <c>0</c> means unlimited for that counter.
 */
class SkinnedAnimationBudget {
public:
    void BeginFrame() noexcept;

    void SetMaxPaletteUpdatesPerFrame(std::uint32_t limit) noexcept { maxPaletteUpdates = limit; }
    void SetMaxSkinnedDrawsPerFrame(std::uint32_t limit) noexcept { maxSkinnedDraws = limit; }

    [[nodiscard]] std::uint32_t GetMaxPaletteUpdatesPerFrame() const noexcept { return maxPaletteUpdates; }
    [[nodiscard]] std::uint32_t GetMaxSkinnedDrawsPerFrame() const noexcept { return maxSkinnedDraws; }

    [[nodiscard]] bool CanComputePalette() const noexcept;
    [[nodiscard]] bool CanSubmitSkinnedDraw() const noexcept;

    [[nodiscard]] bool ConsumePaletteUpdate() noexcept;
    [[nodiscard]] bool ConsumeSkinnedDraw() noexcept;

    void RecordPaletteCacheHit() noexcept { ++paletteCacheHits; }

    [[nodiscard]] std::uint32_t GetPaletteUpdatesUsed() const noexcept { return paletteUpdatesUsed; }
    [[nodiscard]] std::uint32_t GetSkinnedDrawsSubmitted() const noexcept { return skinnedDrawsSubmitted; }
    [[nodiscard]] std::uint32_t GetPaletteCacheHits() const noexcept { return paletteCacheHits; }
    [[nodiscard]] std::uint32_t GetPaletteFallbacksToBindPose() const noexcept { return paletteBindPoseFallbacks; }

    void RecordBindPoseFallback() noexcept { ++paletteBindPoseFallbacks; }

private:
    std::uint32_t maxPaletteUpdates = 0;
    std::uint32_t maxSkinnedDraws = 0;
    std::uint32_t paletteUpdatesUsed = 0;
    std::uint32_t skinnedDrawsSubmitted = 0;
    std::uint32_t paletteCacheHits = 0;
    std::uint32_t paletteBindPoseFallbacks = 0;
};

}  // namespace Spark

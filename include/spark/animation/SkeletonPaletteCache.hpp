#pragma once

#include "spark/animation/SkeletonPaletteCacheKey.hpp"
#include "spark/core/Array.hpp"
#include "spark/core/HashMap.hpp"
#include "spark/math/Matrix4.hpp"

#include <cstdint>

namespace Spark {

/**
 * Frame-scoped palette memo keyed by <c>SkeletonPaletteCacheKey</c>.
 * Identical skeleton + quantized playback can share one CPU solve across instances.
 */
class SkeletonPaletteCache {
public:
    void SetEnabled(bool value) noexcept { enabled = value; }
    [[nodiscard]] bool IsEnabled() const noexcept { return enabled; }

    void SetQuantizationHz(float hz) noexcept { quantizationHz = hz > 1.0e-4F ? hz : 30.0F; }
    [[nodiscard]] float GetQuantizationHz() const noexcept { return quantizationHz; }

    void SetMaxEntries(std::size_t maxEntryCount) noexcept { maxEntries = maxEntryCount; }

    void BeginFrame(std::uint32_t frameIndex) noexcept;
    void Clear() noexcept;

    [[nodiscard]] bool TryCopy(const SkeletonPaletteCacheKey& key, Array<Matrix4>& outPalette);
    void Store(const SkeletonPaletteCacheKey& key, const Array<Matrix4>& palette);

    [[nodiscard]] std::uint32_t GetHitsThisFrame() const noexcept { return hitsThisFrame; }
    [[nodiscard]] std::uint32_t GetStoresThisFrame() const noexcept { return storesThisFrame; }

private:
    class Entry {
    public:
        [[nodiscard]] bool HasKey() const noexcept { return hasKey; }
        [[nodiscard]] bool Matches(const SkeletonPaletteCacheKey& key) const noexcept;
        void Assign(const SkeletonPaletteCacheKey& key, const Array<Matrix4>& palette, std::uint32_t frameIndex);
        void CopyPaletteTo(Array<Matrix4>& outPalette) const;

    private:
        SkeletonPaletteCacheKey storedKey{};
        Array<Matrix4> palette{};
        std::uint32_t lastFrame = 0;
        bool hasKey = false;
    };

    class EntryDigestHasher {
    public:
        [[nodiscard]] std::size_t operator()(std::uint64_t digest) const noexcept { return static_cast<std::size_t>(digest); }
    };

    void PruneIfNeeded();

    HashMap<std::uint64_t, Entry, EntryDigestHasher> entries{};
    bool enabled = true;
    float quantizationHz = 30.0F;
    std::size_t maxEntries = 256;
    std::uint32_t frameIndex = 0;
    std::uint32_t hitsThisFrame = 0;
    std::uint32_t storesThisFrame = 0;
};

}  // namespace Spark

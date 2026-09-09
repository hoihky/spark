#include "spark/animation/SkeletonPaletteCache.hpp"

#include "spark/core/Utility.hpp"

namespace Spark {

bool SkeletonPaletteCache::Entry::Matches(const SkeletonPaletteCacheKey& key) const noexcept {
    return hasKey && storedKey.Matches(key);
}

void SkeletonPaletteCache::Entry::Assign(
        const SkeletonPaletteCacheKey& key,
        const Array<Matrix4>& inPalette,
        const std::uint32_t frame) {
    storedKey = key;
    palette = inPalette;
    lastFrame = frame;
    hasKey = true;
}

void SkeletonPaletteCache::Entry::CopyPaletteTo(Array<Matrix4>& outPalette) const {
    outPalette = palette;
}

void SkeletonPaletteCache::BeginFrame(const std::uint32_t frame) noexcept {
    frameIndex = frame;
    hitsThisFrame = 0;
    storesThisFrame = 0;
}

void SkeletonPaletteCache::Clear() noexcept {
    entries.Clear();
    hitsThisFrame = 0;
    storesThisFrame = 0;
}

bool SkeletonPaletteCache::TryCopy(const SkeletonPaletteCacheKey& key, Array<Matrix4>& outPalette) {
    if (!enabled) {
        return false;
    }
    const std::uint64_t digest = key.Digest();
    const Entry* entry = entries.Find(digest);
    if (entry == nullptr || !entry->Matches(key)) {
        return false;
    }
    entry->CopyPaletteTo(outPalette);
    ++hitsThisFrame;
    return true;
}

void SkeletonPaletteCache::Store(const SkeletonPaletteCacheKey& key, const Array<Matrix4>& palette) {
    if (!enabled || palette.IsEmpty()) {
        return;
    }
    PruneIfNeeded();
    const std::uint64_t digest = key.Digest();
    if (Entry* existing = entries.Find(digest)) {
        existing->Assign(key, palette, frameIndex);
    } else {
        Entry created{};
        created.Assign(key, palette, frameIndex);
        entries.Add(digest, MoveTemp(created));
    }
    ++storesThisFrame;
}

void SkeletonPaletteCache::PruneIfNeeded() {
    if (maxEntries == 0 || entries.GetSize() <= maxEntries) {
        return;
    }
    entries.Clear();
}

}  // namespace Spark

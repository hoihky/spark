#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/HashMap.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/math/Vector4.hpp"
#include "spark/memory/SharedPtr.hpp"

namespace Spark {

class Texture2D;

/**
 * CPU shelf packer that merges multiple small textures into one scene-upload-friendly atlas.
 * Each source texture receives an <c>SetAtlasBinding</c> on the finalized atlas image.
 */
class SceneTextureAtlas {
public:
    static constexpr std::uint32_t kDefaultAtlasSize = 1024;
    static constexpr std::uint32_t kDefaultPadding = 2;

    struct Placement {
        Vector4 uvRect{0.0F, 0.0F, 1.0F, 1.0F};
        std::uint32_t pixelX = 0;
        std::uint32_t pixelY = 0;
        std::uint32_t pixelW = 0;
        std::uint32_t pixelH = 0;
    };

    explicit SceneTextureAtlas(std::uint32_t atlasSize = kDefaultAtlasSize, std::uint32_t padding = kDefaultPadding);
    SceneTextureAtlas(SceneTextureAtlas&&) noexcept = default;
    SceneTextureAtlas& operator=(SceneTextureAtlas&&) noexcept = default;
    SceneTextureAtlas(const SceneTextureAtlas&) = delete;
    SceneTextureAtlas& operator=(const SceneTextureAtlas&) = delete;

    [[nodiscard]] bool IsEmpty() const noexcept { return placements.IsEmpty(); }
    [[nodiscard]] std::uint32_t GetAtlasSize() const noexcept { return atlasSize; }

    /** Returns false when the texture does not fit or was already packed. */
    bool TryAdd(const SharedPtr<Texture2D>& texture);

    /** Builds the atlas <c>Texture2D</c> and binds each packed source via <c>Texture2D::SetAtlasBinding</c>. */
    [[nodiscard]] SharedPtr<Texture2D> Finalize(Utf8String atlasName);

    [[nodiscard]] const Placement* TryGetPlacement(const Texture2D* texture) const noexcept;

private:
    struct PlacementHasher {
        [[nodiscard]] std::size_t operator()(const Texture2D* ptr) const noexcept {
            return reinterpret_cast<std::size_t>(ptr);
        }
    };

    std::uint32_t atlasSize = kDefaultAtlasSize;
    std::uint32_t padding = kDefaultPadding;
    std::uint32_t cursorX = 0;
    std::uint32_t rowY = 0;
    std::uint32_t rowHeight = 0;
    Array<std::uint8_t> atlasPixels;
    Array<SharedPtr<Texture2D>> sources;
    HashMap<const Texture2D*, Placement, PlacementHasher> placements;
};

}  // namespace Spark

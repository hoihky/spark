#pragma once

#include "spark/core/Array.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/Texture2D.hpp"
#include "spark/scene/tilemap/TileAnimation.hpp"
#include "spark/scene/tilemap/TileAutotile.hpp"
#include "spark/scene/tilemap/TileDefinition.hpp"

#include <cstdint>

namespace Spark {

/**
 * Shared atlas layout + per-tile definitions for one or more <c>TilemapComponent</c> instances.
 */
class Tileset {
public:
    Tileset() = default;
    Tileset(SharedPtr<Texture2D> inAtlas, std::uint32_t tilesU, std::uint32_t tilesV) noexcept;

    [[nodiscard]] const SharedPtr<Texture2D>& GetAtlas() const noexcept { return atlas; }
    [[nodiscard]] std::uint32_t GetTilesU() const noexcept { return tilesU; }
    [[nodiscard]] std::uint32_t GetTilesV() const noexcept { return tilesV; }
    [[nodiscard]] std::uint32_t GetCellCount() const noexcept {
        if (tileCountInAtlas > 0U) {
            return tileCountInAtlas;
        }
        return tilesU * tilesV;
    }

    [[nodiscard]] float GetMarginPixels() const noexcept { return marginPixels; }
    [[nodiscard]] float GetSpacingPixels() const noexcept { return spacingPixels; }
    void SetAtlasPadding(const float marginPx, const float spacingPx) noexcept;

    /** Tiled tile pixel size and source image dimensions (0 = infer from texture at UV time). */
    void SetTiledAtlasLayout(
            std::uint32_t tilePixelW,
            std::uint32_t tilePixelH,
            std::uint32_t imagePixelW,
            std::uint32_t imagePixelH,
            std::uint32_t tileCount) noexcept;
    [[nodiscard]] std::uint32_t GetTilePixelWidth() const noexcept { return tilePixelWidth; }
    [[nodiscard]] std::uint32_t GetTilePixelHeight() const noexcept { return tilePixelHeight; }
    [[nodiscard]] std::uint32_t GetImagePixelWidth() const noexcept { return imagePixelWidth; }
    [[nodiscard]] std::uint32_t GetImagePixelHeight() const noexcept { return imagePixelHeight; }

    [[nodiscard]] TileDefinition& Definition(const std::uint16_t tileId) noexcept;
    [[nodiscard]] const TileDefinition& Definition(const std::uint16_t tileId) const noexcept;

    /** Ensures <c>definitions</c> has one entry per atlas cell. */
    void EnsureDefinitions();

    [[nodiscard]] Array<TileAnimationClip>& GetAnimationClips() noexcept { return animationClips; }
    [[nodiscard]] const Array<TileAnimationClip>& GetAnimationClips() const noexcept { return animationClips; }

    [[nodiscard]] std::uint16_t GetAnimationClipIndexForTile(const std::uint16_t tileId) const noexcept;

    /** Returns existing rule set or creates a new one for <c>groupId</c> (1–255). */
    [[nodiscard]] TileAutotileRuleSet* FindAutotileRuleSet(const std::uint8_t groupId) noexcept;
    [[nodiscard]] const TileAutotileRuleSet* FindAutotileRuleSet(const std::uint8_t groupId) const noexcept;
    [[nodiscard]] TileAutotileRuleSet& GetOrCreateAutotileRuleSet(const std::uint8_t groupId);

private:
    SharedPtr<Texture2D> atlas{};
    std::uint32_t tilesU = 1;
    std::uint32_t tilesV = 1;
    float marginPixels = 0.0F;
    float spacingPixels = 0.0F;
    std::uint32_t tilePixelWidth = 0U;
    std::uint32_t tilePixelHeight = 0U;
    std::uint32_t imagePixelWidth = 0U;
    std::uint32_t imagePixelHeight = 0U;
    std::uint32_t tileCountInAtlas = 0U;
    Array<TileDefinition> definitions{};
    Array<TileAnimationClip> animationClips{};
    Array<TileAutotileRuleSet> autotileRuleSets{};
};

/** Builds a tileset from legacy map fields (atlas + grid size). */
[[nodiscard]] SharedPtr<Tileset> CreateTilesetFromAtlas(
        SharedPtr<Texture2D> atlas,
        std::uint32_t tilesU,
        std::uint32_t tilesV);

}  // namespace Spark

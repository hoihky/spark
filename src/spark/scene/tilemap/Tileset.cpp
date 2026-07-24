#include "spark/scene/tilemap/Tileset.hpp"

#include "spark/memory/SharedPtr.hpp"

namespace Spark {

Tileset::Tileset(SharedPtr<Texture2D> inAtlas, const std::uint32_t inTilesU, const std::uint32_t inTilesV) noexcept
        : atlas(MoveTemp(inAtlas)),
          tilesU(inTilesU > 0 ? inTilesU : 1U),
          tilesV(inTilesV > 0 ? inTilesV : 1U) {
    EnsureDefinitions();
}

void Tileset::SetAtlasPadding(const float marginPx, const float spacingPx) noexcept {
    marginPixels = marginPx >= 0.0F ? marginPx : 0.0F;
    spacingPixels = spacingPx >= 0.0F ? spacingPx : 0.0F;
}

void Tileset::SetTiledAtlasLayout(
        const std::uint32_t tilePixelW,
        const std::uint32_t tilePixelH,
        const std::uint32_t imagePixelW,
        const std::uint32_t imagePixelH,
        const std::uint32_t tileCount) noexcept {
    tilePixelWidth = tilePixelW;
    tilePixelHeight = tilePixelH;
    imagePixelWidth = imagePixelW;
    imagePixelHeight = imagePixelH;
    tileCountInAtlas = tileCount;
}

void Tileset::EnsureDefinitions() {
    const std::uint32_t cells = GetCellCount();
    if (cells == 0) {
        definitions.Clear();
        return;
    }
    if (definitions.GetSize() == static_cast<std::size_t>(cells)) {
        return;
    }
    definitions.Resize(static_cast<std::size_t>(cells));
}

TileDefinition& Tileset::Definition(const std::uint16_t tileId) noexcept {
    EnsureDefinitions();
    if (tileId >= GetCellCount()) {
        static TileDefinition invalid{};
        return invalid;
    }
    return definitions[static_cast<std::size_t>(tileId)];
}

const TileDefinition& Tileset::Definition(const std::uint16_t tileId) const noexcept {
    if (tileId >= GetCellCount() || definitions.GetSize() != static_cast<std::size_t>(GetCellCount())) {
        static const TileDefinition kDefault{};
        return kDefault;
    }
    return definitions[static_cast<std::size_t>(tileId)];
}

std::uint16_t Tileset::GetAnimationClipIndexForTile(const std::uint16_t tileId) const noexcept {
    return Definition(tileId).animationClipIndex;
}

TileAutotileRuleSet* Tileset::FindAutotileRuleSet(const std::uint8_t groupId) noexcept {
    if (groupId == 0) {
        return nullptr;
    }
    for (std::size_t i = 0; i < autotileRuleSets.GetSize(); ++i) {
        if (autotileRuleSets[i].groupId == groupId) {
            return &autotileRuleSets[i];
        }
    }
    return nullptr;
}

const TileAutotileRuleSet* Tileset::FindAutotileRuleSet(const std::uint8_t groupId) const noexcept {
    return const_cast<Tileset*>(this)->FindAutotileRuleSet(groupId);
}

TileAutotileRuleSet& Tileset::GetOrCreateAutotileRuleSet(const std::uint8_t groupId) {
    if (TileAutotileRuleSet* existing = FindAutotileRuleSet(groupId); existing != nullptr) {
        return *existing;
    }
    TileAutotileRuleSet set{};
    set.groupId = groupId;
    autotileRuleSets.PushBack(set);
    return autotileRuleSets.GetLast();
}

SharedPtr<Tileset> CreateTilesetFromAtlas(
        SharedPtr<Texture2D> atlas,
        const std::uint32_t tilesU,
        const std::uint32_t tilesV) {
    return MakeShared<Tileset>(MoveTemp(atlas), tilesU, tilesV);
}

}  // namespace Spark

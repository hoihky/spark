#include "spark/scene/SceneTileAtlas.hpp"

namespace Spark {

void TileIdToAtlasUvRect(
        const std::uint16_t tileId,
        const std::uint32_t atlasTilesU,
        const std::uint32_t atlasTilesV,
        Vector4& outUv) noexcept {
    outUv = {0.0F, 0.0F, 1.0F, 1.0F};
    if (atlasTilesU == 0 || atlasTilesV == 0 || tileId == TilemapComponent::kEmptyTile) {
        return;
    }
    const std::uint32_t cells = atlasTilesU * atlasTilesV;
    if (tileId >= cells) {
        return;
    }
    const std::uint32_t tx = tileId % atlasTilesU;
    const std::uint32_t ty = tileId / atlasTilesU;
    const float du = 1.0F / static_cast<float>(atlasTilesU);
    const float dv = 1.0F / static_cast<float>(atlasTilesV);
    outUv.x = static_cast<float>(tx) * du;
    outUv.y = static_cast<float>(ty) * dv;
    outUv.z = static_cast<float>(tx + 1U) * du;
    outUv.w = static_cast<float>(ty + 1U) * dv;
}

}  // namespace Spark

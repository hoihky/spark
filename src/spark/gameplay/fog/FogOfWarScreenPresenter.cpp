#include "spark/gameplay/fog/FogOfWarScreenPresenter.hpp"

#include "spark/engine/SceneRenderParams.hpp"
#include "spark/scene/submit/Scene2DSubmitCaps.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/texture/Texture2D.hpp"

namespace Spark {

namespace {

constexpr std::int16_t kFogSortingLayerOrder = 32767;
constexpr std::int32_t kFogSortingOrder = 32767;

}  // namespace

void FogOfWarScreenPresenter::AppendWorldSprite(
        SceneRenderParams& params,
        GameWorld& world,
        const SharedPtr<Texture2D>& fogMaskTexture,
        const TilemapGridFrame& frame,
        const SceneSubmitDetail::FindSceneTextureFn& findSceneTexture) const noexcept {
    if (!fogMaskTexture || fogMaskTexture->GetWidth() == 0U || fogMaskTexture->GetHeight() == 0U ||
        frame.mapWidth == 0U || frame.mapHeight == 0U) {
        return;
    }
    if (!Scene2DSubmitCaps::HasSpriteCapacity(params)) {
        Scene2DSubmitCaps::NotifySpriteCapacityReached();
        return;
    }

    static_cast<void>(world.Assets().RegisterTexture(fogMaskTexture, "spark/fog/mask"));
    const std::int32_t textureLayer = findSceneTexture(fogMaskTexture, nullptr, nullptr);
    if (textureLayer < 0) {
        return;
    }

    const float mapW = static_cast<float>(frame.mapWidth) * frame.cellSize;
    const float mapH = static_cast<float>(frame.mapHeight) * frame.cellSize;
    const Matrix4 localModel =
            Matrix4::Translation({mapW * 0.5F, mapH * 0.5F, 0.18F}) * Matrix4::Scale({mapW, mapH, 1.0F});

    SceneSpriteDraw sprite{};
    sprite.model = frame.worldFromLocal * localModel;
    sprite.tint = {1.0F, 1.0F, 1.0F, 1.0F};
    sprite.uvRect = fogMaskTexture->ScaleUvRectForSceneLayer({0.0F, 0.0F, 1.0F, 1.0F});
    sprite.normalUvRect = sprite.uvRect;
    sprite.textureLayer = textureLayer;
    sprite.sortingLayerOrder = kFogSortingLayerOrder;
    sprite.sortOrder = kFogSortingOrder;
    sprite.sortWorldY = sprite.model.m[13] + mapH * 0.5F;
    params.sprites.PushBack(sprite);
}

}  // namespace Spark

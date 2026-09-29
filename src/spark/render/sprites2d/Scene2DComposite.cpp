#include "spark/render/sprites2d/Scene2DComposite.hpp"

#include "spark/ecs/components/rendering/Scene2DCompositeViewComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/tilemap/TilemapGridCoordinates.hpp"

#include <algorithm>
#include <cmath>

namespace Spark {

Scene2DMinimapOrthoBounds ComputeMinimapOrthoBoundsSquare(const TilemapGridFrame& frame, const float padding) noexcept {
    Scene2DMinimapOrthoBounds bounds{};
    if (frame.mapWidth == 0U || frame.mapHeight == 0U) {
        return bounds;
    }
    const Vector2 corner = frame.LocalCornerToWorldXY();
    const float mapW = static_cast<float>(frame.mapWidth) * frame.cellSize;
    const float mapH = static_cast<float>(frame.mapHeight) * frame.cellSize;
    const float halfW = mapW * 0.5F;
    const float halfH = mapH * 0.5F;
    bounds.worldCenter = {corner.x + halfW, corner.y + halfH, 0.0F};
    const float halfExtent = std::max(halfW, halfH) * padding;
    bounds.worldHalfWidth = halfExtent;
    bounds.worldHalfHeight = halfExtent;
    return bounds;
}

void WorldXYToMinimapSquareUv(
        const Vector2& worldXY,
        const Scene2DMinimapOrthoBounds& bounds,
        float& outU,
        float& outV) noexcept {
    const float denomW = std::max(bounds.worldHalfWidth * 2.0F, 1.0e-4F);
    const float denomH = std::max(bounds.worldHalfHeight * 2.0F, 1.0e-4F);
    outU = (worldXY.x - (bounds.worldCenter.x - bounds.worldHalfWidth)) / denomW;
    outV = (worldXY.y - (bounds.worldCenter.y - bounds.worldHalfHeight)) / denomH;
}

void FillCompositeViewFromTilemapGridFrame(Scene2DCompositeViewDesc& view, const TilemapGridFrame& frame) noexcept {
    const Scene2DMinimapOrthoBounds ortho = ComputeMinimapOrthoBoundsSquare(frame);
    if (frame.mapWidth == 0U || frame.mapHeight == 0U) {
        return;
    }
    view.worldCenter = ortho.worldCenter;
    view.worldOrthoHalfExtent = ortho.worldHalfHeight;
}

void CollectScene2DCompositeViews(GameWorld& world, SceneRenderParams& params) noexcept {
    params.scene2DCompositeViews.Clear();
    world.ForEachGameObject([&](GameObject* object) {
        if (object == nullptr) {
            return;
        }
        const Scene2DCompositeViewComponent* view = object->GetComponent<Scene2DCompositeViewComponent>();
        if (view == nullptr || !view->IsEnabled()) {
            return;
        }
        if (params.scene2DCompositeViews.GetSize() >= Scene2DRuntimeLimits::MaxScene2DCompositeViews) {
            return;
        }
        params.scene2DCompositeViews.PushBack(view->BuildDesc());
    });
}

}  // namespace Spark

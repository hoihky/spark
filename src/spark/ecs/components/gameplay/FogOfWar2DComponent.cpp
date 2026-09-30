#include "spark/ecs/components/gameplay/FogOfWar2DComponent.hpp"

#include "spark/ecs/components/tilemap/TilemapGameplayGridComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/scene/core/GameWorld.hpp"

namespace Spark {

void FogOfWar2DComponent::OnAttach(GameObject& owner) {
    if (!maskTexture) {
        maskTexture = MakeShared<Texture2D>(Utf8String("FogOfWarMask"));
    }
    SyncGridFromOwner(owner);
}

void FogOfWar2DComponent::SyncGridFromOwner(GameObject& owner) noexcept {
    const TilemapGameplayGridComponent* grid = owner.GetComponent<TilemapGameplayGridComponent>();
    if (grid == nullptr) {
        return;
    }
    const TilemapGameplayGrid& baked = grid->GetGrid();
    exploration.Reset(baked.Width(), baked.Height());
    gridSynced = true;
    if (revealTarget != nullptr) {
        const Vector3 worldPos = revealTarget->GetWorldMatrix().TransformPoint(Vector3::Zero);
        exploration.RevealDisc({worldPos.x, worldPos.y}, visionRadiusWorld, grid->GetGridFrame());
    }
    RebuildMaskTexture();
}

void FogOfWar2DComponent::RevealAtWorldPosition(const Vector2& worldXY, const TilemapGridFrame& frame) noexcept {
    if (!gridSynced) {
        return;
    }
    exploration.RevealDisc(worldXY, visionRadiusWorld, frame);
    RebuildMaskTexture();
}

void FogOfWar2DComponent::RebuildMaskTexture() noexcept {
    if (!maskTexture) {
        return;
    }
    maskBuilder.RebuildMaskTexture(exploration, *maskTexture);
}

void FogOfWar2DComponent::SubmitSceneDraw(
        SceneRenderParams& params,
        GameWorld& world,
        const TilemapGridFrame& frame,
        const SceneSubmitDetail::FindSceneTextureFn& findSceneTexture) const noexcept {
    if (!gridSynced || !maskTexture) {
        return;
    }
    screenPresenter.AppendWorldSprite(params, world, maskTexture, frame, findSceneTexture);
}

}  // namespace Spark

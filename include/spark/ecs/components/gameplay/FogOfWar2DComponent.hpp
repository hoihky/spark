#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/gameplay/fog/FogOfWarExplorationMap.hpp"
#include "spark/gameplay/fog/FogOfWarMaskTextureBuilder.hpp"
#include "spark/gameplay/fog/FogOfWarScreenPresenter.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/submit/detail/SceneSubmitDetail.hpp"
#include "spark/scene/texture/Texture2D.hpp"
#include "spark/scene/tilemap/TilemapGridCoordinates.hpp"

namespace Spark {

class GameObject;
class GameWorld;
class TilemapGameplayGridComponent;
struct SceneRenderParams;

/** Tilemap-aligned fog-of-war exploration and mask presentation. */
class FogOfWar2DComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::FogOfWar2D;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    void OnAttach(GameObject& owner) override;

    void SetVisionRadiusWorld(const float radius) noexcept { visionRadiusWorld = radius; }
    [[nodiscard]] float GetVisionRadiusWorld() const noexcept { return visionRadiusWorld; }

    void SetRevealTarget(GameObject* target) noexcept { revealTarget = target; }
    [[nodiscard]] GameObject* GetRevealTarget() const noexcept { return revealTarget; }

    [[nodiscard]] const SharedPtr<Texture2D>& GetMaskTexture() const noexcept { return maskTexture; }
    [[nodiscard]] const FogOfWarExplorationMap& GetExplorationMap() const noexcept { return exploration; }

    void SyncGridFromOwner(GameObject& owner) noexcept;
    void RevealAtWorldPosition(const Vector2& worldXY, const TilemapGridFrame& frame) noexcept;
    void RebuildMaskTexture() noexcept;

    void SubmitSceneDraw(
            SceneRenderParams& params,
            GameWorld& world,
            const TilemapGridFrame& frame,
            const SceneSubmitDetail::FindSceneTextureFn& findSceneTexture) const noexcept;

private:
    FogOfWarExplorationMap exploration{};
    FogOfWarMaskTextureBuilder maskBuilder{};
    FogOfWarScreenPresenter screenPresenter{};
    SharedPtr<Texture2D> maskTexture{};
    GameObject* revealTarget = nullptr;
    float visionRadiusWorld = 5.5F;
    bool gridSynced = false;
};

}  // namespace Spark

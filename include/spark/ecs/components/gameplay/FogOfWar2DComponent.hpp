#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/gameplay/fog/FogOfWarExplorationMap.hpp"
#include "spark/gameplay/fog/FogOfWarMaskTextureBuilder.hpp"
#include "spark/gameplay/fog/FogOfWarScreenPresenter.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/submit/detail/SceneSubmitDetail.hpp"
#include "spark/scene/texture/Texture2D.hpp"
#include "spark/scene/tilemap/TilemapGridCoordinates.hpp"
#include "spark/scripting/SparkScriptBind.hpp"

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

    SPARK_SCRIPT_BIND(set_vision_radius_world)
    void SetVisionRadiusWorld(const float radius) noexcept { visionRadiusWorld = radius; }
    SPARK_SCRIPT_BIND(get_vision_radius_world)
    [[nodiscard]] float GetVisionRadiusWorld() const noexcept { return visionRadiusWorld; }

    SPARK_SCRIPT_BIND(set_reveal_target)
    void SetRevealTarget(GameObject* target) noexcept { revealTarget = target; }
    SPARK_SCRIPT_BIND(get_reveal_target)
    [[nodiscard]] GameObject* GetRevealTarget() const noexcept { return revealTarget; }

    [[nodiscard]] const SharedPtr<Texture2D>& GetMaskTexture() const noexcept { return maskTexture; }
    SPARK_SCRIPT_BIND(get_exploration_map)
    [[nodiscard]] const FogOfWarExplorationMap& GetExplorationMap() const noexcept { return exploration; }

    SPARK_SCRIPT_BIND(sync_grid_from_owner)
    void SyncGridFromOwner(GameObject& owner) noexcept;
    SPARK_SCRIPT_BIND(reveal_at_world_position)
    void RevealAtWorldPosition(const Vector2& worldXY, const TilemapGridFrame& frame) noexcept;
    SPARK_SCRIPT_BIND(rebuild_mask_texture)
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

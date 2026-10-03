#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/engine/FrameTiming.hpp"
#include "spark/ai/path/IGridWalkability.hpp"
#include "spark/scene/tilemap/TilemapEditRevision.hpp"
#include "spark/scene/tilemap/TilemapGameplayGrid.hpp"
#include "spark/scene/tilemap/TilemapGameplayWalkRule.hpp"
#include "spark/scene/tilemap/TilemapGridCoordinates.hpp"
#include "spark/scripting/SparkScriptBind.hpp"

namespace Spark {

class GameObject;
class IEngineContext;
class TilemapComponent;

/**
 * Cached walkability grid for the sibling <c>TilemapComponent</c>. Rebake after editing tiles
 * via <c>RequestRebake()</c> + <c>RebakeIfNeeded()</c> (or enable auto rebake).
 */
class TilemapGameplayGridComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::TilemapGameplayGrid;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    SPARK_SCRIPT_BIND(get_walk_rule)
    [[nodiscard]] TilemapGameplayWalkRule GetWalkRule() const noexcept { return walkRule; }
    void SetWalkRule(const TilemapGameplayWalkRule rule) noexcept {
        walkRule = rule;
        RequestRebake();
    }

    SPARK_SCRIPT_BIND(get_auto_rebake)
    [[nodiscard]] bool GetAutoRebake() const noexcept { return autoRebake; }
    SPARK_SCRIPT_BIND(set_auto_rebake)
    void SetAutoRebake(const bool enabled) noexcept { autoRebake = enabled; }

    SPARK_SCRIPT_BIND(request_rebake)
    void RequestRebake() noexcept { rebakeRequested = true; }

    /** Rebakes when <c>RequestRebake()</c> was called or <c>autoRebake</c> is true. */
    SPARK_SCRIPT_BIND(rebake_if_needed)
    void RebakeIfNeeded(const GameObject& owner) noexcept;

    /** Partial rebake for editor dirty regions (Phase C). */
    SPARK_SCRIPT_BIND(rebake_region)
    void RebakeRegion(const GameObject& owner, const TilemapCellRegion& region) noexcept;

    void OnUpdate(const FrameTiming& timing, GameObject& owner, IEngineContext& context) override;

    SPARK_SCRIPT_BIND(get_grid)
    [[nodiscard]] const TilemapGameplayGrid& GetGrid() const noexcept { return grid; }
    SPARK_SCRIPT_BIND(get_grid_frame)
    [[nodiscard]] const TilemapGridFrame& GetGridFrame() const noexcept { return frame; }
    SPARK_SCRIPT_BIND(get_walkability)
    [[nodiscard]] const IGridWalkability& GetWalkability() const noexcept { return grid.AsWalkability(); }

private:
    TilemapGameplayGrid grid{};
    TilemapGridFrame frame{};
    TilemapGameplayWalkRule walkRule = TilemapGameplayWalkRule::OccupiedWalkable;
    bool autoRebake = false;
    bool rebakeRequested = true;
};

}  // namespace Spark

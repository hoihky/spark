#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/engine/FrameTiming.hpp"
#include "spark/ai/path/IGridWalkability.hpp"
#include "spark/scene/tilemap/TilemapGameplayGrid.hpp"
#include "spark/scene/tilemap/TilemapGameplayWalkRule.hpp"
#include "spark/scene/tilemap/TilemapGridCoordinates.hpp"

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

    [[nodiscard]] TilemapGameplayWalkRule GetWalkRule() const noexcept { return walkRule; }
    void SetWalkRule(const TilemapGameplayWalkRule rule) noexcept {
        walkRule = rule;
        RequestRebake();
    }

    [[nodiscard]] bool GetAutoRebake() const noexcept { return autoRebake; }
    void SetAutoRebake(const bool enabled) noexcept { autoRebake = enabled; }

    void RequestRebake() noexcept { rebakeRequested = true; }

    /** Rebakes when <c>RequestRebake()</c> was called or <c>autoRebake</c> is true. */
    void RebakeIfNeeded(const GameObject& owner) noexcept;

    void OnUpdate(const FrameTiming& timing, GameObject& owner, IEngineContext& context) override;

    [[nodiscard]] const TilemapGameplayGrid& GetGrid() const noexcept { return grid; }
    [[nodiscard]] const TilemapGridFrame& GetGridFrame() const noexcept { return frame; }
    [[nodiscard]] const IGridWalkability& GetWalkability() const noexcept { return grid.AsWalkability(); }

private:
    TilemapGameplayGrid grid{};
    TilemapGridFrame frame{};
    TilemapGameplayWalkRule walkRule = TilemapGameplayWalkRule::OccupiedWalkable;
    bool autoRebake = false;
    bool rebakeRequested = true;
};

}  // namespace Spark

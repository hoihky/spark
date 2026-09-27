#pragma once

#include "spark/scene/tilemap/TilemapEditRevision.hpp"

#include <cstdint>

namespace Spark {

class GameObject;
class GameWorld;
class PhysicsSubsystem;

/** Rebakes gameplay grid, autotile, and optional physics query statics from an edit revision. */
class TilemapDerivedDataRebaker final {
public:
    class Options {
    public:
        std::uint32_t neighborMargin = 1U;
        bool gameplayGrid = true;
        bool autotile = true;
        bool physicsQueryStatics = false;
    };

    class Result {
    public:
        [[nodiscard]] bool DidRebakeGameplayGrid() const noexcept { return gameplayGridRebaked; }
        [[nodiscard]] bool DidRebakeAutotile() const noexcept { return autotileRebaked; }
        [[nodiscard]] bool DidRebuildPhysicsQueryStatics() const noexcept { return physicsQueryStaticsRebuilt; }
        [[nodiscard]] const TilemapCellRegion& GetRebakedRegion() const noexcept { return rebakedRegion; }

        bool gameplayGridRebaked = false;
        bool autotileRebaked = false;
        bool physicsQueryStaticsRebuilt = false;
        TilemapCellRegion rebakedRegion{};
    };

    [[nodiscard]] Result RebakeForRevision(
            GameObject& owner,
            const TilemapEditRevision& revision) const noexcept;

    [[nodiscard]] Result RebakeForRevision(
            GameObject& owner,
            const TilemapEditRevision& revision,
            const Options& options,
            GameWorld* worldForPhysics = nullptr,
            PhysicsSubsystem* physics = nullptr) const noexcept;
};

}  // namespace Spark

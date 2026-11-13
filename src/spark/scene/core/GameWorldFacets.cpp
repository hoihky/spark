#include "spark/scene/core/GameWorld.hpp"

#include "spark/scene/core/world/WorldAssets.hpp"
#include "spark/scene/core/world/WorldEntities.hpp"
#include "spark/scene/core/world/WorldPhysics.hpp"
#include "spark/scene/core/world/WorldQuery.hpp"
#include "spark/scene/core/world/WorldRendering.hpp"
#include "spark/scene/core/world/WorldSimulation.hpp"

namespace Spark {

WorldEntities GameWorld::Entities() noexcept {
    return WorldEntities{*this};
}

WorldEntities GameWorld::Entities() const noexcept {
    return WorldEntities{const_cast<GameWorld&>(*this)};
}

WorldAssets GameWorld::Assets() noexcept {
    return WorldAssets{*this};
}

WorldAssets GameWorld::Assets() const noexcept {
    return WorldAssets{const_cast<GameWorld&>(*this)};
}

WorldSimulation GameWorld::Simulation() noexcept {
    return WorldSimulation{*this};
}

WorldSimulation GameWorld::Simulation() const noexcept {
    return WorldSimulation{const_cast<GameWorld&>(*this)};
}

WorldRendering GameWorld::Rendering() noexcept {
    return WorldRendering{*this};
}

WorldRendering GameWorld::Rendering() const noexcept {
    return WorldRendering{const_cast<GameWorld&>(*this)};
}

WorldPhysics GameWorld::Physics(PhysicsSubsystem& physics) noexcept {
    return WorldPhysics{*this, physics};
}

WorldPhysics GameWorld::Physics(PhysicsSubsystem& physics) const noexcept {
    return WorldPhysics{const_cast<GameWorld&>(*this), physics};
}

WorldQuery GameWorld::QueryObjects() noexcept {
    return WorldQuery{*this};
}

WorldQuery GameWorld::QueryObjects() const noexcept {
    return WorldQuery{const_cast<GameWorld&>(*this)};
}

}  // namespace Spark

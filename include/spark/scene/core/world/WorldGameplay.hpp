#pragma once

#include "spark/scene/core/world/WorldView.hpp"

namespace Spark {

class GameObject;

/** Gameplay interaction and post-physics hooks bound to one world. */
class WorldGameplay final : public WorldView {
public:
    using WorldView::WorldView;

#if SPARK_HAS_GAMEPLAY
    void AfterPhysics2D() noexcept;
    [[nodiscard]] bool InteractPress(GameObject& instigator) noexcept;
    void InteractIfPressed2D(GameObject& instigator, bool pressed) noexcept;
#else
    void AfterPhysics2D() noexcept {}
    [[nodiscard]] bool InteractPress(GameObject&) noexcept { return false; }
    void InteractIfPressed2D(GameObject&, bool) noexcept {}
#endif
};

}  // namespace Spark

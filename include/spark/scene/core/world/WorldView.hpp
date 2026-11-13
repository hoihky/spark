#pragma once

namespace Spark {

class GameWorld;

/** Non-owning view over a <c>GameWorld</c> (zero storage beyond the reference). */
class WorldView {
public:
    explicit WorldView(GameWorld& world) noexcept : world_(world) {}

    [[nodiscard]] GameWorld& GetWorld() noexcept { return world_; }
    [[nodiscard]] const GameWorld& GetWorld() const noexcept { return world_; }

protected:
    GameWorld& world_;
};

}  // namespace Spark

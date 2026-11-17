#pragma once

namespace Spark {

class GameWorld;

/** Non-owning view over a <c>GameWorld</c> (zero storage beyond the reference). */
class WorldView {
public:
    explicit WorldView(GameWorld& world) noexcept : hostWorld(world) {}

    [[nodiscard]] GameWorld& GetWorld() noexcept { return hostWorld; }
    [[nodiscard]] const GameWorld& GetWorld() const noexcept { return hostWorld; }

protected:
    GameWorld& hostWorld;
};

}  // namespace Spark

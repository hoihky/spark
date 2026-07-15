#include "spark/engine/Engine.hpp"
#include "spark/engine/Game.hpp"

/**
 * Empty starting point: extend Spark::Game and add systems, scenes, and rendering in the overrides.
 * The base Game::OnUpdate runs ECS GameObject updates and sound cues when you spawn content in the world.
 */
class MyGame final : public Spark::Game {};

int main() {
    Spark::Engine engine(Spark::Engine::NewGame<MyGame>());
    engine.Run();
    return 0;
}

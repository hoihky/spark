#include "Platformer2DGame.hpp"
#include "spark/engine/Engine.hpp"

int main() {
    Spark::Engine engine(Spark::Engine::NewGame<Spark::Platformer2DGame>());
    engine.Run();
    return 0;
}

#include "FpsGame.hpp"
#include "spark/engine/Engine.hpp"

int main() {
    Spark::Engine engine(Spark::Engine::NewGame<Spark::FpsGame>());
    engine.Run();
    return 0;
}

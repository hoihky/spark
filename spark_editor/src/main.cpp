#include "spark_editor/EditorGame.hpp"
#include "spark/engine/Engine.hpp"

#include <exception>
#include <iostream>
#include <print>

int main() {
    try {
        Spark::Engine engine(Spark::NewEditorGame());
        engine.Run();
    } catch (const std::exception& e) {
        std::println(std::cerr, "Spark Editor: {}", e.what());
        return 1;
    }
    return 0;
}

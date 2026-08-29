#include "spark/editor/EditorGame.hpp"
#include "spark/engine/Engine.hpp"
#include "spark/engine/EngineRunOptions.hpp"

#include <exception>
#include <iostream>
#include <print>

int main() {
    try {
        Spark::Engine engine(Spark::NewEditorGame());
        engine.Run(Spark::EngineRunOptions{});
    } catch (const std::exception& e) {
        std::println(std::cerr, "SparkEditor: {}", e.what());
        return 1;
    }
    return 0;
}

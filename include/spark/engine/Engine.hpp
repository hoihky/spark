#pragma once

#include "spark/engine/EngineRunOptions.hpp"
#include "spark/core/Utility.hpp"
#include "spark/engine/IGame.hpp"
#include "spark/memory/UniquePtr.hpp"

namespace Spark {

/**
 * Owns subsystems and drives the main loop. Game code implements IGame only — do not modify Engine.
 */
class Engine {
public:
    explicit Engine(UniquePtr<IGame> game);
    ~Engine();

    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;
    Engine(Engine&&) = delete;
    Engine& operator=(Engine&&) = delete;

    void Run(const EngineRunOptions& options = {});

    template<typename GameType, typename... Args>
    static UniquePtr<IGame> NewGame(Args&&... args) {
        return UniquePtr<IGame>(new GameType(Forward<Args>(args)...));
    }

private:
    struct Impl;
    UniquePtr<Impl> impl;
};

}  // namespace Spark

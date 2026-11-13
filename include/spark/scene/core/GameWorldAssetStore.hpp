#pragma once

#include "spark/scene/assets/GameWorldAssetCache.hpp"
#include "spark/scene/assets/GameWorldAssetLoader.hpp"

namespace Spark {

/** Owns synchronous cache + async loader for one world (lifecycle only). */
class GameWorldAssetStore final {
public:
    GameWorldAssetStore();
    ~GameWorldAssetStore();

    GameWorldAssetStore(const GameWorldAssetStore&) = delete;
    GameWorldAssetStore& operator=(const GameWorldAssetStore&) = delete;

    void Start();
    void Shutdown();

    [[nodiscard]] GameWorldAssetCache& Cache() noexcept { return cache; }
    [[nodiscard]] const GameWorldAssetCache& Cache() const noexcept { return cache; }

    [[nodiscard]] GameWorldAssetLoader& Loader() noexcept { return loader; }
    [[nodiscard]] const GameWorldAssetLoader& Loader() const noexcept { return loader; }

private:
    GameWorldAssetCache cache;
    GameWorldAssetLoader loader;
};

}  // namespace Spark

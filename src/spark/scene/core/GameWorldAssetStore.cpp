#include "spark/scene/core/GameWorldAssetStore.hpp"

namespace Spark {

GameWorldAssetStore::GameWorldAssetStore() = default;

GameWorldAssetStore::~GameWorldAssetStore() {
    Shutdown();
}

void GameWorldAssetStore::Start() {
    loader.Start();
}

void GameWorldAssetStore::Shutdown() {
    loader.Shutdown();
}

}  // namespace Spark

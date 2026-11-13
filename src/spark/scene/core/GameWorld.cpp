#include "spark/scene/core/GameWorld.hpp"

#include "spark/ecs/components/rendering/MaterialComponent.hpp"
#include "spark/ecs/components/rendering/MultiMaterialComponent.hpp"
#include "spark/engine/IEngineContext.hpp"
#include "spark/scene/material/MaterialAssetLoader.hpp"
#include "spark/text/Font.hpp"

namespace Spark {

GameWorld::GameWorld() : entityGraph_(*this) {
    assetStore_.Start();
}

GameWorld::~GameWorld() {
    presentation_.Shutdown(*this);
    assetStore_.Shutdown();
    entityGraph_.DestroyAllRootObjectsImmediate();
}

void GameWorld::UpdateGameObjects(const FrameTiming& timing, IEngineContext& context) {
    assetStore_.Loader().Pump(*this);
    entityGraph_.SetUpdating(true);
    entityGraph_.ForEachActive([&](GameObject* o) {
        if (o == nullptr || o->pendingDestroy) {
            return;
        }
        if (MaterialComponent* material = o->GetComponent<MaterialComponent>()) {
            material->TryApplyMaterialAsset(*this);
        }
        if (MultiMaterialComponent* multiMaterial = o->GetComponent<MultiMaterialComponent>()) {
            multiMaterial->TryApplyMaterialAssets(*this);
        }
        o->UpdateComponents(timing, context);
    });
    entityGraph_.SetUpdating(false);
    entityGraph_.FlushPendingDestroys();
}

bool GameWorld::AwaitGltf(const char* path, GltfAsset& out) {
    out = GltfAsset{};
    if (path == nullptr || path[0] == '\0') {
        return false;
    }
    if (TryGetCachedGltf(path, out) && out.mesh) {
        return true;
    }
    const AssetLoadOutcome<GltfAsset> loaded = TryLoadGltf(path);
    out = loaded.value;
    return loaded.ok && static_cast<bool>(out.mesh);
}

bool GameWorld::AwaitSkinnedGltf(const char* path, SkinnedGltfAsset& out) {
    out = SkinnedGltfAsset{};
    if (path == nullptr || path[0] == '\0') {
        return false;
    }
    if (TryGetCachedSkinnedGltf(path, out) && out.mesh && out.skeleton) {
        return true;
    }
    const AssetLoadOutcome<SkinnedGltfAsset> loaded = TryLoadSkinnedGltf(path);
    out = loaded.value;
    return loaded.ok && static_cast<bool>(out.mesh) && static_cast<bool>(out.skeleton);
}

void GameWorld::SetUiFont(SharedPtr<Font> font) {
    presentation_.SetUiFont(MoveTemp(font));
}

void GameWorld::SetUiBoldFont(SharedPtr<Font> font) {
    presentation_.SetUiBoldFont(MoveTemp(font));
}

bool GameWorld::SaveMaterialAsset(
        const char* path,
        const MaterialAsset& material,
        const char* relativeToDir,
        const GameWorldAssetCache* textureKeys) {
    return MaterialAssetLoader::TrySaveToFile(path, material, relativeToDir, textureKeys);
}

}  // namespace Spark

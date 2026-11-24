#include "spark/scene/core/GameWorld.hpp"

#include "spark/ecs/components/rendering/MaterialComponent.hpp"
#include "spark/ecs/components/rendering/MultiMaterialComponent.hpp"
#include "spark/engine/IEngineContext.hpp"
#include "spark/ui/text/Font.hpp"

namespace Spark {

GameWorld::GameWorld() : entityGraph(*this) {
    assetStore.Start();
}

GameWorld::~GameWorld() {
    presentation.Shutdown(*this);
    assetStore.Shutdown();
    entityGraph.DestroyAllRootObjectsImmediate();
}

void GameWorld::UpdateGameObjects(const FrameTiming& timing, IEngineContext& context) {
    assetStore.Loader().Pump(*this);
    entityGraph.BeginDeferredDestroy();
    entityGraph.ForEachActive([&](GameObject* o) {
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
    entityGraph.EndDeferredDestroy();
    entityGraph.FlushPendingDestroys();
}

void GameWorld::SetUiFont(SharedPtr<Font> font) {
    presentation.SetUiFont(MoveTemp(font));
}

void GameWorld::SetUiBoldFont(SharedPtr<Font> font) {
    presentation.SetUiBoldFont(MoveTemp(font));
}

}  // namespace Spark

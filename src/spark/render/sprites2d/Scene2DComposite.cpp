#include "spark/render/sprites2d/Scene2DComposite.hpp"

#include "spark/ecs/components/rendering/Scene2DCompositeViewComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/scene/core/GameWorld.hpp"

namespace Spark {

void CollectScene2DCompositeViews(GameWorld& world, SceneRenderParams& params) noexcept {
    params.scene2DCompositeViews.Clear();
    world.ForEachGameObject([&](GameObject* object) {
        if (object == nullptr) {
            return;
        }
        const Scene2DCompositeViewComponent* view = object->GetComponent<Scene2DCompositeViewComponent>();
        if (view == nullptr || !view->IsEnabled()) {
            return;
        }
        if (params.scene2DCompositeViews.GetSize() >= Scene2DRuntimeLimits::MaxScene2DCompositeViews) {
            return;
        }
        params.scene2DCompositeViews.PushBack(view->BuildDesc());
    });
}

}  // namespace Spark

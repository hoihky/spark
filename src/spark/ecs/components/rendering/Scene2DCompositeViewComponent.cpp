#include "spark/ecs/components/rendering/Scene2DCompositeViewComponent.hpp"

namespace Spark {

Scene2DCompositeViewDesc Scene2DCompositeViewComponent::BuildDesc() const noexcept {
    Scene2DCompositeViewDesc desc{};
    desc.feature = feature;
    desc.target = target;
    desc.screenX = screenX;
    desc.screenY = screenY;
    desc.screenW = screenW;
    desc.screenH = screenH;
    desc.worldCenter = worldCenter;
    desc.worldOrthoHalfExtent = worldOrthoHalfExtent;
    desc.enabled = enabled;
    return desc;
}

}  // namespace Spark

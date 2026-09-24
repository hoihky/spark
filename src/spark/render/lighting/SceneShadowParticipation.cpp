#include "spark/render/lighting/SceneShadowParticipation.hpp"

#include "spark/ecs/components/rendering/MaterialComponent.hpp"

namespace Spark {

std::int32_t SceneShadowParticipation::ResolveAgainstDefault(const std::int32_t defaultFlags) const noexcept {
    std::int32_t flags = defaultFlags;
    if (castOverride_.HasValue()) {
        if (*castOverride_) {
            flags |= kSceneShadowCast;
        } else {
            flags &= ~kSceneShadowCast;
        }
    }
    if (receiveOverride_.HasValue()) {
        if (*receiveOverride_) {
            flags |= kSceneShadowReceive;
        } else {
            flags &= ~kSceneShadowReceive;
        }
    }
    return flags;
}

void SceneShadowParticipation::ApplyTo(MaterialComponent& material) const noexcept {
    if (castOverride_.HasValue()) {
        material.SetShadowCastOverride(*castOverride_);
    } else {
        material.ClearShadowCastOverride();
    }
    if (receiveOverride_.HasValue()) {
        material.SetShadowReceiveOverride(*receiveOverride_);
    } else {
        material.ClearShadowReceiveOverride();
    }
}

SceneShadowParticipation SceneShadowParticipation::FromMaterial(const MaterialComponent& material) noexcept {
    SceneShadowParticipation p;
    p.castOverride_ = material.GetShadowCastOverride();
    p.receiveOverride_ = material.GetShadowReceiveOverride();
    return p;
}

}  // namespace Spark

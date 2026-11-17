#include "spark/render/lighting/SceneShadowParticipation.hpp"

#include "spark/ecs/components/rendering/MaterialComponent.hpp"

namespace Spark {

std::int32_t SceneShadowParticipation::ResolveAgainstDefault(const std::int32_t defaultFlags) const noexcept {
    std::int32_t flags = defaultFlags;
    if (castOverride.HasValue()) {
        if (*castOverride) {
            flags |= kSceneShadowCast;
        } else {
            flags &= ~kSceneShadowCast;
        }
    }
    if (receiveOverride.HasValue()) {
        if (*receiveOverride) {
            flags |= kSceneShadowReceive;
        } else {
            flags &= ~kSceneShadowReceive;
        }
    }
    return flags;
}

void SceneShadowParticipation::ApplyTo(MaterialComponent& material) const noexcept {
    if (castOverride.HasValue()) {
        material.SetShadowCastOverride(*castOverride);
    } else {
        material.ClearShadowCastOverride();
    }
    if (receiveOverride.HasValue()) {
        material.SetShadowReceiveOverride(*receiveOverride);
    } else {
        material.ClearShadowReceiveOverride();
    }
}

SceneShadowParticipation SceneShadowParticipation::FromMaterial(const MaterialComponent& material) noexcept {
    SceneShadowParticipation p;
    p.castOverride = material.GetShadowCastOverride();
    p.receiveOverride = material.GetShadowReceiveOverride();
    return p;
}

}  // namespace Spark

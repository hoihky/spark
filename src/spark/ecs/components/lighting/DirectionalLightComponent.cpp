#include "spark/ecs/components/lighting/DirectionalLightComponent.hpp"

#include "spark/ecs/GameObject.hpp"

namespace Spark {

DirectionalLightComponent::DirectionalLightComponent(Vector3 inColor, float inIntensity)
        : color(inColor), intensity(inIntensity) {}

void DirectionalLightComponent::OnSignal(GameObject& /*owner*/, SignalId /*id*/, const SignalPayload& /*payload*/) {}

void DirectionalLightComponent::SetColor(const Vector3& c) {
    color = c;
}

void DirectionalLightComponent::SetIntensity(const float v) {
    intensity = v;
}

void DirectionalLightComponent::SetEnabled(const bool e) {
    enabled = e;
}

void DirectionalLightComponent::SetCastsShadow(const bool c) {
    castsShadow = c;
}

}  // namespace Spark

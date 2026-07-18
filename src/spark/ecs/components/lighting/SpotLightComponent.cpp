#include "spark/ecs/components/lighting/SpotLightComponent.hpp"

#include "spark/ecs/GameObject.hpp"

namespace Spark {

SpotLightComponent::SpotLightComponent(
        Vector3 inColor, const float inIntensity, const float inRange, const float innerConeDeg, const float outerConeDeg)
    : color(inColor), intensity(inIntensity), range(inRange), innerConeDegrees(innerConeDeg), outerConeDegrees(outerConeDeg) {}

void SpotLightComponent::OnSignal(GameObject& /*owner*/, SignalId /*id*/, const SignalPayload& /*payload*/) {}

void SpotLightComponent::SetColor(const Vector3& c) {
    color = c;
}

void SpotLightComponent::SetIntensity(const float v) {
    intensity = v;
}

void SpotLightComponent::SetRange(const float r) {
    range = r;
}

void SpotLightComponent::SetInnerConeDegrees(const float deg) {
    innerConeDegrees = deg;
}

void SpotLightComponent::SetOuterConeDegrees(const float deg) {
    outerConeDegrees = deg;
}

void SpotLightComponent::SetEnabled(const bool e) {
    enabled = e;
}

void SpotLightComponent::SetCastsShadow(const bool c) {
    castsShadow = c;
}

}  // namespace Spark

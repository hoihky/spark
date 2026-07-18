#include "spark/ecs/components/lighting/PointLightComponent.hpp"

#include "spark/ecs/GameObject.hpp"

namespace Spark {

PointLightComponent::PointLightComponent(Vector3 inColor, float inIntensity, float inRange)
    : color(inColor), intensity(inIntensity), range(inRange) {}

void PointLightComponent::OnSignal(GameObject& /*owner*/, SignalId /*id*/, const SignalPayload& /*payload*/) {
}

void PointLightComponent::SetColor(const Vector3& c) {
    color = c;
}

void PointLightComponent::SetIntensity(float v) {
    intensity = v;
}

void PointLightComponent::SetRange(float r) {
    range = r;
}

void PointLightComponent::SetEnabled(bool e) {
    enabled = e;
}

void PointLightComponent::SetCastsShadow(bool c) {
    castsShadow = c;
}

}  // namespace Spark

#include "spark/ecs/components/lighting/PointLight2DComponent.hpp"

#include <algorithm>
#include <cmath>

namespace Spark {

PointLight2DComponent::PointLight2DComponent(Vector3 colorIn, float intensityIn, float rangeIn) noexcept
        : color(colorIn), intensity(intensityIn), effectiveIntensity(intensityIn), range(rangeIn) {}

void PointLight2DComponent::SetFlicker(const bool on, const float amount, const float frequencyHz) noexcept {
    flickerEnabled = on;
    flickerAmount = std::max(0.0F, amount);
    flickerHz = std::max(0.01F, frequencyHz);
}

void PointLight2DComponent::OnUpdate(const FrameTiming& timing, GameObject& /*owner*/, IEngineContext& /*context*/) {
    effectiveIntensity = intensity;
    if (!flickerEnabled) {
        return;
    }
    flickerPhase += timing.deltaTimeSeconds * flickerHz * 6.2831855F;
    const float wobble = 0.5F + 0.5F * std::sin(flickerPhase);
    effectiveIntensity = intensity * (1.0F - flickerAmount * wobble);
}

}  // namespace Spark

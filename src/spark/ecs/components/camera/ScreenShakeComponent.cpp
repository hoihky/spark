#include "spark/ecs/components/camera/ScreenShakeComponent.hpp"

#include <algorithm>
#include <cmath>

namespace Spark {

namespace {

[[nodiscard]] float HashNoise(const std::uint32_t seed, const float t) noexcept {
    const float x = static_cast<float>(seed) * 0.1031F + t * 12.9898F;
    return std::sin(x * 43758.5453F) * 2.0F - 1.0F;
}

}  // namespace

void ScreenShakeComponent::AddImpulse(
        const Vector2 amplitude,
        const float durationSeconds,
        const float frequencyHz) noexcept {
    if (durationSeconds <= 1.0e-5F) {
        return;
    }
    CameraShakeImpulse impulse{};
    impulse.amplitude = amplitude;
    impulse.durationSeconds = durationSeconds;
    impulse.frequencyHz = std::max(1.0F, frequencyHz);
    impulse.seed = nextSeed++;
    impulses.PushBack(impulse);
}

void ScreenShakeComponent::AddTrauma(const float amount) noexcept {
    trauma = std::clamp(trauma + amount, 0.0F, 1.0F);
}

void ScreenShakeComponent::Clear() noexcept {
    impulses.Clear();
    currentOffset = Vector2::Zero;
    trauma = 0.0F;
}

Vector2 ScreenShakeComponent::Tick(const float deltaSeconds) noexcept {
    shakeTime += deltaSeconds;
    currentOffset = Vector2::Zero;

    for (std::size_t i = impulses.GetSize(); i > 0; --i) {
        CameraShakeImpulse& impulse = impulses[i - 1];
        impulse.elapsedSeconds += deltaSeconds;
        if (impulse.elapsedSeconds >= impulse.durationSeconds) {
            impulses.RemoveAt(i - 1);
            continue;
        }
        const float u = impulse.elapsedSeconds / impulse.durationSeconds;
        const float envelope = (1.0F - u) * (1.0F - u);
        const float phase = shakeTime * impulse.frequencyHz * 6.2831853F;
        const float nx = HashNoise(impulse.seed, phase);
        const float ny = HashNoise(impulse.seed + 17U, phase + 1.7F);
        currentOffset.x += impulse.amplitude.x * envelope * nx;
        currentOffset.y += impulse.amplitude.y * envelope * ny;
    }

    if (trauma > 1.0e-4F) {
        const float traumaShake = trauma * trauma;
        const float phase = shakeTime * 32.0F;
        currentOffset.x += traumaShake * 0.35F * HashNoise(91U, phase);
        currentOffset.y += traumaShake * 0.28F * HashNoise(113U, phase + 2.1F);
        trauma = std::max(0.0F, trauma - traumaDecayPerSecond * deltaSeconds);
    }

    return currentOffset;
}

void ScreenShakeComponent::OnUpdate(
        const FrameTiming& timing,
        GameObject& /*owner*/,
        IEngineContext& /*context*/) {
    Tick(timing.deltaTimeSeconds);
}

}  // namespace Spark

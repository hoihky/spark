#pragma once

#include <cstdint>

namespace Spark {

/** CPU-side tuning for water screen-space reflections (W3-02 / W3-03). */
class WaterScreenSpaceReflectionSettings {
public:
    static constexpr std::int32_t kDefaultMaxSteps = 24;
    static constexpr float kDefaultMaxRayDistance = 48.0F;
    static constexpr float kDefaultThickness = 0.15F;
    static constexpr float kDefaultStepScale = 1.0F;
    static constexpr float kDefaultStrength = 1.0F;
    static constexpr float kDefaultHorizonFadeStart = -0.02F;
    static constexpr float kDefaultHorizonFadeEnd = -0.30F;

    [[nodiscard]] bool IsEnabled() const noexcept { return enabled; }
    [[nodiscard]] std::int32_t GetMaxSteps() const noexcept { return maxSteps; }
    [[nodiscard]] float GetMaxRayDistance() const noexcept { return maxRayDistance; }
    [[nodiscard]] float GetThickness() const noexcept { return thickness; }
    [[nodiscard]] float GetStepScale() const noexcept { return stepScale; }
    [[nodiscard]] float GetStrength() const noexcept { return strength; }
    [[nodiscard]] float GetHorizonFadeStart() const noexcept { return horizonFadeStart; }
    [[nodiscard]] float GetHorizonFadeEnd() const noexcept { return horizonFadeEnd; }

    void SetEnabled(const bool value) noexcept { enabled = value; }
    void SetMaxSteps(const std::int32_t value) noexcept { maxSteps = value > 0 ? value : 1; }
    void SetMaxRayDistance(const float value) noexcept { maxRayDistance = value > 0.0F ? value : kDefaultMaxRayDistance; }
    void SetThickness(const float value) noexcept { thickness = value > 0.0F ? value : kDefaultThickness; }
    void SetStepScale(const float value) noexcept { stepScale = value > 0.0F ? value : kDefaultStepScale; }
    void SetStrength(const float value) noexcept { strength = value; }
    void SetHorizonFadeStart(const float value) noexcept { horizonFadeStart = value; }
    void SetHorizonFadeEnd(const float value) noexcept { horizonFadeEnd = value; }

private:
    bool enabled = true;
    std::int32_t maxSteps = kDefaultMaxSteps;
    float maxRayDistance = kDefaultMaxRayDistance;
    float thickness = kDefaultThickness;
    float stepScale = kDefaultStepScale;
    float strength = kDefaultStrength;
    float horizonFadeStart = kDefaultHorizonFadeStart;
    float horizonFadeEnd = kDefaultHorizonFadeEnd;
};

}  // namespace Spark

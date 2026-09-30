#pragma once

#include "spark/math/Vector3.hpp"

namespace Spark {

/**
 * Authoring-time wind parameters (F0). Applied by <c>WindEnvironmentComponent</c> and simulated by
 * <c>WindSubsystem</c> each frame.
 */
class WindSettings {
public:
    WindSettings() = default;

    void SetEnabled(bool value) noexcept { enabled = value; }
    [[nodiscard]] bool IsEnabled() const noexcept { return enabled; }

    /** Horizontal direction in world space; Y is ignored and forced to zero when normalized. */
    void SetDirectionWorld(const Vector3& directionWorld) noexcept { direction = directionWorld; }
    [[nodiscard]] Vector3 GetDirectionWorld() const noexcept { return direction; }

    void SetBaseSpeedMetersPerSecond(float metersPerSecond) noexcept { baseSpeed = metersPerSecond; }
    [[nodiscard]] float GetBaseSpeedMetersPerSecond() const noexcept { return baseSpeed; }

    void SetGustAmplitude(float value) noexcept { gustAmplitude = value; }
    [[nodiscard]] float GetGustAmplitude() const noexcept { return gustAmplitude; }

    void SetGustFrequencyHertz(float value) noexcept { gustFrequency = value; }
    [[nodiscard]] float GetGustFrequencyHertz() const noexcept { return gustFrequency; }

    void SetTurbulence(float value) noexcept { turbulence = value; }
    [[nodiscard]] float GetTurbulence() const noexcept { return turbulence; }

    /** Returns XZ-normalized direction, or +X when near zero length. */
    [[nodiscard]] Vector3 NormalizedDirectionXZ() const noexcept;

private:
    bool enabled = true;
    Vector3 direction{1.0F, 0.0F, 0.0F};
    float baseSpeed = 0.0F;
    float gustAmplitude = 0.35F;
    float gustFrequency = 0.22F;
    float turbulence = 0.45F;
};

}  // namespace Spark

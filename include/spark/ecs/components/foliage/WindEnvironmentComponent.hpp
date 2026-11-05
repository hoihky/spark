#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/scene/foliage/WindSettings.hpp"

namespace Spark {

/**
 * Drives global wind for the world via <c>WindSubsystem</c> (F0).
 * Place one instance on the scene root (or the first active component wins).
 */
class WindEnvironmentComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::WindEnvironment;

    WindEnvironmentComponent() = default;
    explicit WindEnvironmentComponent(WindSettings settingsIn) : settings(settingsIn) {}

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    void OnUpdate(const FrameTiming& timing, GameObject& owner, IEngineContext& context) override;

    WindSettings& GetSettings() noexcept { return settings; }
    [[nodiscard]] const WindSettings& GetSettings() const noexcept { return settings; }

    void SetWindEnabled(bool value) noexcept { settings.SetEnabled(value); }

    [[nodiscard]] bool IsWindEnabled() const noexcept { return settings.IsEnabled(); }

    void SetDirectionWorld(const Vector3& directionWorld) noexcept { settings.SetDirectionWorld(directionWorld); }

    [[nodiscard]] Vector3 GetDirectionWorld() const noexcept { return settings.GetDirectionWorld(); }

    void SetBaseSpeedMetersPerSecond(float metersPerSecond) noexcept
    {
        settings.SetBaseSpeedMetersPerSecond(metersPerSecond);
    }

    [[nodiscard]] float GetBaseSpeedMetersPerSecond() const noexcept
    {
        return settings.GetBaseSpeedMetersPerSecond();
    }

    void SetGustAmplitude(float value) noexcept { settings.SetGustAmplitude(value); }

    [[nodiscard]] float GetGustAmplitude() const noexcept { return settings.GetGustAmplitude(); }

    void SetGustFrequencyHertz(float hertz) noexcept { settings.SetGustFrequencyHertz(hertz); }

    [[nodiscard]] float GetGustFrequencyHertz() const noexcept { return settings.GetGustFrequencyHertz(); }

    void SetTurbulence(float value) noexcept { settings.SetTurbulence(value); }

    [[nodiscard]] float GetTurbulence() const noexcept { return settings.GetTurbulence(); }

private:
    WindSettings settings{};
};

}  // namespace Spark

#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/scripting/SparkScriptBind.hpp"
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

    SPARK_SCRIPT_BIND(set_enabled)
    void SetWindEnabled(bool value) noexcept { settings.SetEnabled(value); }

    SPARK_SCRIPT_BIND(get_enabled)
    [[nodiscard]] bool IsWindEnabled() const noexcept { return settings.IsEnabled(); }

    SPARK_SCRIPT_BIND(set_direction)
    void SetDirectionWorld(const Vector3& directionWorld) noexcept { settings.SetDirectionWorld(directionWorld); }

    SPARK_SCRIPT_BIND(get_direction)
    [[nodiscard]] Vector3 GetDirectionWorld() const noexcept { return settings.GetDirectionWorld(); }

    SPARK_SCRIPT_BIND(set_base_speed)
    void SetBaseSpeedMetersPerSecond(float metersPerSecond) noexcept
    {
        settings.SetBaseSpeedMetersPerSecond(metersPerSecond);
    }

    SPARK_SCRIPT_BIND(get_base_speed)
    [[nodiscard]] float GetBaseSpeedMetersPerSecond() const noexcept
    {
        return settings.GetBaseSpeedMetersPerSecond();
    }

    SPARK_SCRIPT_BIND(set_gust_amplitude)
    void SetGustAmplitude(float value) noexcept { settings.SetGustAmplitude(value); }

    SPARK_SCRIPT_BIND(get_gust_amplitude)
    [[nodiscard]] float GetGustAmplitude() const noexcept { return settings.GetGustAmplitude(); }

    SPARK_SCRIPT_BIND(set_gust_frequency)
    void SetGustFrequencyHertz(float hertz) noexcept { settings.SetGustFrequencyHertz(hertz); }

    SPARK_SCRIPT_BIND(get_gust_frequency)
    [[nodiscard]] float GetGustFrequencyHertz() const noexcept { return settings.GetGustFrequencyHertz(); }

    SPARK_SCRIPT_BIND(set_turbulence)
    void SetTurbulence(float value) noexcept { settings.SetTurbulence(value); }

    SPARK_SCRIPT_BIND(get_turbulence)
    [[nodiscard]] float GetTurbulence() const noexcept { return settings.GetTurbulence(); }

private:
    WindSettings settings{};
};

}  // namespace Spark

#pragma once

#include "spark/ecs/GameComponent.hpp"
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

private:
    WindSettings settings{};
};

}  // namespace Spark

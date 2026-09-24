#pragma once

#include "spark/ecs/components/physics/2d/TriggerVolume2DComponent.hpp"
#include "spark/engine/FrameTiming.hpp"
#include "spark/physics/Collision2D.hpp"

namespace Spark {

struct TriggerVolume2DWorld {
    TriggerVolume2DShape shape = TriggerVolume2DShape::Box;
    CollisionAabb2 box{};
    float circleCx = 0.0F;
    float circleCy = 0.0F;
    float circleR = 0.5F;
    CollisionAabb2 bounds{};
};

struct TriggerProbe2DWorld {
    CollisionAabb2 box{};
    float circleCx = 0.0F;
    float circleCy = 0.0F;
    float circleR = 0.5F;
    bool useCircle = false;
};

struct TriggerVolume2DSettings {
    bool includeCharacterControllers = true;
    bool includeDynamicRigidbodies = true;
    bool includeColliderWithoutRigidbody = true;
};

void BuildTriggerVolume2DWorld(
        GameObject& owner,
        const TriggerVolume2DComponent& volume,
        TriggerVolume2DWorld& outWorld) noexcept;

[[nodiscard]] bool TriggerVolume2DOverlapsProbe(
        const TriggerVolume2DWorld& volume,
        const TriggerProbe2DWorld& probe) noexcept;

[[nodiscard]] bool TryBuildTriggerProbe2DFromObject(
        GameObject& object,
        const TriggerVolume2DSettings& settings,
        TriggerProbe2DWorld& outProbe) noexcept;

class TriggerVolumeWorld2D {
public:
    TriggerVolumeWorld2D() = default;
    explicit TriggerVolumeWorld2D(TriggerVolume2DSettings settingsIn) noexcept : settings(settingsIn) {}

    void Simulate(GameWorld& world, const FrameTiming& timing);

    [[nodiscard]] TriggerVolume2DSettings& GetSettings() noexcept { return settings; }
    [[nodiscard]] const TriggerVolume2DSettings& GetSettings() const noexcept { return settings; }
    void SetSettings(TriggerVolume2DSettings settingsIn) noexcept { settings = settingsIn; }

private:
    TriggerVolume2DSettings settings{};
};

void SimulateTriggerVolumes2D(
        GameWorld& world,
        const FrameTiming& timing,
        const TriggerVolume2DSettings& settings = {});

}  // namespace Spark

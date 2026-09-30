#pragma once

#include "spark/scene/foliage/WindFrameState.hpp"
#include "spark/scene/foliage/WindSettings.hpp"

namespace Spark {

class GameWorld;
struct SceneRenderParams;

/**
 * Per-<c>GameWorld</c> wind simulation (F0). <c>WindEnvironmentComponent</c> registers settings;
 * scene submit copies <c>WindFrameState</c> into <c>SceneRenderParams</c>.
 */
class WindSubsystem {
public:
    void Reset() noexcept;

    /** Called from <c>WindEnvironmentComponent</c> when it is the active environment driver. */
    void ApplyEnvironmentSettings(const WindSettings& settings) noexcept;

    void AdvanceSimulation(float deltaTimeSeconds) noexcept;

    [[nodiscard]] const WindFrameState& GetFrameState() const noexcept { return frameState; }

    void WriteFrameToSceneRenderParams(SceneRenderParams& params) const noexcept;

private:
    WindSettings activeSettings{};
    bool hasEnvironment = false;
    float simulationTimeSeconds = 0.0F;
    WindFrameState frameState{};
};

}  // namespace Spark

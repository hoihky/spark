#pragma once

#include "spark/engine/SceneRenderParams.hpp"

namespace Spark {

class GameWorld;

/** Resolved time-of-day from <c>TimeOfDayDriverComponent</c> (highest priority wins). */
struct FrameTimeOfDayState {
    bool active = false;
    bool useTimeOfDay = false;
    float timeOfDay = 0.5F;
};

[[nodiscard]] FrameTimeOfDayState& GetFrameTimeOfDayState() noexcept;
[[nodiscard]] const FrameTimeOfDayState& GetFrameTimeOfDayStateConst() noexcept;

void ProcessTimeOfDayDrivers(GameWorld& world, float deltaTimeSeconds) noexcept;

/** Applies fog/post-process volumes at <c>cameraPositionWorld</c> into <c>params</c>. */
void ApplyRegionalRenderVolumes(
        const GameWorld& world,
        const Vector3& cameraPositionWorld,
        SceneRenderParams& params) noexcept;

}  // namespace Spark

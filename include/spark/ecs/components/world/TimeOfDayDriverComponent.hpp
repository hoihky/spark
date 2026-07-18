#pragma once

#include "spark/ecs/GameComponent.hpp"

namespace Spark {

/** Drives <c>SceneRenderParams::useTimeOfDay</c> / <c>timeOfDay</c> each frame before scene submit. */
class TimeOfDayDriverComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::TimeOfDayDriver;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    [[nodiscard]] bool IsEnabled() const noexcept { return enabled; }
    void SetEnabled(const bool e) noexcept { enabled = e; }

    [[nodiscard]] bool IsLooping() const noexcept { return loop; }
    void SetLooping(const bool l) noexcept { loop = l; }

    /** Seconds for a full 0→1 cycle when <c>loop</c> is true; ignored when zero (manual time only). */
    [[nodiscard]] float GetDayLengthSeconds() const noexcept { return dayLengthSeconds; }
    void SetDayLengthSeconds(const float s) noexcept { dayLengthSeconds = s; }

    [[nodiscard]] float GetTimeOfDay() const noexcept { return timeOfDay; }
    void SetTimeOfDay(const float t) noexcept { timeOfDay = t; }

    [[nodiscard]] std::int32_t GetPriority() const noexcept { return priority; }
    void SetPriority(const std::int32_t p) noexcept { priority = p; }

private:
    bool enabled = true;
    bool loop = true;
    float dayLengthSeconds = 120.0F;
    float timeOfDay = 0.35F;
    std::int32_t priority = 0;
};

}  // namespace Spark

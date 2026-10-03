#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/scripting/SparkScriptBind.hpp"

namespace Spark {

/** Drives <c>SceneRenderParams::useTimeOfDay</c> / <c>timeOfDay</c> each frame before scene submit. */
class TimeOfDayDriverComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::TimeOfDayDriver;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    SPARK_SCRIPT_BIND(is_enabled)
    [[nodiscard]] bool IsEnabled() const noexcept { return enabled; }
    SPARK_SCRIPT_BIND(set_enabled)
    void SetEnabled(const bool e) noexcept { enabled = e; }

    SPARK_SCRIPT_BIND(is_looping)
    [[nodiscard]] bool IsLooping() const noexcept { return loop; }
    SPARK_SCRIPT_BIND(set_looping)
    void SetLooping(const bool l) noexcept { loop = l; }

    /** Seconds for a full 0→1 cycle when <c>loop</c> is true; ignored when zero (manual time only). */
    SPARK_SCRIPT_BIND(get_day_length_seconds)
    [[nodiscard]] float GetDayLengthSeconds() const noexcept { return dayLengthSeconds; }
    SPARK_SCRIPT_BIND(set_day_length_seconds)
    void SetDayLengthSeconds(const float s) noexcept { dayLengthSeconds = s; }

    SPARK_SCRIPT_BIND(get_time_of_day)
    [[nodiscard]] float GetTimeOfDay() const noexcept { return timeOfDay; }
    SPARK_SCRIPT_BIND(set_time_of_day)
    void SetTimeOfDay(const float t) noexcept { timeOfDay = t; }

    SPARK_SCRIPT_BIND(get_priority)
    [[nodiscard]] std::int32_t GetPriority() const noexcept { return priority; }
    SPARK_SCRIPT_BIND(set_priority)
    void SetPriority(const std::int32_t p) noexcept { priority = p; }

private:
    bool enabled = true;
    bool loop = true;
    float dayLengthSeconds = 120.0F;
    float timeOfDay = 0.35F;
    std::int32_t priority = 0;
};

}  // namespace Spark

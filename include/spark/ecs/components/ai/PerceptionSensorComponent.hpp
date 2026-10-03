#pragma once

#include "spark/core/Array.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/ecs/GameObject.hpp"

#include <cstdint>
#include "spark/scripting/SparkScriptBind.hpp"

namespace Spark {

/** Sight/hearing query updated each frame by <c>ProcessPerceptionSensors</c>. */
class PerceptionSensorComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::PerceptionSensor;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    SPARK_SCRIPT_BIND(is_enabled)
    [[nodiscard]] bool IsEnabled() const noexcept { return enabled; }
    SPARK_SCRIPT_BIND(set_enabled)
    void SetEnabled(const bool e) noexcept { enabled = e; }

    SPARK_SCRIPT_BIND(get_sight_radius)
    [[nodiscard]] float GetSightRadius() const noexcept { return sightRadius; }
    SPARK_SCRIPT_BIND(set_sight_radius)
    void SetSightRadius(const float r) noexcept { sightRadius = r; }

    SPARK_SCRIPT_BIND(get_hearing_radius)
    [[nodiscard]] float GetHearingRadius() const noexcept { return hearingRadius; }
    SPARK_SCRIPT_BIND(set_hearing_radius)
    void SetHearingRadius(const float r) noexcept { hearingRadius = r; }

    SPARK_SCRIPT_BIND(get_sight_fov_degrees)
    [[nodiscard]] float GetSightFovDegrees() const noexcept { return sightFovDegrees; }
    SPARK_SCRIPT_BIND(set_sight_fov_degrees)
    void SetSightFovDegrees(const float d) noexcept { sightFovDegrees = d; }

    SPARK_SCRIPT_BIND(get_target_category_mask)
    [[nodiscard]] std::uint16_t GetTargetCategoryMask() const noexcept { return targetCategoryMask; }
    SPARK_SCRIPT_BIND(set_target_category_mask)
    void SetTargetCategoryMask(const std::uint16_t m) noexcept { targetCategoryMask = m; }

    [[nodiscard]] const Array<GameObject*>& GetDetectedObjects() const noexcept { return detected; }
    SPARK_SCRIPT_BIND(clear_detected)
    void ClearDetected() noexcept { detected.Clear(); }

    void AddDetected(GameObject* o) noexcept {
        if (o != nullptr) {
            detected.PushBack(o);
        }
    }

private:
    bool enabled = true;
    float sightRadius = 12.0F;
    float hearingRadius = 8.0F;
    float sightFovDegrees = 120.0F;
    std::uint16_t targetCategoryMask = 0xFFFFu;
    Array<GameObject*> detected{};
};

}  // namespace Spark

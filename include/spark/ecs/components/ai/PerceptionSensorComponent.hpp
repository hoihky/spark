#pragma once

#include "spark/core/Array.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/ecs/GameObject.hpp"

#include <cstdint>

namespace Spark {

/** Sight/hearing query updated each frame by <c>ProcessPerceptionSensors</c>. */
class PerceptionSensorComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::PerceptionSensor;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    [[nodiscard]] bool IsEnabled() const noexcept { return enabled; }
    void SetEnabled(const bool e) noexcept { enabled = e; }

    [[nodiscard]] float GetSightRadius() const noexcept { return sightRadius; }
    void SetSightRadius(const float r) noexcept { sightRadius = r; }

    [[nodiscard]] float GetHearingRadius() const noexcept { return hearingRadius; }
    void SetHearingRadius(const float r) noexcept { hearingRadius = r; }

    [[nodiscard]] float GetSightFovDegrees() const noexcept { return sightFovDegrees; }
    void SetSightFovDegrees(const float d) noexcept { sightFovDegrees = d; }

    [[nodiscard]] std::uint16_t GetTargetCategoryMask() const noexcept { return targetCategoryMask; }
    void SetTargetCategoryMask(const std::uint16_t m) noexcept { targetCategoryMask = m; }

    [[nodiscard]] const Array<GameObject*>& GetDetectedObjects() const noexcept { return detected; }
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

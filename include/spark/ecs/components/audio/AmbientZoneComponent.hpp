#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/scene/VolumeRegions.hpp"

namespace Spark {

/**
 * Region-based audio mix applied when the listener is inside the volume (priority resolves overlaps).
 * Processed by <c>ProcessAmbientZones</c> before sound cues flush.
 */
class AmbientZoneComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::AmbientZone;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    [[nodiscard]] bool IsEnabled() const noexcept { return enabled; }
    void SetEnabled(const bool e) noexcept { enabled = e; }

    [[nodiscard]] VolumeShape GetShape() const noexcept { return shape; }
    void SetShape(const VolumeShape s) noexcept { shape = s; }

    [[nodiscard]] const Vector3& GetHalfExtents() const noexcept { return halfExtents; }
    void SetHalfExtents(const Vector3& e) noexcept { halfExtents = e; }

    [[nodiscard]] float GetVolumeScale() const noexcept { return volumeScale; }
    void SetVolumeScale(const float v) noexcept { volumeScale = v; }

    [[nodiscard]] float GetLowPassAmount() const noexcept { return lowPassAmount; }
    void SetLowPassAmount(const float a) noexcept { lowPassAmount = a; }

    [[nodiscard]] std::int32_t GetPriority() const noexcept { return priority; }
    void SetPriority(const std::int32_t p) noexcept { priority = p; }

private:
    bool enabled = true;
    VolumeShape shape = VolumeShape::Box;
    Vector3 halfExtents{4.0F, 3.0F, 4.0F};
    float volumeScale = 0.85F;
    float lowPassAmount = 0.0F;
    std::int32_t priority = 0;
};

}  // namespace Spark

#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/scene/volume/VolumeRegions.hpp"
#include "spark/scripting/SparkScriptBind.hpp"

namespace Spark {

/**
 * Region-based audio mix applied when the listener is inside the volume (priority resolves overlaps).
 * Processed by <c>ProcessAmbientZones</c> before sound cues flush.
 */
class AmbientZoneComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::AmbientZone;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    SPARK_SCRIPT_BIND(is_enabled)
    [[nodiscard]] bool IsEnabled() const noexcept { return enabled; }
    SPARK_SCRIPT_BIND(set_enabled)
    void SetEnabled(const bool e) noexcept { enabled = e; }

    SPARK_SCRIPT_BIND(get_shape)
    [[nodiscard]] VolumeShape GetShape() const noexcept { return shape; }
    SPARK_SCRIPT_BIND(set_shape)
    void SetShape(const VolumeShape s) noexcept { shape = s; }

    SPARK_SCRIPT_BIND(get_half_extents)
    [[nodiscard]] const Vector3& GetHalfExtents() const noexcept { return halfExtents; }
    SPARK_SCRIPT_BIND(set_half_extents)
    void SetHalfExtents(const Vector3& e) noexcept { halfExtents = e; }

    SPARK_SCRIPT_BIND(get_volume_scale)
    [[nodiscard]] float GetVolumeScale() const noexcept { return volumeScale; }
    SPARK_SCRIPT_BIND(set_volume_scale)
    void SetVolumeScale(const float v) noexcept { volumeScale = v; }

    SPARK_SCRIPT_BIND(get_low_pass_amount)
    [[nodiscard]] float GetLowPassAmount() const noexcept { return lowPassAmount; }
    SPARK_SCRIPT_BIND(set_low_pass_amount)
    void SetLowPassAmount(const float a) noexcept { lowPassAmount = a; }

    SPARK_SCRIPT_BIND(get_priority)
    [[nodiscard]] std::int32_t GetPriority() const noexcept { return priority; }
    SPARK_SCRIPT_BIND(set_priority)
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

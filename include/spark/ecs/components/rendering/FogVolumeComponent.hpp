#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/scene/volume/VolumeRegions.hpp"
#include "spark/scripting/SparkScriptBind.hpp"

namespace Spark {

/** Regional fog overrides collected into <c>SceneRenderParams</c> each submit. */
class FogVolumeComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::FogVolume;

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

    SPARK_SCRIPT_BIND(get_fog_color)
    [[nodiscard]] Vector3 GetFogColor() const noexcept { return fogColor; }
    SPARK_SCRIPT_BIND(set_fog_color)
    void SetFogColor(const Vector3& c) noexcept { fogColor = c; }

    SPARK_SCRIPT_BIND(get_fog_density)
    [[nodiscard]] float GetFogDensity() const noexcept { return fogDensity; }
    SPARK_SCRIPT_BIND(set_fog_density)
    void SetFogDensity(const float d) noexcept { fogDensity = d; }

    SPARK_SCRIPT_BIND(get_fog_start)
    [[nodiscard]] float GetFogStart() const noexcept { return fogStart; }
    SPARK_SCRIPT_BIND(set_fog_start)
    void SetFogStart(const float s) noexcept { fogStart = s; }

    SPARK_SCRIPT_BIND(get_fog_end)
    [[nodiscard]] float GetFogEnd() const noexcept { return fogEnd; }
    SPARK_SCRIPT_BIND(set_fog_end)
    void SetFogEnd(const float e) noexcept { fogEnd = e; }

    SPARK_SCRIPT_BIND(get_priority)
    [[nodiscard]] std::int32_t GetPriority() const noexcept { return priority; }
    SPARK_SCRIPT_BIND(set_priority)
    void SetPriority(const std::int32_t p) noexcept { priority = p; }

private:
    bool enabled = true;
    VolumeShape shape = VolumeShape::Box;
    Vector3 halfExtents{8.0F, 6.0F, 8.0F};
    Vector3 fogColor{0.72F, 0.78F, 0.86F};
    float fogDensity = 0.02F;
    float fogStart = 4.0F;
    float fogEnd = 64.0F;
    std::int32_t priority = 0;
};

}  // namespace Spark

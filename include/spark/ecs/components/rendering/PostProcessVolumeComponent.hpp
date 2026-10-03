#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/scene/volume/VolumeRegions.hpp"
#include "spark/scripting/SparkScriptBind.hpp"

namespace Spark {

/** Regional post-process overrides (SSAO, exposure) blended into <c>SceneRenderParams</c>. */
class PostProcessVolumeComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::PostProcessVolume;

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

    SPARK_SCRIPT_BIND(get_priority)
    [[nodiscard]] std::int32_t GetPriority() const noexcept { return priority; }
    SPARK_SCRIPT_BIND(set_priority)
    void SetPriority(const std::int32_t p) noexcept { priority = p; }

    SPARK_SCRIPT_BIND(has_s_ssao_override)
    [[nodiscard]] bool HasSsaoOverride() const noexcept { return hasSsao; }
    SPARK_SCRIPT_BIND(get_ssao_enabled)
    [[nodiscard]] bool GetSsaoEnabled() const noexcept { return ssaoEnabled; }
    void SetSsaoEnabled(const bool e) noexcept {
        hasSsao = true;
        ssaoEnabled = e;
    }

    SPARK_SCRIPT_BIND(has_s_exposure_override)
    [[nodiscard]] bool HasExposureOverride() const noexcept { return hasExposure; }
    SPARK_SCRIPT_BIND(get_exposure)
    [[nodiscard]] float GetExposure() const noexcept { return exposure; }
    void SetExposure(const float e) noexcept {
        hasExposure = true;
        exposure = e;
    }

    SPARK_SCRIPT_BIND(has_s_ambient_scale_override)
    [[nodiscard]] bool HasAmbientScaleOverride() const noexcept { return hasAmbientScale; }
    SPARK_SCRIPT_BIND(get_ambient_scale)
    [[nodiscard]] float GetAmbientScale() const noexcept { return ambientScale; }
    void SetAmbientScale(const float s) noexcept {
        hasAmbientScale = true;
        ambientScale = s;
    }

private:
    bool enabled = true;
    VolumeShape shape = VolumeShape::Box;
    Vector3 halfExtents{6.0F, 4.0F, 6.0F};
    std::int32_t priority = 0;
    bool hasSsao = false;
    bool ssaoEnabled = true;
    bool hasExposure = false;
    float exposure = 1.0F;
    bool hasAmbientScale = false;
    float ambientScale = 1.0F;
};

}  // namespace Spark

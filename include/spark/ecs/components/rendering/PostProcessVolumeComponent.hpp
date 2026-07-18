#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/scene/VolumeRegions.hpp"

namespace Spark {

/** Regional post-process overrides (SSAO, exposure) blended into <c>SceneRenderParams</c>. */
class PostProcessVolumeComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::PostProcessVolume;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    [[nodiscard]] bool IsEnabled() const noexcept { return enabled; }
    void SetEnabled(const bool e) noexcept { enabled = e; }

    [[nodiscard]] VolumeShape GetShape() const noexcept { return shape; }
    void SetShape(const VolumeShape s) noexcept { shape = s; }

    [[nodiscard]] const Vector3& GetHalfExtents() const noexcept { return halfExtents; }
    void SetHalfExtents(const Vector3& e) noexcept { halfExtents = e; }

    [[nodiscard]] std::int32_t GetPriority() const noexcept { return priority; }
    void SetPriority(const std::int32_t p) noexcept { priority = p; }

    [[nodiscard]] bool HasSsaoOverride() const noexcept { return hasSsao; }
    [[nodiscard]] bool GetSsaoEnabled() const noexcept { return ssaoEnabled; }
    void SetSsaoEnabled(const bool e) noexcept {
        hasSsao = true;
        ssaoEnabled = e;
    }

    [[nodiscard]] bool HasExposureOverride() const noexcept { return hasExposure; }
    [[nodiscard]] float GetExposure() const noexcept { return exposure; }
    void SetExposure(const float e) noexcept {
        hasExposure = true;
        exposure = e;
    }

    [[nodiscard]] bool HasAmbientScaleOverride() const noexcept { return hasAmbientScale; }
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

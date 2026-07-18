#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/scene/VolumeRegions.hpp"

namespace Spark {

/** Regional fog overrides collected into <c>SceneRenderParams</c> each submit. */
class FogVolumeComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::FogVolume;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    [[nodiscard]] bool IsEnabled() const noexcept { return enabled; }
    void SetEnabled(const bool e) noexcept { enabled = e; }

    [[nodiscard]] VolumeShape GetShape() const noexcept { return shape; }
    void SetShape(const VolumeShape s) noexcept { shape = s; }

    [[nodiscard]] const Vector3& GetHalfExtents() const noexcept { return halfExtents; }
    void SetHalfExtents(const Vector3& e) noexcept { halfExtents = e; }

    [[nodiscard]] Vector3 GetFogColor() const noexcept { return fogColor; }
    void SetFogColor(const Vector3& c) noexcept { fogColor = c; }

    [[nodiscard]] float GetFogDensity() const noexcept { return fogDensity; }
    void SetFogDensity(const float d) noexcept { fogDensity = d; }

    [[nodiscard]] float GetFogStart() const noexcept { return fogStart; }
    void SetFogStart(const float s) noexcept { fogStart = s; }

    [[nodiscard]] float GetFogEnd() const noexcept { return fogEnd; }
    void SetFogEnd(const float e) noexcept { fogEnd = e; }

    [[nodiscard]] std::int32_t GetPriority() const noexcept { return priority; }
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
